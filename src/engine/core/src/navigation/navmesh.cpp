#include "halley/navigation/navmesh.h"

#include <cassert>

#include "halley/data_structures/priority_queue.h"
#include "halley/maths/random.h"
#include "halley/maths/ray.h"
#include "halley/support/logger.h"
#include "halley/bytes/byte_serializer.h"
using namespace Halley;

Navmesh::Navmesh()
{}

Navmesh::Navmesh(Vector<PolygonData> polys, const NavmeshBounds& bounds, int subWorld)
	: subWorld(subWorld)
	, origin(bounds.origin)
	, normalisedCoordinatesBase(bounds.base)
	, scaleFactor(bounds.scaleFactor)
{
	Expects(polys.size() < static_cast<size_t>(std::numeric_limits<NodeId>::max()));

	// Generate nodes
	nodes.resize(polys.size());
	for (size_t i = 0; i < polys.size(); ++i) {
		assert(polys[i].connections.size() == polys[i].polygon.getNumSides());

		auto& node = nodes[i];
		
		node.pos = polys[i].polygon.getBoundingCircle().getCentre();
		node.nConnections = polys[i].connections.size();
		size_t j = 0;
		for (auto& connection: polys[i].connections) {
			const float cost = connection >= 0 ? ((polys[connection].polygon.getCentre() - polys[i].polygon.getCentre()) * scaleFactor).length() * polys[connection].weight : std::numeric_limits<float>::infinity();
			node.connections[j] = connection >= 0 ? OptionalLite<NodeId>(gsl::narrow<NodeId>(connection)) : OptionalLite<NodeId>();
			node.costs[j] = cost;
			
			if (connection < -1) {
				// Portal
				const int id = -connection - 2;
				addToPortals(NodeAndConn(static_cast<uint16_t>(i), static_cast<uint16_t>(j)), id);
			}
			
			++j;
		}
	}

	// Move polygons
	polygons.resize(polys.size());
	for (size_t i = 0; i < polys.size(); ++i) {
		polygons[i] = std::move(polys[i].polygon);
	}

	// Move Weights
	weights.resize(polys.size());
	for (size_t i = 0; i < polys.size(); i++) {
		weights[i] = polys.at(i).weight;
	}
	processPolygons();

	// Generate edges
	postProcessPortals();
	generateOpenEdges();
}

Navmesh::Navmesh(const ConfigNode& nodeData)
{
	scaleFactor = nodeData["scaleFactor"].asVector2f(Vector2f(1.0f, 1.0f));
	origin = nodeData["origin"].asVector2f(Vector2f(0, 0));
	normalisedCoordinatesBase = nodeData.hasKey("base") ? Base2D(nodeData["base"]) : Base2D(Vector2f(1.0f, 0.0f), Vector2f(0.0f, 1.0f));
	nodes = nodeData["nodes"].asVector<Node>();
	polygons = nodeData["polygons"].asVector<Polygon>();
	portals = nodeData["portals"].asVector<Portal>();
	subWorld = nodeData["subWorld"].asInt(0);
	weights = nodeData["weights"].asVector<float>({});

	processPolygons();
	generateOpenEdges();
}

ConfigNode Navmesh::toConfigNode() const
{
	ConfigNode::MapType result;

	result["scaleFactor"] = scaleFactor;
	result["origin"] = origin;
	result["base"] = normalisedCoordinatesBase.toConfigNode();
	result["nodes"] = nodes;
	result["polygons"] = polygons;
	result["portals"] = portals;
	result["subWorld"] = subWorld;
	result["weights"] = weights;
	
	return result;
}

void Navmesh::serialize(Serializer& s) const
{
	s << scaleFactor;
	s << origin;
	s << normalisedCoordinatesBase;
	s << nodes;
	s << polygons;
	s << portals;
	s << subWorld;
	s << weights;
}

void Navmesh::deserialize(Deserializer& s)
{
	s >> scaleFactor;
	s >> origin;
	s >> normalisedCoordinatesBase;
	s >> nodes;
	s >> polygons;
	s >> portals;
	s >> subWorld;
	s >> weights;

	processPolygons();
	generateOpenEdges();
}

const Polygon& Navmesh::getPolygon(int id) const
{
	return polygons[id];
}

std::optional<Navmesh::NodeId> Navmesh::getNodeAt(Vector2f position) const
{
	const auto& polyIndices = getPolygonsAt(position, true);
	
	for (auto i: polyIndices) {
		if (polygons[i].isPointInside(position)) {
			return i;
		}
	}

	const float maxDistanceToPolygon = 5.0f;

	// Haven't found, look for the closest one in this grid cell...
	{
		float bestDist = std::numeric_limits<float>::infinity();
		int bestNode = -1;
		for (auto i: polyIndices) {
			const auto p = polygons[i].getClosestPoint(position);
			const float distSquared = (p - position).squaredLength();
			if (distSquared < bestDist && std::sqrt(distSquared) < maxDistanceToPolygon) {
				bestDist = distSquared;
				bestNode = i;
			}
		}
		if (bestNode != -1) {
			return gsl::narrow<NodeId>(bestNode);
		}
	}

	// If we don't find it even in this cell, then look on the open edges
	{
		float bestDist = std::numeric_limits<float>::infinity();
		int bestNode = -1;
		for (const auto& edge: openEdges) {
			const float distSquared = (position - edge.second.getClosestPoint(position)).squaredLength();
			if (distSquared < bestDist && std::sqrt(distSquared) < maxDistanceToPolygon) {
				bestDist = distSquared;
				bestNode = edge.first;
			}
		}
		if (bestNode != -1) {
			return gsl::narrow<NodeId>(bestNode);
		}
	}

	// Give up
	return {};
}

bool Navmesh::containsPoint(Vector2f position) const
{
	const auto& polyIndices = getPolygonsAt(position, false);
	
	for (auto i: polyIndices) {
		if (polygons[i].isPointInside(position)) {
			return true;
		}
	}

	return false;
}

std::optional<Vector2f> Navmesh::getClosestPointTo(Vector2f pos, float anisotropy, float maxDist) const
{
	if (boundingCircle.getDistanceTo(pos) > maxDist) {
		return {};
	}

	if (containsPoint(pos)) {
		return pos;
	}

	float bestDist = maxDist;
	std::optional<Vector2f> bestPoint;

	for (const auto& poly: polygons) {
		// Coarse test vs circle first
		const auto distToCircle = poly.getBoundingCircle().getDistanceTo(pos);
		if (distToCircle <= bestDist) {
			const auto p = poly.getClosestPoint(pos, anisotropy);
			const float dist = (p - pos).length();
			if (dist < bestDist) {
				bestPoint = p;
				bestDist = dist;
			}
		}
	}

	return bestPoint;
}

NavmeshBounds::NavmeshBounds(Vector2f origin, Vector2f side0, Vector2f side1, size_t side0Divisions, size_t side1Divisions, Vector2f scaleFactor)
	: origin(origin)
	, side0(side0)
	, side1(side1)
	, side0Divisions(side0Divisions)
	, side1Divisions(side1Divisions)
	, scaleFactor(scaleFactor)
	, base(side0, side1)
{
	const auto u = side0.normalized();
	const auto v = side1.normalized();
	const auto p0 = origin;
	const auto p1 = origin + side0 + side1;
	edges = {
		Line(p0, u),
		Line(p0, v),
		Line(p1, u),
		Line(p1, v)
	};
}

bool NavmeshBounds::isPointOnEdge(Vector2f point, float threshold) const
{
	for (auto& e: edges) {
		if (e.getDistance(point) < threshold) {
			return true;
		}
	}
	return false;
}

bool NavmeshSubworldPortal::operator==(const NavmeshSubworldPortal& other) const
{
	return segment == other.segment && normal == other.normal && subworldDelta == other.subworldDelta;
}

bool NavmeshSubworldPortal::operator!=(const NavmeshSubworldPortal& other) const
{
	return segment != other.segment || normal != other.normal || subworldDelta != other.subworldDelta;
}

bool NavmeshSubworldPortal::isBeyondPortal(Vector2f p) const
{
	return (p - segment.a).dot(normal) > 0;
}

Navmesh::Node::Node(const ConfigNode& node)
{
	pos = Vector2f(node["pos"].asVector2i());
	
	const auto& connNodes = node["connNodes"].asSequence();
	const auto& connCosts = node["connCosts"].asSequence();
	if (connNodes.size() != connCosts.size()) {
		throw Exception("Invalid navigation data", 0);
	}

	nConnections = connNodes.size();
	assert(nConnections <= Navmesh::MaxConnections);
	for (size_t i = 0; i < connNodes.size(); ++i) {
		const int conn = connNodes[i].asInt();
		connections[i] = conn != -1 ? OptionalLite<NodeId>(gsl::narrow<NodeId>(conn)) : OptionalLite<NodeId>();
		costs[i] = connCosts[i].asFloat();
		if (std::isnan(costs[i])) {
			costs[i] = std::numeric_limits<float>::infinity();
		}
	}
}

ConfigNode Navmesh::Node::toConfigNode() const
{
	ConfigNode::MapType result;
	result["pos"] = Vector2i(pos);

	ConfigNode::SequenceType connNodes;
	ConfigNode::SequenceType connCosts;
	connNodes.reserve(nConnections);
	connCosts.reserve(nConnections);
	for (size_t i = 0; i < nConnections; ++i) {
		connNodes.push_back(ConfigNode(connections[i].has_value() ? static_cast<int>(connections[i].value()) : -1));
		connCosts.push_back(ConfigNode(costs[i]));
	}
	result["connNodes"] = std::move(connNodes);
	result["connCosts"] = std::move(connCosts);
	
	return result;
}

void Navmesh::Node::serialize(Serializer& s) const
{
	s << pos;
	s << static_cast<uint64_t>(nConnections);
	for (size_t i = 0; i < nConnections; ++i) {
		s << connections[i];
		s << costs[i];
	}
}

void Navmesh::Node::deserialize(Deserializer& s)
{
	uint64_t n;

	s >> pos;
	s >> n;
	nConnections = static_cast<size_t>(n);

	for (size_t i = 0; i < nConnections; ++i) {
		s >> connections[i];
		s >> costs[i];
	}
}

Navmesh::NodeAndConn::NodeAndConn(const ConfigNode& configNode)
	: node(static_cast<uint16_t>(configNode.asVector2i().x))
	, connectionIdx(static_cast<uint16_t>(configNode.asVector2i().y))
{
}

ConfigNode Navmesh::NodeAndConn::toConfigNode() const
{
	return ConfigNode(Vector2i(node, connectionIdx));
}

void Navmesh::NodeAndConn::serialize(Serializer& s) const
{
	s << node;
	s << connectionIdx;
}

void Navmesh::NodeAndConn::deserialize(Deserializer& s)
{
	s >> node;
	s >> connectionIdx;
}

std::optional<Vector<Navmesh::NodeAndConn>> Navmesh::pathfindNodes(const NavigationQuery& query) const
{
	if (query.from.subWorld != subWorld || query.to.subWorld != subWorld) {
		return {};
	}

	auto fromId = getNodeAt(query.from.pos);
	auto toId = getNodeAt(query.to.pos);

	if (!fromId || !toId) {
		return {};
	}

	return pathfind(fromId.value(), toId.value());
}

std::optional<NavigationPath> Navmesh::pathfind(const NavigationQuery& query) const
{
	auto nodePath = pathfindNodes(query);
	if (!nodePath) {
		return {};
	}

	return makePath(query, nodePath.value(), true);
}

Vector<Navmesh::NodeAndConn> Navmesh::makeResult(const Vector<State>& state, int startId, int endId) const
{
	Vector<NodeAndConn> result;
	for (NodeAndConn curNode(endId); true; curNode = state[curNode.node].cameFrom) {
		result.push_back(curNode);
		if (curNode.node == startId) {
			break;
		}
	}
	std::reverse(result.begin(), result.end());
	return result;
}

std::optional<Vector<Navmesh::NodeAndConn>> Navmesh::pathfind(int fromId, int toId) const
{
	// Ensure the query is valid
	if (fromId < 0 || fromId >= static_cast<int>(nodes.size()) || toId < 0 || toId >= static_cast<int>(nodes.size())) {
		// Invalid query
		return {};
	}

	// State map. Using vector for perf, trading space for CPU.
	// TODO: measure this vs unordered_map or some other hashtable, it's not impossible that it'd perform better (compact memory)
	Vector<State> state(nodes.size(), State{});

	// Open set
	auto openSet = PriorityQueue<NodeId, NodeComparator>(NodeComparator(state));
	openSet.reserve(std::min(static_cast<size_t>(100), nodes.size()));

	// Define heuristic function
	const Vector2f endPos = nodes[toId].pos;
	auto h = [&] (Vector2f pos) -> float
	{
		constexpr float epsilon = 1.5f;
		return (pos - endPos).length() * epsilon;
	};

	// Initialize the query
	{
		auto& firstNodeState = state[fromId];
		firstNodeState.cameFrom = NodeAndConn();
		firstNodeState.gScore = 0;
		firstNodeState.fScore = h(nodes[fromId].pos);
		firstNodeState.inOpenSet = true;
		openSet.push(fromId);
	}

	// Run A*
	while (!openSet.empty()) {
		const auto curId = openSet.top();
		if (curId == toId) {
			// Done!
			return makeResult(state, fromId, toId);
		}

		state[curId].inOpenSet = false;
		state[curId].inClosedSet = true;
		openSet.pop();
		
		const float gScore = state[curId].gScore;
		const auto& curNode = nodes[curId];
		for (size_t i = 0; i < curNode.nConnections; ++i) {
			if (curNode.connections[i]) {
				const auto nodeId = curNode.connections[i].value();
				if (!state[nodeId].inClosedSet) {
					auto& neighState = state[nodeId];
					const float neighScore = gScore + curNode.costs[i];

					if (neighScore < neighState.gScore) {
						neighState.cameFrom = NodeAndConn(curId, static_cast<uint16_t>(i));
						neighState.gScore = neighScore;
						neighState.fScore = neighScore + h(nodes[nodeId].pos);
						if (!neighState.inOpenSet) {
							neighState.inOpenSet = true;
							openSet.push(nodeId);
						} else {
							openSet.update(nodeId);
						}
					}
				}
			}
		}
	}
	
	return {};
}

std::optional<NavigationPath> Navmesh::makePath(const NavigationQuery& query, const Vector<NodeAndConn>& nodePath, bool postProcess) const
{
	Vector<Vector2f> points;
	points.reserve(nodePath.size() + 1);
	
	points.push_back(query.from.pos);
	for (size_t i = 1; i < nodePath.size(); ++i) {
		const auto cur = nodePath[i - 1];

		const auto& prevPoly = polygons[cur.node];
		const auto edge = prevPoly.getEdge(cur.connectionIdx);
		
		points.push_back(0.5f * (edge.a + edge.b));
	}
	points.push_back(query.to.pos);

	if (postProcess) {
		if (query.postProcessingType != NavigationQuery::PostProcessingType::None) {
			postProcessPath(points, query.postProcessingType);
		}
		if (query.quantizationType != NavigationQuery::QuantizationType::None) {
			quantizePath(points, query.quantizationType);
		}
	}

	// Check for NaN/inf
	for (auto& p: points) {
		if (!p.isValid()) {
			Logger::logError("Navmesh query " + toString(query) + " generated NaNs and/or infs. Aborting.");
			return NavigationPath(query, {});
		}
	}

	return NavigationPath(query, points);
}

/*
void Navmesh::postProcessPath(Vector<Vector2f>& points, NavigationQuery::PostProcessingType type) const
{
	if (type == NavigationQuery::PostProcessingType::None) {
		return;
	}
	if (points.size() <= 2) {
		return;
	}

	Vector<NodeId> nodeIds;
	nodeIds.resize(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		nodeIds[i] = getNodeAt(points[i]).value_or(static_cast<NodeId>(-1));
		assert(nodeIds[i] != static_cast<NodeId>(-1));
	}

	Vector<float> pathCosts;
	pathCosts.resize(points.size());
	pathCosts[0] = 0.0f;
	for(size_t i = 1; i < points.size(); i++) {
		
		const Vector2f delta = points.at(i) - points.at(i-1);
		const float len = delta.length();
		const auto ray = Ray(points.at(i-1), delta / len);

		const auto startPoint = nodeIds[i-1];
		const auto col = findRayCollision(ray, len, startPoint);
		pathCosts[i] = col.second;
	}
	
	int dst = static_cast<int>(points.size()) - 1;
	while (dst > 1) {
		const Vector2f dstPoint = points[dst];
		int lastSafeFrom = dst - 1;
		float lastSafeCost = -1.0f;
		for (int i = lastSafeFrom - 1; i >= 0; --i) {
			const Vector2f curPoint = points[i];

			const Vector2f delta = dstPoint - curPoint;
			const float len = delta.length();
			const auto ray = Ray(curPoint, delta / len);

			const auto startPoint = nodeIds[i];
			const auto [col, dist] = findRayCollision(ray, len, startPoint);
			if (col) {
				// No shortcut from here, abort
				break;
			} 

			float originalDist = 0.0f;
			for (int p = i + 1; p <= dst; p++) {
				originalDist += pathCosts.at(p);
			}

			if (originalDist <= dist) {
				break;
			}
			
			// This is safe
			lastSafeFrom = i;
			lastSafeCost = dist;
		}

		const int eraseCount = (dst - lastSafeFrom - 1);
		if (eraseCount > 0) {
			points.erase(points.begin() + (lastSafeFrom + 1), points.begin() + dst);
			nodeIds.erase(nodeIds.begin() + (lastSafeFrom + 1), nodeIds.begin() + dst);
			pathCosts[dst] = lastSafeCost;
			pathCosts.erase(pathCosts.begin() + (lastSafeFrom + 1), pathCosts.begin() + dst);			
		}

		if (eraseCount > 0 && type == NavigationQuery::PostProcessingType::Aggressive) {
			dst = static_cast<int>(points.size()) - 1;
		} else {
			dst = lastSafeFrom;
		}
	}
}
*/

void Navmesh::postProcessPath(Vector<Vector2f>& points, NavigationQuery::PostProcessingType type) const
{
	if (type == NavigationQuery::PostProcessingType::None || points.size() <= 2) {
		return;
	}

	Vector<NodeId> nodeIds;
	nodeIds.resize(points.size());
	for (size_t i = 0; i < points.size(); ++i) {
		nodeIds[i] = getNodeAt(points[i]).value_or(static_cast<NodeId>(-1));
		assert(nodeIds[i] != static_cast<NodeId>(-1));
	}

	for (size_t i = 1; i < points.size() - 1; ) {
		// This point can be removed if the previous point has direct line of sight to the next

		const auto p0 = points.at(i - 1);
		const auto p1 = points.at(i);
		const auto p2 = points.at(i + 1);
		const auto delta = p2 - p0;
		const auto origDist = delta.length();
		const auto dir = delta / origDist;

		const auto ray = Ray(p0, dir);
		const auto col = findRayCollision(ray, origDist, nodeIds[i - 1]);

		if (!col.first) {
			// No collision, so this is a candidate for removal
			// But first, make sure that the new path is not much more expensive than the previous

			const auto col0 = findRayCollision(Ray(p0, (p1 - p0).normalized()), (p1 - p0).length(), nodeIds[i - 1]);
			const auto col1 = findRayCollision(Ray(p1, (p2 - p1).normalized()), (p2 - p1).length(), nodeIds[i]);
			if (col.second < (col0.second + col1.second) * 1.2f) {
				points.erase(points.begin() + i);
				nodeIds.erase(nodeIds.begin() + i);

				// i is now the next item, so no need to increment
				continue;
			}
		}

		// Didn't remove a point, so increment i
		++i;
	}
}

void Navmesh::quantizePath(Vector<Vector2f>& points, NavigationQuery::QuantizationType type) const
{
	if (type == NavigationQuery::QuantizationType::Quantize8Way) {
		quantizePath8Way(points, Vector2f(1, 1));
	} else if (type == NavigationQuery::QuantizationType::Quantize8WayIsometric) {
		quantizePath8Way(points, Vector2f(1, 2));
	}
}

void Navmesh::quantizePath8Way(Vector<Vector2f>& points, Vector2f scale) const
{
	if (points.size() < 2) {
		return;
	}

	Vector<Vector2f> result;
	result.reserve(points.size());
	result.push_back(points[0]);

	for (size_t i = 1; i < points.size(); i++) {
		const auto a = points[i - 1];
		const auto b = points[i];

		// Construct a parallelogram around the path
		// One of these will be a diagonal, the other will be horizontal or vertical
		// d0 is the diagonal, d1 is the remaining (horizontal or vertical)
		const auto delta = (b - a) * scale;
		const auto d0 = std::abs(delta.x) > std::abs(delta.y) ? Vector2f(signOf(delta.x) * std::abs(delta.y), delta.y) : Vector2f(delta.x, signOf(delta.y) * std::abs(delta.x));
		const auto d1 = delta - d0;

		// If either vector is too small, then it's not worth doing it
		const float threshold = 1.0f;
		if (d0.length() >= threshold && d1.length() >= threshold) {
			// Two potential target points, c and d, are constructed
			const auto c = a + d0 / scale;
			const auto d = a + d1 / scale;

			// See if either path is acceptable
			if (isPathClear(a, c, b)) {
				result.push_back(c);
			} else if (isPathClear(a, d, b)) {
				result.push_back(d);
			}
		}

		result.push_back(b);
	}

	points = std::move(result);
}

bool Navmesh::isPathClear(Vector2f a, Vector2f b, Vector2f c) const
{
	return !findRayCollision(b, c) && !findRayCollision(a, b);
}

std::optional<Vector2f> Navmesh::findRayCollision(Vector2f from, Vector2f to) const
{
	const auto delta = to - from;
	const auto len = delta.length();
	return findRayCollision(Ray(from, delta / len), len);
}

std::optional<Vector2f> Navmesh::findRayCollision(Ray ray, float maxDistance) const
{
	if (const auto startNode = getNodeAt(ray.p)) {
		return findRayCollision(ray, maxDistance, startNode.value()).first;
	} else {
		return Vector2f(ray.p);
	}
}

std::pair<std::optional<Vector2f>, float> Navmesh::findRayCollision(Ray ray, float maxDistance, NodeId initialPolygon) const
{
	float distanceLeft = maxDistance;
	float weightedDistance = 0.0f;
	NodeId curPoly = initialPolygon;
	std::optional<NodeId> prevPoly;
	while (distanceLeft > 0) {
		const auto& poly = polygons.at(curPoly);
		std::optional<size_t> edgeIdx = poly.getExitEdge(ray);
		if (!edgeIdx) {
			// Something went wrong
			return { ray.p, weightedDistance };
		}

		const auto nextPoly = nodes[curPoly].connections[edgeIdx.value()];
		if (nextPoly && (nextPoly == prevPoly)) {
			//Logger::logError("Navmesh::findRayCollision error: ping-ponging on navmesh. Prev = " + toString(prevPoly) + ", cur = " + toString(curPoly) + ", next = " + toString(nextPoly));

			// Try again, from the next edge
			//edgeIdx = poly.getExitEdge(ray, edgeIdx.value() + 1);
			edgeIdx = poly.getExitEdge(ray, edgeIdx.value() + 1);
			if (!edgeIdx) {
				//Logger::logError("Could not recover from navmesh ping-pong, aborting.");
				return { ray.p, weightedDistance };
			}
		}

		// Find intersection with that edge
		const auto edge = poly.getEdge(edgeIdx.value());
		const Line edgeLine(edge.a, (edge.b - edge.a).normalized());
		std::optional<Vector2f> intersection = edgeLine.intersection(Line(ray.p, ray.dir));
		if (!intersection) {
			// Parallel - Must be overlapping due to exitEdge test previously

			// ray.p must be between edge.a and edge.b OR be at edge.a or edge.b
			// The corner intersected by the ray is b if edge goes in the same direction as the ray, a otherwise
			const bool useEdgeB = (edge.b - edge.a).dot(ray.dir) > 0;
			const auto endCorner = useEdgeB ? edge.b : edge.a;

			const float intersectDist = std::min(distanceLeft, (endCorner - ray.p).length());
			intersection = ray.p + ray.dir * intersectDist;

			if (intersectDist <= 0) {
				// We are already at end corner, pick the next edge round with the same corner
				if (useEdgeB) {
					edgeIdx = (edgeIdx.value() + 1) % poly.getNumSides();
				} else {
					edgeIdx = (edgeIdx.value() + poly.getNumSides() - 1) % poly.getNumSides();
				}
			}
		}

		// Check how much more we have left to go and stop if we reach the destination
		constexpr float epsilon = 0.1f;
		float distMoved = (ray.p - intersection.value()).length();

		if (distMoved < std::min(epsilon, distanceLeft)) {
			// If distMoved is zero, this can infinite loop (seen it in practice)
			distMoved = epsilon;
		}
		weightedDistance += std::min(distMoved, distanceLeft) * (weights.empty() ? 1.0f : weights.at(curPoly));
		distanceLeft -= distMoved;
		if (distanceLeft < epsilon) {
			return { {}, weightedDistance };
		}

		// Move to the next polygon on navmesh
		if (nextPoly) {
			prevPoly = curPoly;
			curPoly = nextPoly.value();
			ray = Ray(intersection.value(), ray.dir);
		} else {
			// Hit the edge of the navmesh
			return { intersection.value(), weightedDistance };
		}
	}

	// No collisions
	return { std::nullopt, weightedDistance };
}

void Navmesh::setWorldPosition(Vector2f newOffset, Vector2i gridPos)
{
	const auto delta = newOffset - offset;
	offset = newOffset;
	worldGridPos = gridPos;

	for (auto& node: nodes) {
		node.pos += delta;
	}
	for (auto& poly: polygons) {
		poly.translate(delta);
	}
	for (auto& portal: portals) {
		portal.translate(delta);
	}
	for (auto& edge: openEdges) {
		edge.second.a += delta;
		edge.second.b += delta;
	}
	origin += delta;
	computeBoundingCircle();
}

void Navmesh::markPortalConnected(size_t idx)
{
	portals[idx].connected = true;
}

void Navmesh::markPortalsDisconnected()
{
	for (auto& portal: portals) {
		portal.connected = false;
	}
}

void Navmesh::processPolygons()
{
	addPolygonsToGrid();
	computeArea();
	computeBoundingCircle();
}

void Navmesh::addPolygonsToGrid()
{
	polyGrid.resize(gridSize.x * gridSize.y);
	for (auto& c: polyGrid) {
		c.clear();
	}
	for (size_t i = 0; i < polygons.size(); ++i) {
		addPolygonToGrid(polygons[i], gsl::narrow<NodeId>(i));
	}
}

void Navmesh::addPolygonToGrid(const Polygon& poly, NodeId idx)
{
	int minX = gridSize.x - 1;
	int minY = gridSize.y - 1;
	int maxX = 0;
	int maxY = 0;

	for (const auto& v: poly.getVertices()) {
		const auto p = Vector2i(normalisedCoordinatesBase.inverseTransform(v - origin) * Vector2f(gridSize)).floor();
		minX = std::min(minX, std::max(0, p.x));
		maxX = std::max(maxX, std::min(gridSize.x - 1, p.x));
		minY = std::min(minY, std::max(0, p.y));
		maxY = std::max(maxY, std::min(gridSize.y - 1, p.y));
	}

	for (int y = minY; y <= maxY; ++y) {
		for (int x = minX; x <= maxX; ++x) {
			polyGrid[x + y * gridSize.x].push_back(idx);
		}
	}
}

std::optional<Vector2i> Navmesh::getGridAt(Vector2f pos, bool allowOutside) const
{
	const auto p = Vector2i((normalisedCoordinatesBase.inverseTransform(pos - origin) * Vector2f(gridSize)).floor());
	const int x = clamp(p.x, 0, gridSize.x - 1);
	const int y = clamp(p.y, 0, gridSize.y - 1);

	if (!allowOutside && (x != p.x || y != p.y)) {
		return {};
	}

	return Vector2i(x, y);
}

gsl::span<const Navmesh::NodeId> Navmesh::getPolygonsAt(Vector2f pos, bool allowOutside) const
{
	if (const auto p = getGridAt(pos, allowOutside)) {
		return getPolygonsAt(*p);
	} else {
		return {};
	}
}

gsl::span<const Navmesh::NodeId> Navmesh::getPolygonsAt(Vector2i gridPos) const
{
	return polyGrid[gridPos.x + gridPos.y * gridSize.x];
}

float Navmesh::getArea() const
{
	return totalArea;
}

Vector2f Navmesh::getRandomPoint(Random& rng) const
{
	if (polygons.empty()) {
		return Vector2f();
	}
	
	const float areaThreshold = rng.getFloat(0, totalArea);
	float accum = 0;
	for (const auto& p: polygons) {
		accum += p.getArea();
		if (accum > areaThreshold) {
			return p.getCentre();
		}
	}
	return polygons[0].getCentre();
}

void Navmesh::computeArea()
{
	totalArea = 0;
	for (const auto& p: polygons) {
		totalArea += p.getArea();
	}
}

void Navmesh::computeBoundingCircle()
{
	auto centre = origin + normalisedCoordinatesBase.transform(Vector2f(0.5f, 0.5f));
	auto left = origin + normalisedCoordinatesBase.transform(Vector2f(1.0f, 0.0f));
	auto radius = std::max((origin - left).length(), (origin - centre).length());
	boundingCircle = Circle(centre, radius);

	/*
	for (const auto& poly: polygons) {
		for (const auto& p: poly.getVertices()) {
			if (!boundingCircle.contains(p)) {
				assert(false);
			}
		}
	}
	*/
}

void Navmesh::generateOpenEdges()
{
	openEdges.clear();
	for (size_t nodeId = 0; nodeId < nodes.size(); ++nodeId) {
		const auto& node = nodes[nodeId];
		for (size_t i = 0; i < node.nConnections; ++i) {
			if (!node.connections[i]) {
				openEdges.emplace_back(static_cast<uint16_t>(nodeId), polygons[nodeId].getEdge(i));
			}
		}
	}
}

void Navmesh::addToPortals(NodeAndConn nodeAndConn, int id)
{
	getPortals(id).connections.push_back(nodeAndConn);
}

Navmesh::Portal& Navmesh::getPortals(int id)
{
	for (auto& edge: portals) {
		if (edge.id == id) {
			return edge;
		}
	}
	return portals.emplace_back(Portal(id));
}

void Navmesh::postProcessPortals()
{
	auto edges = std::move(portals);

	for (auto& edge: edges) {
		edge.postProcess(polygons, portals);
	}
}

Navmesh::Portal::Portal(int id)
	: id(id)
{
	updateLocal();
}

Navmesh::Portal::Portal(const ConfigNode& node)
{
	id = node["id"].asInt();
	pos = node["pos"].asVector2f();
	vertices = node["vertices"].asVector<Vector2f>();
	connections = node["connections"].asVector<NodeAndConn>();
	updateLocal();
}

ConfigNode Navmesh::Portal::toConfigNode() const
{
	ConfigNode::MapType result;

	result["id"] = id;
	result["pos"] = pos;
	result["vertices"] = vertices;
	result["connections"] = connections;
	
	return result;
}

void Navmesh::Portal::serialize(Serializer& s) const
{
	s << id;
	s << pos;
	s << vertices;
	s << connections;
}

void Navmesh::Portal::deserialize(Deserializer& s)
{
	s >> id;
	s >> pos;
	s >> vertices;
	s >> connections;
	updateLocal();
}

void Navmesh::Portal::postProcess(gsl::span<const Polygon> polygons, Vector<Portal>& dst)
{
	const float epsilon = 0.01f;
	Vector<std::deque<Vector2f>> chains;
	for (auto& conn: connections) {
		const auto edge = polygons[conn.node].getEdge(conn.connectionIdx);

		// Go through all the chains, and merge with it, if possible
		std::optional<size_t> mergedLeft;
		std::optional<size_t> mergedRight;
		for (size_t i = 0; i < chains.size(); ++i) {
			auto& chain = chains[i];
			if (!mergedLeft && edge.a.epsilonEquals(chain.back(), epsilon)) {
				chain.push_back(edge.b);
				mergedLeft = i;
			}
			if (!mergedRight && edge.b.epsilonEquals(chain.front(), epsilon)) {
				chain.push_front(edge.a);
				mergedRight = i;
			}
		}

		// Found no chains
		if (!mergedLeft && !mergedRight) {
			chains.push_back(std::deque<Vector2f>{ edge.a, edge.b });
		}

		// Found two chains, merge them
		if (mergedLeft && mergedRight) {
			auto& left = chains[mergedLeft.value()];
			auto& right = chains[mergedRight.value()];
			left.pop_back();
			left.pop_back();
			left.insert(left.end(), right.begin(), right.end());
			chains.erase(chains.begin() + mergedRight.value());
		}
	}

	// Output edges
	for (auto& chain: chains) {
		auto& result = dst.emplace_back(id);
		auto& vs = result.vertices;
		vs.insert(vs.end(), chain.begin(), chain.end());

		// Find the centre of edge
		// First find the length of the edge, then pick the point at half of that length
		result.pos = vs[0]; // Fallback
		float midLen = 0;
		for (size_t i = 1; i < vs.size(); ++i) {
			midLen += (vs[i] - vs[i - 1]).length();
		}
		midLen *= 0.5f;
		float curLen = 0;
		for (size_t i = 1; i < vs.size(); ++i) {
			const float segmentLen = (vs[i] - vs[i - 1]).length();
			if (curLen + segmentLen > midLen) {
				const float t = (midLen - curLen) / segmentLen;
				result.pos = lerp(vs[i - 1], vs[i], t);
				break;
			} else {
				curLen += segmentLen;
			}
		}
		
		// TODO: output connections too
		
	}
}

bool Navmesh::Portal::canJoinWith(const Portal& other, float epsilon) const
{
	const auto a0 = vertices.front();
	const auto a1 = vertices.back();
	const auto b0 = other.vertices.front();
	const auto b1 = other.vertices.back();

	if (subWorldLink && other.subWorldLink) {
		// Subworld links are trickier because the edges might not align exactly
		// Instead verify:
		// a) they're collinear
		// b) they overlap on their longitudinal axis
		// c) they're close to each other on their transversal axis

		const auto n0 = (a1 - a0).normalized();
		const auto n1 = (b1 - b0).normalized();
		const auto n2 = (b0 - a0).squaredLength() > (b1 - a0).squaredLength() ? (b0 - a0).normalized() : (b1 - a0).normalized() ;
		if (std::abs(n0.dot(n1)) < 0.99f) {
			// Not pointing the same direction
			return false;
		}

		if (std::abs(n0.dot(n2)) < 0.99f) {
			// Not on the same line
			return false;
		}

		const auto p0 = LineSegment(a0, a1).project(n0);
		const auto p1 = LineSegment(b0, b1).project(n0);
		if (!p0.overlaps(p1)) {
			// Not overlapping
			return false;
		}
		
		return true;
	} else {
		// TODO: can use edge ids to validate this connection, if the algorithm ever makes connections it shouldn't
		if (vertices.empty()) {
			return false;
		}
		
		// Link between different scenes, 0 must pair with 2 and 1 with 3.
		if (!regionLink && std::abs(id - other.id) != 2) {
			return false;
		}
		
		return (a0.epsilonEquals(b1, epsilon) && a1.epsilonEquals(b0, epsilon))
			|| (a0.epsilonEquals(b0, epsilon) && a1.epsilonEquals(b1, epsilon));

		/*
		if (vertices.size() != other.vertices.size()) {
			return false;
		}


		const size_t n = vertices.size();
		for (size_t i = 0; i < n; ++i) {
			if (!vertices[i].epsilonEquals(other.vertices[n - i - 1], epsilon)) {
				return false;
			}
		}
		return true;
		*/
	}
}

void Navmesh::Portal::updateLocal()
{
	// 0, 1, 2, 3 = map edges
	// 4-5 = subworld
	// 6+ = other regions
	subWorldLink = id >= 4 && id <= 5;
	regionLink = id >= 6;
}

void Navmesh::Portal::translate(Vector2f offset)
{
	pos += offset;
	for (auto& v: vertices) {
		v += offset;
	}
}

Vector2f Navmesh::Portal::getClosestPoint(Vector2f pos) const
{
	// NB: this assumes a linear portal
	const auto segment = LineSegment(vertices.front(), vertices.back());
	return segment.getClosestPoint(pos);
}
