#pragma once

#include "navigation_path.h"
#include "navigation_query.h"
#include "halley/maths/polygon.h"
#include "halley/maths/base_transform.h"

namespace Halley {
	class Random;

	struct NavmeshBounds {
		Vector2f origin;
		Vector2f side0;
		Vector2f side1;
		size_t side0Divisions;
		size_t side1Divisions;
		Vector2f scaleFactor;
		Base2D base;
		std::array<Line, 4> edges;

		NavmeshBounds(Vector2f origin, Vector2f side0, Vector2f side1, size_t side0Divisions, size_t side1Divisions, Vector2f scaleFactor);
		bool isPointOnEdge(Vector2f point, float threshold) const;
	};

	struct NavmeshSubworldPortal {
		LineSegment segment;
		Vector2f normal;
		int subworldDelta = 0;

		bool operator==(const NavmeshSubworldPortal& other) const;
		bool operator!=(const NavmeshSubworldPortal& other) const;
		bool isBeyondPortal(Vector2f p) const;
	};

	class Navmesh {
	public:
		constexpr static size_t MaxConnections = 8;
		using NodeId = uint16_t;
		
		class PolygonData {
		public:
			Polygon polygon;
			Vector<int> connections;
			float weight;
		};

		class alignas(64) Node {
		public:
			Vector2f pos;
			size_t nConnections = 0;
			std::array<OptionalLite<NodeId>, MaxConnections> connections;
			std::array<float, MaxConnections> costs;

			Node() = default;
			Node(const ConfigNode& node);

			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};
		
		struct NodeAndConn {
			NodeId node;
			uint16_t connectionIdx;
			
			NodeAndConn(NodeId node = -1, uint16_t connection = std::numeric_limits<uint16_t>::max()) : node(node), connectionIdx(connection) {}
			NodeAndConn(const ConfigNode& node);

			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);
		};

		struct Portal {
			int id;
			Vector2f pos;
			Vector<NodeAndConn> connections;
			Vector<Vector2f> vertices;
			bool connected = false;
			bool regionLink = false;
			bool subWorldLink = false;

			Portal(int id = 0);
			explicit Portal(const ConfigNode& node);

			ConfigNode toConfigNode() const;

			void serialize(Serializer& s) const;
			void deserialize(Deserializer& s);

			void postProcess(gsl::span<const Polygon> polygons, Vector<Portal>& dst);
			
			bool canJoinWith(const Portal& other, float epsilon) const;
			void updateLocal();
			void translate(Vector2f offset);

			Vector2f getClosestPoint(Vector2f pos) const;
		};
		
		Navmesh();
		Navmesh(const ConfigNode& nodeData);
		Navmesh(Vector<PolygonData> polygons, const NavmeshBounds& bounds, int subWorld);

		[[nodiscard]] ConfigNode toConfigNode() const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		[[nodiscard]] std::optional<Vector<NodeAndConn>> pathfindNodes(const NavigationQuery& query) const;
		[[nodiscard]] std::optional<NavigationPath> makePath(const NavigationQuery& query, const Vector<NodeAndConn>& nodePath, bool postProcess) const;
		[[nodiscard]] std::optional<NavigationPath> pathfind(const NavigationQuery& query) const;
		void postProcessPath(Vector<Vector2f>& points, NavigationQuery::PostProcessingType type) const;
		void quantizePath(Vector<Vector2f>& points, NavigationQuery::QuantizationType type) const;

		[[nodiscard]] const Vector<Node>& getNodes() const { return nodes; }
		[[nodiscard]] const Vector<Polygon>& getPolygons() const { return polygons; }
		[[nodiscard]] const Vector<Portal>& getPortals() const { return portals; }
		[[nodiscard]] const Vector<float>& getWeights() const { return weights; }
		[[nodiscard]] const Vector<std::pair<uint16_t, LineSegment>>& getOpenEdges() const { return openEdges; }
		[[nodiscard]] const Polygon& getPolygon(int id) const;
		[[nodiscard]] size_t getNumNodes() const { return nodes.size(); }
		[[nodiscard]] std::optional<NodeId> getNodeAt(Vector2f position) const;
		[[nodiscard]] bool containsPoint(Vector2f position) const;
		[[nodiscard]] std::optional<Vector2f> getClosestPointTo(Vector2f pos, float anisotropy = 1.0f) const;
		
		// Returns empty if no collision is found (i.e. fully contained within navmesh)
		// Otherwise returns collision point
		[[nodiscard]] std::optional<Vector2f> findRayCollision(Vector2f from, Vector2f to) const;
		[[nodiscard]] std::optional<Vector2f> findRayCollision(Ray ray, float maxDistance) const;
		[[nodiscard]] std::pair<std::optional<Vector2f>, float> findRayCollision(Ray ray, float maxDistance, NodeId initialPolygon) const;

		void setWorldPosition(Vector2f offset, Vector2i worldGridPos);
		[[nodiscard]] Vector2i getWorldGridPos() const { return worldGridPos; }
		[[nodiscard]] int getSubWorld() const { return subWorld; }
		[[nodiscard]] Vector2f getOffset() const { return offset; }

		void markPortalConnected(size_t idx);
		void markPortalsDisconnected();

		float getArea() const;
		Vector2f getRandomPoint(Random& rng) const;

		Base2D getNormalisedCoordinatesBase() const { return normalisedCoordinatesBase; }

	private:
		struct State {
			float gScore = std::numeric_limits<float>::infinity();
			float fScore = std::numeric_limits<float>::infinity();
			NodeAndConn cameFrom;
			bool inOpenSet = false;
			bool inClosedSet = false;
		};

		class NodeComparator {
		public:
			NodeComparator(const Vector<State>& state) : state(state) {}
			
			bool operator()(Navmesh::NodeId a, Navmesh::NodeId b) const
			{
				return state[a].fScore > state[b].fScore;
			}

		private:
			const Vector<State>& state;
		};

		Vector<Node> nodes;
		Vector<Polygon> polygons;
		Vector<Portal> portals;
		Vector<float> weights;
		Vector<std::pair<uint16_t, LineSegment>> openEdges;
		int subWorld = 0;

		Vector2i gridSize = Vector2i(20, 20);
		Vector<Vector<NodeId>> polyGrid; // Quick lookup of polygons

		Vector2f origin;
		Base2D normalisedCoordinatesBase;
		Vector2f scaleFactor; // Use for non-uniform movement, e.g. in isometric when moving left/right is cheaper than up/down (in that case, scaleFactor might be something like (1.0f, 2.0f))

		Vector2f offset;
		Vector2i worldGridPos;

		float totalArea = 0;

		std::optional<Vector<NodeAndConn>> pathfind(int fromId, int toId) const;
		Vector<NodeAndConn> makeResult(const Vector<State>& state, int startId, int endId) const;
		void quantizePath8Way(Vector<Vector2f>& points, Vector2f scale) const;

		void processPolygons();
		void addPolygonsToGrid();
		void addPolygonToGrid(const Polygon& poly, NodeId idx);
		gsl::span<const NodeId> getPolygonsAt(Vector2f pos, bool allowOutside) const;

		void addToPortals(NodeAndConn nodeAndConn, int id);
		Portal& getPortals(int id);
		void postProcessPortals();

		void computeArea();

		void generateOpenEdges();

		bool isPathClear(Vector2f a, Vector2f b, Vector2f c) const;
	};
}
