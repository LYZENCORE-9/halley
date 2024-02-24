#include "translate_gizmo.h"

#include <components/sprite_component.h>

#include "halley/entity/components/transform_2d_component.h"
#include "halley/game/scene_editor_interface.h"
#include "halley/graphics/painter.h"
using namespace Halley;

TranslateGizmo::TranslateGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow)
	: SceneEditorGizmo(snapRules)
	, factory(factory)
	, sceneEditorWindow(sceneEditorWindow)
{
	loadHandles();
}

void TranslateGizmo::loadHandles()
{
	if (getEntities().empty()) {
		handle.reset();
	} else {
		handle = SceneEditorGizmoHandle();
		handle->setBoundsCheck([=] (Vector2f myPos, Vector2f mousePos) -> bool
		{
			return getHandleBounds(*handle).contains(mousePos);
		});
		handle->setGridSnap(getSnapRules().grid);
	}
}

void TranslateGizmo::update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState)
{
	const auto curMode = fromString<TranslateGizmoMode>(sceneEditorWindow.getSetting(EditorSettingType::Editor, "tools.translate.mode").asString("pivot"));
	if (curMode != mode) {
		setMode(curMode);
	}
	
	// Drag
	if (handle) {
		const auto delta = handle->update(inputState);
		if (delta) {
			pendingMoveBy += delta.value();
		}
		doMoveBy();
		doRotate();

		if (!handle->isHeld()) {
			const auto n = getEntities().size();
			size_t nValid = 0;
			Vector2f pos;
			for (size_t i = 0; i < n; ++i) {
				const auto* transform = getComponent<Transform2DComponent>(i);
				if (transform) {
					++nValid;
					pos += transform->getGlobalPosition();
				}
			}
			handle->setEnabled(nValid > 0);
			if (nValid > 0) {
				handle->setPosition(pos / static_cast<float>(nValid), true);
			}
		}
	}
}

void TranslateGizmo::draw(Painter& painter, const ISceneEditor& sceneEditor) const
{
	if (handle && handle->isEnabled()) {
		const float zoom = getZoom();
		const auto overCol = Colour4f(0.6f, 0.6f, 1);
		const auto outCol = Colour4f(0.4f, 0.4f, 1.0f);
		const auto col = handle->isOver() ? overCol : outCol;
		const auto circle = getHandleBounds(*handle);

		const auto centre = circle.getCentre();
		const auto radius = circle.getRadius();
		const float lineWidth = 2.0f / zoom;
		const float fineLineWidth = 1.0f / zoom;

		Painter::LineParameters params;
		params.pixelAlign = false;
		
		painter.drawCircle(centre, radius, lineWidth + 2 / zoom, Colour4f(0, 0, 0, 0.5f), {}, params);
		painter.drawCircle(centre, radius, lineWidth, col, {}, params);
		painter.drawLine({{ centre - Vector2f(radius * 0.6f, 0), centre + Vector2f(radius * 0.6f, 0) }}, fineLineWidth, col, false, {}, params);
		painter.drawLine({{ centre - Vector2f(0, radius * 0.6f), centre + Vector2f(0, radius * 0.6f) }}, fineLineWidth, col, false, {}, params);
	}
}

bool TranslateGizmo::isHighlighted() const
{
	return handle && handle->isOver();
}

std::shared_ptr<UIWidget> TranslateGizmo::makeUI()
{
	auto ui = factory.makeUI("halley/translate_gizmo_toolbar");
	ui->setInteractWithMouse(true);

	uiMode = ui->getWidgetAs<UIList>("mode");

	const auto initialMode = sceneEditorWindow.getSetting(EditorSettingType::Editor, "tools.translate.mode").asString("pivot");
	setMode(fromString<TranslateGizmoMode>(initialMode));
	ui->bindData("mode", initialMode, [=] (const String& value)
	{
		setMode(fromString<TranslateGizmoMode>(value));
	});

	ui->setHandle(UIEventType::ButtonClicked, "turnLeft", [=] (const UIEvent& event)
	{
		rotate(1);
	});

	ui->setHandle(UIEventType::ButtonClicked, "turnRight", [=] (const UIEvent& event)
	{
		rotate(-1);
	});
	
	return ui;
}

Vector<String> TranslateGizmo::getHighlightedComponents() const
{
	return { "Transform2D" };
}

bool TranslateGizmo::onKeyPress(KeyboardKeyPress key)
{
	const bool fast = (int(key.mod) & int(KeyMods::Shift)) != 0;
	const bool iso = (int(key.mod) & int(KeyMods::Ctrl)) != 0;
	const int speed = fast ? 5 : 1;
	const Vector2i xAxis = (iso ? Vector2i(2, 1) : Vector2i(1, 0)) * speed;
	const Vector2i yAxis = (iso ? Vector2i(2, -1) : Vector2i(0, -1)) * speed;

	if (key.key == KeyCode::Left) {
		moveBy(-xAxis);
		return true;
	}
	if (key.key == KeyCode::Right) {
		moveBy(xAxis);
		return true;
	}
	if (key.key == KeyCode::Up) {
		moveBy(yAxis);
		return true;
	}
	if (key.key == KeyCode::Down) {
		moveBy(-yAxis);
		return true;
	}
	if (key.is(KeyCode::Q)) {
		rotate(1);
		return true;
	}
	if (key.is(KeyCode::E)) {
		rotate(-1);
		return true;
	}

	return false;
}

bool TranslateGizmo::canBoxSelectEntities() const
{
	return true;
}

void TranslateGizmo::onEntityChanged()
{
	loadHandles();
}

Circle TranslateGizmo::getHandleBounds(const SceneEditorGizmoHandle& handle) const
{
	const auto pos = handle.getPosition();
	return Circle(pos, 10.0f / getZoom());
}

void TranslateGizmo::updatePositionData(Vector2f pos, size_t idx)
{
	auto* data = getComponentData("Transform2D", idx);
	if (data) {
		(*data)["position"] = pos;
	}
	markModified("Transform2D", "position", idx);
}

void TranslateGizmo::updateRotationData(Angle1f rotation, size_t idx)
{
	auto* data = getComponentData("Transform2D", idx);
	if (data) {
		(*data)["rotation"] = rotation.getDegrees();
	}
	markModified("Transform2D", "rotation", idx);
}

Vector2f TranslateGizmo::getObjectOffset(size_t idx) const
{
	if (mode == TranslateGizmoMode::Centre) {
		auto sprite = getComponent<SpriteComponent>(idx);
		if (sprite) {
			auto offset = sprite->sprite.getAABB().getCenter() - sprite->sprite.getPosition();
			if (getSnapRules().grid == GridSnapMode::Pixel) {
				offset = offset.round();
			}
			return offset;
		}		
	}
	return Vector2f();
}

void TranslateGizmo::setMode(TranslateGizmoMode mode)
{
	this->mode = mode;
	uiMode->setSelectedOptionId(toString(mode));
	sceneEditorWindow.setSetting(EditorSettingType::Editor, "tools.translate.mode", ConfigNode(toString(mode)));
}

void TranslateGizmo::moveBy(Vector2i delta)
{
	pendingMoveBy += Vector2f(delta);
}

void TranslateGizmo::doMoveBy()
{
	if (pendingMoveBy.manhattanLength() < 0.0001f) {
		return;
	}

	doMoveBy(pendingMoveBy);

	pendingMoveBy = Vector2f();
}

void TranslateGizmo::doMoveBy(Vector2f delta)
{
	const auto& entities = getEntities();
	const size_t n = entities.size();

	Vector<Vector2f> targetPos;
	targetPos.resize(n);

	for (size_t i = 0; i < n; ++i) {
		if (const auto transform = getComponent<Transform2DComponent>(i)) {
			targetPos[i] = transform->getGlobalPosition() + Vector2f(delta);
		}
	}
	
	for (size_t i = 0; i < n; ++i) {
		if (!isDescendentOf(entities[i], entities)) {
			if (const auto transform = getComponent<Transform2DComponent>(i)) {
				transform->setGlobalPosition(targetPos[i]);
				const auto newLocalPos = transform->getLocalPosition();
				updatePositionData(newLocalPos, i);
			}
		}
	}
}

void TranslateGizmo::rotate(int delta)
{
	pendingRotate += delta;
}

void TranslateGizmo::doRotate()
{
	if (pendingRotate == 0) {
		return;
	}

	const auto& entities = getEntities();
	const size_t n = entities.size();

	for (size_t i = 0; i < n; ++i) {
		if (!isDescendentOf(entities[i], entities)) {
			if (const auto transform = getComponent<Transform2DComponent>(i)) {
				const auto newRotation = transform->getLocalRotation() + Angle1f::fromDegrees(90.0f * pendingRotate);
				transform->setLocalRotation(newRotation);
				updateRotationData(newRotation, i);
			}
		}
	}

	pendingRotate = 0;
}

bool TranslateGizmo::isDescendentOf(EntityRef e, gsl::span<const EntityRef> entities) const
{
	if (!e.hasParent()) {
		return false;
	}

	if (std_ex::contains(entities, e.getParent())) {
		return true;
	}

	return isDescendentOf(e.getParent(), entities);
}
