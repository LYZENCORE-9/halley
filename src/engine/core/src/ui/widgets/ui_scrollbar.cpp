#include "halley/ui/widgets/ui_scrollbar.h"
#include "halley/ui/ui_style.h"
#include "halley/ui/widgets/ui_image.h"
#include "halley/ui/widgets/ui_scroll_pane.h"
#include "halley/ui/widgets/ui_button.h"

using namespace Halley;

UIScrollBar::UIScrollBar(String id, UIScrollDirection direction, UIStyle style, bool alwaysShow)
	: UIWidget(std::move(id), Vector2f(), UISizer(direction == UIScrollDirection::Horizontal ? UISizerType::Horizontal : UISizerType::Vertical))
	, direction(direction)
	, alwaysShow(alwaysShow)
{
	styles.emplace_back(style);
	if (style.hasSubStyle(direction == UIScrollDirection::Horizontal ? "left" : "up")) {
		b0 = std::make_shared<UIButton>("b0", style.getSubStyle(direction == UIScrollDirection::Horizontal ? "left" : "up"));
		UIWidget::add(b0);
	}

	auto barStyle = style.getSubStyle("bar");
	bar = std::make_shared<UIImage>(barStyle.getSprite(direction == UIScrollDirection::Horizontal ? "horizontal" : "vertical"));
	thumb = std::make_shared<UIScrollThumb>(style.getSubStyle("thumb"));
	thumbMinSize = thumb->getMinimumSize();
	bar->add(thumb);
	UIWidget::add(bar, 1);

	if (style.hasSubStyle(direction == UIScrollDirection::Horizontal ? "right" : "down")) {
		b1 = std::make_shared<UIButton>("b1", style.getSubStyle(direction == UIScrollDirection::Horizontal ? "right" : "down"));
		UIWidget::add(b1);
	}

	setHandle(UIEventType::ButtonClicked, [=] (const UIEvent& event)
	{
		if (event.getSourceId() == "b0") {
			scrollLines(-1);
		} else if (event.getSourceId() == "b1") {
			scrollLines(1);
		}
	});

	setHandle(UIEventType::Dragged, [=] (const UIEvent& event)
	{
		onScrollDrag(event.getVectorData() - bar->getPosition());
	});
}

void UIScrollBar::update(Time t, bool moved)
{
	checkActive();
	thumb->setActive(isEnabled());
	if (b0) {
		b0->setEnabled(isEnabled());
	}
	if (b1) {
		b1->setEnabled(isEnabled());
	}

	if (isEnabled()) {
		int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;

		float coverage = pane->getCoverageSize(direction);
		float barSize = bar->getSize()[axis];
		float sz = round(barSize * coverage);
		float szAdjustment = 0.0f;
		if (sz < thumbMinSize[axis]) {
			szAdjustment = thumbMinSize[axis] - sz;
			sz = thumbMinSize[axis];
		}

		auto thumbSize = bar->getSize();
		thumbSize[axis] = sz;
		thumbSize[1 - axis] = thumbMinSize[1 - axis];
		Vector2f thumbPos;
		thumbPos[axis] = round(pane->getRelativeScrollPosition()[axis] * (barSize - szAdjustment));
		thumbPos[1 - axis] = std::round((bar->getSize()[1 - axis] - thumbSize[1 - axis]) / 2);

		thumb->setPosition(bar->getPosition() + thumbPos);
		thumb->setMinSize(thumbSize);
	}
}

void UIScrollBar::setScrollPane(UIScrollPane& p)
{
	pane = &p;
}

bool UIScrollBar::canInteractWithMouse() const
{
	return true;
}

void UIScrollBar::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	if (isEnabled()) {
		int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;

		auto relative = (mousePos - bar->getPosition()) / bar->getSize();
		float clickPos = relative[axis];
		float coverage = pane->getCoverageSize(direction);
		auto curPos = pane->getRelativeScrollPosition()[axis];

		float dir = clickPos < curPos + coverage * 0.5f ? -1.0f : 1.0f;
		pane->setRelativeScroll(curPos + coverage * dir, direction);
	}
}

void UIScrollBar::releaseMouse(Vector2f mousePos, int button)
{
}

void UIScrollBar::scrollLines(int lines)
{
	if (isEnabled()) {
		int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;
		auto pos = pane->getScrollPosition();
		pos[axis] += pane->getScrollSpeed() * lines;
		pane->scrollTo(pos);
	}
}

void UIScrollBar::onScrollDrag(Vector2f relativePos)
{
	if (isEnabled()) {
		int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;
		auto relative = relativePos / bar->getSize();
		float clickPos = relative[axis];
		pane->setRelativeScroll(clickPos, direction);
	}
}

void UIScrollBar::checkActive()
{
	bool enabled = pane && pane->canScroll(direction);
	setEnabled(enabled);
	setActive(alwaysShow || enabled);
}

UIScrollThumb::UIScrollThumb(UIStyle style)
	: UIButton("scrollThumb", style)
{
	setShrinkOnLayout(true);
}

void UIScrollThumb::onMouseOver(Vector2f mousePos)
{
	if (dragging) {
		setDragPos(mousePos - mouseStartPos + myStartPos);
	}
}

void UIScrollThumb::pressMouse(Vector2f mousePos, int button, KeyMods keyMods)
{
	UIButton::pressMouse(mousePos, button, keyMods);
	if (button == 0) {
		dragging = true;
		mouseStartPos = mousePos;
		myStartPos = getPosition();
	}
}

void UIScrollThumb::releaseMouse(Vector2f mousePos, int button)
{
	UIButton::releaseMouse(mousePos, button);
	if (button == 0) {
		if (dragging) {
			onMouseOver(mousePos);
			dragging = false;
		}
	}
}

std::optional<MouseCursorMode> UIScrollThumb::getMouseCursorMode() const
{
	return std::nullopt;
}

void UIScrollThumb::setDragPos(Vector2f pos)
{
	sendEvent(UIEvent(UIEventType::Dragged, getId(), pos));
}

void UIScrollBar::setAlwaysShow(bool show)
{
	alwaysShow = show;
}

bool UIScrollBar::isAlwaysShow() const
{
	return alwaysShow;
}
