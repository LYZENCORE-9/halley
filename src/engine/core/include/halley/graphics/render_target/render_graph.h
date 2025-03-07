#pragma once
#include <memory>

#include "halley/data_structures/vector.h"

#include "render_graph_pin_type.h"
#include "halley/graphics/texture_descriptor.h"
#include "halley/graphics/sprite/sprite.h"

namespace Halley {
	class RenderGraphDefinition;
	class Camera;
	class TextureRenderTarget;
	class Texture;
	class RenderTarget;
	class VideoAPI;
	class RenderGraph;
	class RenderContext;
	class Painter;
	class Material;
	class RenderGraphNode;

	class RenderGraph {
	public:
		using PaintMethod = std::function<void(Painter&)>;
		using DrawCallback = std::function<void(SpriteMaskBase, Painter&)>;

		RenderGraph();
		explicit RenderGraph(std::shared_ptr<const RenderGraphDefinition> graphDefinition);

		void update();
		void render(const RenderContext& rc, VideoAPI& video, std::optional<Vector2i> renderSize = {});

		void clearCameras();
		const Camera* tryGetCamera(std::string_view id) const;
		void setCamera(std::string_view id, const Camera& camera);

		void setDrawCallback(DrawCallback callback);
		void draw(SpriteMaskBase mask, Painter& painter) const;

		const PaintMethod* tryGetPaintMethod(std::string_view id) const;
		void setPaintMethod(std::string_view id, PaintMethod method);

		void applyVariable(Material& material, const String& name, const ConfigNode& value) const;
		void setVariable(std::string_view name, float value);
		void setVariable(std::string_view name, Vector2f value);
		void setVariable(std::string_view name, Vector3f value);
		void setVariable(std::string_view name, Vector4f value);
		void setVariable(std::string_view name, Colour4f value);

		void setImageOutputCallback(std::string_view name, std::function<void(Image&)> callback);
		void clearImageOutputCallbacks();
		Image* getImageOutputForNode(const String& nodeId, Vector2i imageSize) const;
		void notifyImage(const String& nodeId) const;

		std::shared_ptr<Texture> getOutputTexture(const String& id);
		void setRenderSize(const String& id, const Vector2i& size);

		void setRemapOutputNode(std::string_view toNodeName);
		bool remapNode(uint8_t toNodeInputPin, std::string_view fromNodeName, uint8_t toNodeOutputPin);
		void resetRemapNode();

		void resetGraph();

	private:
		enum class VariableType {
			None,
			Float,
			Float2,
			Float3,
			Float4
		};
		
		struct Variable {
			Vector4f var;
			VariableType type = VariableType::None;

			void apply(Material& material, const String& name) const;
			Variable& operator=(float v);
			Variable& operator=(Vector2f v);
			Variable& operator=(Vector3f v);
			Variable& operator=(Vector4f v);
			Variable& operator=(Colour4f v);
		};

		struct ImageOutputCallback {
			mutable std::unique_ptr<Image> image;
			std::function<void(Image&)> calllback;
		};
		
		Vector<std::unique_ptr<RenderGraphNode>> nodes;
		std::map<GraphNodeId, RenderGraphNode*> nodeMap;
		
		std::map<String, Camera> cameras;
		std::map<String, PaintMethod> paintMethods;
		std::map<String, Variable> variables;
		std::map<String, ImageOutputCallback> imageOutputCallbacks;
		DrawCallback drawCallback;

		String remapOutputNode;
		Vector<std::pair<uint8_t, Vector<std::pair<String, uint8_t>>>> defaultOutputMapping;

		std::shared_ptr<const RenderGraphDefinition> graphDefinition;
		int lastDefinitionVersion = 0;

		void addNode(GraphNodeId id, std::unique_ptr<RenderGraphNode> node);
		RenderGraphNode* getNode(GraphNodeId id);
		RenderGraphNode* tryGetNode(const String& id);

		void loadDefinition(std::shared_ptr<const RenderGraphDefinition> definition);
	};


	class RenderGraphCommand
	{
	public:
		virtual void apply(RenderGraph& renderGraph) const = 0;
		virtual ~RenderGraphCommand() = default;
	};

	class RenderGraphChangeSizeCommand : public RenderGraphCommand
	{
	public:
		RenderGraphChangeSizeCommand(String id, Vector2i newSize)
			: id(std::move(id))
			, newSize(newSize)
		{  }

		void apply(RenderGraph& renderGraph) const override
		{
			renderGraph.setRenderSize(id, newSize);
		}

	private:
		const String id{};
		const Vector2i newSize{};
	};
}
