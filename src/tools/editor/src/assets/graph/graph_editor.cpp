#include "graph_editor.h"


#include "ui_graph_node.h"
#include "src/ui/scroll_background.h"
using namespace Halley;

GraphEditor::GraphEditor(UIFactory& factory, Resources& gameResources, Project& project, AssetType type)
	: AssetEditor(factory, gameResources, project, type)
	, connectionsStyle(factory.getStyle("graphConnections"))
{
	factory.loadUI(*this, "halley/graph_editor");
}

void GraphEditor::onResourceLoaded()
{
	infiniCanvas->clear();
	infiniCanvas->add(std::make_shared<GraphConnections>(*this));
}

void GraphEditor::onMakeUI()
{
	infiniCanvas = getWidgetAs<InfiniCanvas>("infiniCanvas");
	infiniCanvas->setZoomEnabled(false);
}

std::shared_ptr<UIGraphNode> GraphEditor::getNode(std::string_view id)
{
	return getWidgetAs<UIGraphNode>(id);
}

void GraphEditor::drawConnections(UIPainter& painter)
{
}

Colour4f GraphEditor::getColourForPinType(RenderGraphPinType pinType) const
{
	switch (pinType) {
	case RenderGraphPinType::ColourBuffer:
		return connectionsStyle.getColour("colourBuffer");
	case RenderGraphPinType::DepthStencilBuffer:
		return connectionsStyle.getColour("depthStencilBuffer");
	case RenderGraphPinType::Texture:
		return connectionsStyle.getColour("texture");
	case RenderGraphPinType::Dependency:
		return connectionsStyle.getColour("dependency");
	default:
		return Colour4f(1, 1, 1);
	}
}

void GraphEditor::addNode(std::shared_ptr<UIGraphNode> node)
{
	infiniCanvas->add(node, 0, Vector4f(node->getPosition()), UISizerAlignFlags::Top | UISizerAlignFlags::Left);
}

GraphConnections::GraphConnections(GraphEditor& editor)
	: UIWidget()
	, editor(editor)
{}

void GraphConnections::draw(UIPainter& painter) const
{
	editor.drawConnections(painter);
}

