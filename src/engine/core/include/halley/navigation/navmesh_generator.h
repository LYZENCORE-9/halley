#pragma once

#include "navmesh.h"
#include "navmesh_set.h"

namespace Halley {
	class NavmeshGenerator {
	public:
		struct Params {
			NavmeshBounds bounds;
			gsl::span<const Polygon> obstacles;
			gsl::span<const Polygon> regions;
			gsl::span<const NavmeshSubworldPortal> subworldPortals;
			gsl::span<const Vector2f> poison; // These points destroy whatever navmesh they touch
			int subWorld = 0;
			float agentSize = 1.0f;
			std::function<float(int, const Polygon&)> getPolygonWeightCallback;
			String sceneName;
		};

		static NavmeshSet generate(const Params& params);

	private:
		enum class NavmeshNodePortalSide {
			Unknown,
			Before,
			Beyond
		};
		
		class NavmeshNode {
		public:
			Polygon polygon;
			Vector<int> connections;
			int region = -1;
			int regionGroup = 0;
			int remap = 0;
			bool alive = true;
			bool tagged = false;
			NavmeshNodePortalSide beyondPortal = NavmeshNodePortalSide::Unknown;

			NavmeshNode() = default;
			NavmeshNode(Polygon p)
				: polygon(std::move(p))
				, connections(polygon.getNumSides(), -1)
			{}
			NavmeshNode(Polygon polygon, Vector<int> connections)
				: polygon(std::move(polygon))
				, connections(std::move(connections))
			{}
		};
		
		constexpr static size_t maxPolygonSides = 8;

		static Vector<Polygon> generateByPolygonSubtraction(gsl::span<const Polygon> inputPolygons, gsl::span<const Polygon> obstacles, Circle bounds);
		static Vector<Polygon> preProcessObstacles(gsl::span<const Polygon> obstacles, float agentSize);
		static Polygon makeAgentMask(float agentSize);

		static Polygon makeCell(Vector2i coord, Vector2f origin, Vector2f u, Vector2f v);
		static void insertPolygons(Vector<NavmeshNode>& src, Vector<NavmeshNode>& dst);

		static Vector<NavmeshNode> toNavmeshNode(Vector<Polygon> polygons);
		static void generateConnectivity(gsl::span<NavmeshNode> polygons);
		static void postProcessPolygons(Vector<NavmeshNode>& polygons, float maxSize, bool allowSimplification, const NavmeshBounds& bounds, const String& sceneName);
		static void removeDeadPolygons(Vector<NavmeshNode>& polygons);
		static void tagEdgeConnections(gsl::span<NavmeshNode> nodes, gsl::span<const Line> mapEdges);

		static std::optional<NavmeshNode> merge(const NavmeshNode& a, const NavmeshNode& b, size_t aEdgeIdx, size_t bEdgeIdx, size_t aIdx, size_t bIdx, float maxSize, bool allowSimplification, const NavmeshBounds& bounds);
		static void remapConnections(NavmeshNode& poly, int from, int to);

		static void simplifyPolygon(NavmeshNode& node, float threshold, const NavmeshBounds& bounds);
		static void simplifyPolygons(Vector<NavmeshNode>& nodes, const NavmeshBounds& bounds);

		static void limitPolygonSides(Vector<Polygon>& polygons, size_t maxSides);

		static void splitByPortals(Vector<NavmeshNode>& nodes, gsl::span<const NavmeshSubworldPortal> portals);
		static void splitByRegions(Vector<NavmeshNode>& nodes, gsl::span<const Polygon> regions);
		static size_t split(Vector<NavmeshNode>& nodes, size_t idx, LineSegment splitBy, std::function<void(NavmeshNode&)> postProcess);

		static void removeNodesBeyondPortals(Vector<NavmeshNode>& nodes);
		static void removePoisonedNodes(Vector<NavmeshNode>& nodes, gsl::span<const Vector2f> poisons);

		static void applyRegions(gsl::span<NavmeshNode> nodes, gsl::span<const Polygon> regions);
		static int assignRegions(gsl::span<NavmeshNode> nodes);
		static void floodFillRegion(gsl::span<NavmeshNode> nodes, NavmeshNode& firstNode, int regionGroup, int region);

		static std::optional<size_t> getNavmeshEdge(NavmeshNode& node, size_t side, gsl::span<const Line> mapEdges, gsl::span<const NavmeshSubworldPortal> subworldPortals);

		static Navmesh makeNavmesh(gsl::span<NavmeshNode> nodes, const NavmeshBounds& bounds, gsl::span<const NavmeshSubworldPortal> subworldPortals, int region, int subWorld, std::function<float(int, const Polygon&)> getPolygonWeightCallback, const String& sceneName);
	};
}
