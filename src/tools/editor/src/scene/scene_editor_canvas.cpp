#include "scene_editor_canvas.h"


#include "scene_editor_game_bridge.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/render_target/render_surface.h"
#include "halley/input/input_keyboard.h"
#include "halley/input/input_keys.h"
#include "scene_editor_gizmo_collection.h"
#include "halley/game/scene_editor_interface.h"
#include "scene_editor_window.h"
#include "halley/tools/project/project.h"
#include "src/project/core_api_wrapper.h"

using namespace Halley;

SceneEditorCanvas::SceneEditorCanvas(String id, UIFactory& factory, Resources& resources, const HalleyAPI& api, Project& project, std::optional<UISizer> sizer)
	: UIWidget(std::move(id), Vector2f(32, 32), std::move(sizer))
	, factory(factory)
	, resources(resources)
	, project(project)
{
	border.setImage(resources, "whitebox.png").setColour(Colour4f());
	surface = std::make_shared<RenderSurface>(*api.video, resources);

	keyboard = api.input->getKeyboard();
	mouse = api.input->getMouse();

	setHandle(UIEventType::MouseWheel, [this](const UIEvent& event)
	{
		onMouseWheel(event);
	});

	project.withDLL([&] (ProjectDLL& dll)
	{
		dll.addReloadListener(*this);
	});
}

SceneEditorCanvas::~SceneEditorCanvas()
{
	project.withDLL([&] (ProjectDLL& dll)
	{
		dll.removeReloadListener(*this);
	});
}

void SceneEditorCanvas::onProjectDLLStatusChange(ProjectDLL::Status status)
{
	if (status == ProjectDLL::Status::Unloaded) {
		ready = false;
	}
}

void SceneEditorCanvas::onActiveChanged(bool active)
{
	if (!active) {
		ready = false;
	}
}

void SceneEditorCanvas::update(Time t, bool moved)
{
	updateInputState();
	
	if (frameN > 0 && gameBridge) {
		outputState.clear();
		ready = gameBridge->update(t, inputState, outputState) || ready;
	}

	if (inputState.rightClickPressed && !outputState.blockRightClick) {
		openRightClickMenu();
	}

	notifyOutputState();
	clearInputState();

	surface->setSize(getCanvasSize());
	++frameN;
}

void SceneEditorCanvas::draw(UIPainter& p) const
{
	p.draw([=, holdThis = shared_from_this()] (Painter& painter) {
		const auto pos = getPosition();
		const auto size = getSize();

		Sprite canvas;

		if (surface->isReady()) {
			canvas = surface->getSurfaceSprite();
		} else {
			canvas.setImage(resources, "whitebox.png").setColour(Colour4f(0.2f, 0.2f, 0.2f));
		}

		canvas.setPos(getPosition() + Vector2f(1, 1)).setSize(Vector2f(getCanvasSize()));

		canvas.draw(painter);

		// Draw border
		auto b = border.clone().setColour(borderColour);
		const float w = static_cast<float>(borderWidth);
		b.setPos(pos).setSize(Vector2f(size.x, w)).draw(painter);
		b.setPos(pos + Vector2f(0, size.y - w)).setSize(Vector2f(size.x, w)).draw(painter);
		b.setPos(pos).setSize(Vector2f(w, size.y)).draw(painter);
		b.setPos(pos + Vector2f(size.x - w, 0)).setSize(Vector2f(w, size.y)).draw(painter);
	});
}

bool SceneEditorCanvas::hasRender() const
{
	return true;
}

void SceneEditorCanvas::render(RenderContext& rc) const
{
	if (gameBridge && surface->isReady() && ready) {
		auto context = rc.with(surface->getRenderTarget());
		gameBridge->render(context);
	}
}

bool SceneEditorCanvas::canInteractWithMouse() const
{
	return true;
}

bool SceneEditorCanvas::isFocusLocked() const
{
	return inputState.leftClickHeld || inputState.middleClickHeld || inputState.rightClickHeld;
}

bool SceneEditorCanvas::canReceiveFocus() const
{
	return true;
}

void SceneEditorCanvas::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	switch (button) {
	case 0:
		inputState.leftClickPressed = inputState.leftClickHeld = true;
		break;
	case 1:
		inputState.middleClickPressed = inputState.middleClickHeld = true;
		break;
	case 2:
		inputState.rightClickPressed = inputState.rightClickHeld = true;
		break;
	}

	if (!dragging) {
		if (button == 1 || (tool == "drag" && button == 0) || (inputState.spaceHeld && button == 0)) {
			dragButton = button;
			dragging = true;
			lastMousePos = mousePos;
		}
	}
}

void SceneEditorCanvas::releaseMouse(Vector2f mousePos, int button)
{
	switch (button) {
	case 0:
		inputState.leftClickReleased = true;
		inputState.leftClickHeld = false;
		break;
	case 1:
		inputState.middleClickReleased = true;
		inputState.middleClickHeld = false;
		break;
	case 2:
		inputState.rightClickReleased = true;
		inputState.rightClickHeld = false;
		break;
	}

	if (button == dragButton && dragging) {
		onMouseOver(mousePos);
		dragging = false;
	}
}

void SceneEditorCanvas::onMouseOver(Vector2f mousePos)
{
	inputState.rawMousePos = mousePos;

	if (dragging && gameBridge) {
		gameBridge->dragCamera(lastMousePos - mousePos);
	}
	lastMousePos = mousePos;
}

void SceneEditorCanvas::onMouseWheel(const UIEvent& event)
{
	if (inputState.altHeld) {
		gameBridge->cycleHighlight(event.getIntData());
	} else {
		gameBridge->changeZoom(event.getIntData(), lastMousePos - getPosition() - getSize() / 2);
	}
}

void SceneEditorCanvas::setGameBridge(SceneEditorGameBridge& bridge)
{
	gameBridge = &bridge;
}

void SceneEditorCanvas::setSceneEditorWindow(SceneEditorWindow& window)
{
	editorWindow = &window;
}

std::shared_ptr<UIWidget> SceneEditorCanvas::setTool(const String& tool, const String& componentName, const String& fieldName)
{
	this->tool = tool;
	return gameBridge->getGizmos().setTool(tool, componentName, fieldName);
}

void SceneEditorCanvas::setBorder(int width, Colour4f colour)
{
	borderWidth = width;
	borderColour = colour;
}

void SceneEditorCanvas::updateInputState()
{
	inputState.ctrlHeld = keyboard->isButtonDown(KeyCode::LCtrl) || keyboard->isButtonDown(KeyCode::RCtrl);
	inputState.shiftHeld = keyboard->isButtonDown(KeyCode::LShift) || keyboard->isButtonDown(KeyCode::RShift);
	inputState.altHeld = keyboard->isButtonDown(KeyCode::LAlt) || keyboard->isButtonDown(KeyCode::RAlt);
	inputState.spaceHeld = keyboard->isButtonDown(KeyCode::Space);

	if (!isMouseOver() && !isFocusLocked()) {
		inputState.rawMousePos.reset();
	}

	inputState.viewRect = getRect();
}

void SceneEditorCanvas::notifyOutputState()
{
	auto& fields = outputState.fieldsChanged;
	if (!fields.empty()) {
		const auto n = fields.size();
		Vector<String> ids;
		Vector<const EntityData*> oldDatas;
		Vector<EntityData*> newDatas;
		ids.reserve(n);
		oldDatas.reserve(n);
		newDatas.reserve(n);
		for (size_t i = 0; i < n; ++i) {
			auto& f = fields[i];
			ids.push_back(f.entityId.toString());
			oldDatas.push_back(&f.oldData);
			newDatas.push_back(f.newData);
		}
		
		editorWindow->onEntitiesModified(ids, oldDatas, newDatas);
		editorWindow->onFieldChangedByGizmo(fields[0].componentName, fields[0].fieldName);
		
		fields.clear();
	}

	if (outputState.newSelection) {
		Vector<String> ids;
		ids.reserve(outputState.newSelection->size());
		for (auto& uuid: outputState.newSelection.value()) {
			ids.push_back(uuid.toString());
		}
		editorWindow->selectEntities(ids, outputState.selectionMode);
	}
}

void SceneEditorCanvas::clearInputState()
{
	inputState.clear();
}

void SceneEditorInputState::clear()
{
	leftClickPressed = false;
	leftClickReleased = false;
	middleClickPressed = false;
	middleClickReleased = false;
	rightClickPressed = false;
	rightClickReleased = false;
}

void SceneEditorCanvas::openRightClickMenu()
{
	if (gameBridge && gameBridge->getMousePos() && inputState.rawMousePos) {
		const auto mousePos = gameBridge->getMousePos().value();
		const auto menuOptions = gameBridge->getSceneContextMenu(mousePos);

		if (menuOptions.empty()) {
			return;
		}
		
		auto menu = std::make_shared<UIPopupMenu>("scene_editor_canvas_popup", factory.getStyle("popupMenu"), menuOptions);

		menu->setHandle(UIEventType::PopupAccept, [this] (const UIEvent& e) {
			gameBridge->onSceneContextMenuHighlight("");
			gameBridge->onSceneContextMenuSelection(e.getStringData());
		});

		menu->setHandle(UIEventType::PopupHoveredChanged, [this] (const UIEvent& e) {
			gameBridge->onSceneContextMenuHighlight(e.getStringData());
		});

		menu->setHandle(UIEventType::PopupCanceled, [this] (const UIEvent& e) {
			gameBridge->onSceneContextMenuHighlight("");
		});

		menu->spawnOnRoot(*getRoot());
	}
}

Vector2i SceneEditorCanvas::getCanvasSize() const
{
	auto size = Vector2i(getSize()) - Vector2i(2, 2);
	size += size.modulo(Vector2i(2, 2)); // Make even
	return size;
}
