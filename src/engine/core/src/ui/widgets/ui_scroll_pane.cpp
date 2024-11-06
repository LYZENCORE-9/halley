#include "halley/ui/widgets/ui_scroll_pane.h"
#include "halley/ui/ui_style.h"
#include "halley/support/logger.h"

using namespace Halley;

UIScrollPane::UIScrollPane(String id, Vector2f clipSize, UISizer&& sizer, bool scrollHorizontal, bool scrollVertical, float scrollSpeed, bool alwaysSmooth)
	: UIWidget(std::move(id), Vector2f(), std::move(sizer))
	, clipSize(clipSize)
	, scrollSpeed(scrollSpeed)
	, alwaysSmooth(alwaysSmooth)
	, scrollHorizontal(scrollHorizontal)
	, scrollVertical(scrollVertical)
{
	setHandle(UIEventType::MouseWheel, [this] (const UIEvent& event)
	{
		onMouseWheel(event);
	});

	setHandle(UIEventType::MakeAreaVisible, [this] (const UIEvent& event)
	{
		refresh();
		scrollToShow(event.getRectData() + getBasePosition(event.getSourceId()), false, false);
	});

	setHandle(UIEventType::MakeAreaVisibleCentered, [this] (const UIEvent& event)
	{
		refresh();
		scrollToShow(event.getRectData() + getBasePosition(event.getSourceId()), true, false);
	});

	setHandle(UIEventType::MakeAreaVisibleContinuous, [this] (const UIEvent& event)
	{
		refresh();
		scrollToShow(event.getRectData() + getBasePosition(event.getSourceId()), false, true);
	});
}

UIScrollPane::UIScrollPane(Vector2f clipSize, UISizer&& sizer, bool scrollHorizontal, bool scrollVertical)
: UIScrollPane("", clipSize, std::move(sizer), scrollHorizontal, scrollVertical)
{
}

Vector2f UIScrollPane::getScrollPosition() const
{
	return scrollPos;
}

Vector2f UIScrollPane::getRelativeScrollPosition() const
{
	return scrollPos / std::max(contentsSize, Vector2f(1.0f, 1.0f));
}

Vector2f UIScrollPane::getRelativeScrollEndPosition() const
{
	return (scrollPos + clipSize) / std::max(contentsSize, Vector2f(1.0f, 1.0f));
}

void UIScrollPane::setClipSize(Vector2f clipSize)
{
	this->clipSize = clipSize;
}

void UIScrollPane::scrollTo(Vector2f position)
{
	const auto target = clampScrollPos(position);
	const auto old = scrollPos;

	if (scrollHorizontal) {
		scrollPos.x = target.x;
	}
	
	if (scrollVertical) {
		scrollPos.y = target.y;
	}

	if (scrollPos != old) {
		sendEventDown(UIEvent(UIEventType::ScrollPositionChanged, getId(), Vector2f(scrollPos)));
	}
}

Vector2f UIScrollPane::clampScrollPos(Vector2f pos) const
{
	return Vector2f(clamp2(pos.x, 0.0f, scrollHorizontal ? std::max(contentsSize.x - getSize().x, 0.0f) : 0.0f),
					clamp2(pos.y, 0.0f, scrollVertical ? std::max(contentsSize.y - getSize().y, 0.0f) : 0.0f));
}

void UIScrollPane::scrollBy(Vector2f delta)
{
	scrollTo(scrollPos + delta);
}

void UIScrollPane::setScrollSpeed(float speed)
{
	scrollSpeed = speed;
}

void UIScrollPane::update(Time t, bool moved)
{
	if (targetScrollTo) {
		const auto p0 = scrollPos;
		const auto p1 = *targetScrollTo;
		const auto delta = p1 - p0;
		const auto dist = delta.length();
		const auto speed = std::max(scrollSpeed, dist / 0.1f);

		if (dist < 1) {
			scrollTo(p1);
			targetScrollTo = {};
		} else {
			const auto p = lerp(p0, p1, clamp(static_cast<float>(speed * t) / dist, 0.0f, 1.0f));
			if ((p - p1).length() < 1) {
				scrollTo(p1);
				targetScrollTo = {};
			} else {
				scrollTo(p);
			}
		}
	}

	refresh();
	if (t > 0.0001) {
		lastDeltaT = t;
	}
}

bool UIScrollPane::isScrolling(float threshold) const
{
	if (targetScrollTo) {
		const auto p0 = scrollPos;
		const auto p1 = *targetScrollTo;
		const auto delta = p1 - p0;
		const auto dist = delta.length();
		return dist > threshold;
	} else {
		return false;
	}
}

bool UIScrollPane::canScroll(UIScrollDirection direction) const
{
	auto contentsSize = UIWidget::getLayoutMinimumSize(false);
	if (direction == UIScrollDirection::Horizontal) {
		return scrollHorizontal && getSize().x < contentsSize.x;
	} else {
		return scrollVertical && getSize().y < contentsSize.y;
	}
}

float UIScrollPane::getCoverageSize(UIScrollDirection direction) const
{
	auto contentsSize = UIWidget::getLayoutMinimumSize(false);
	if (direction == UIScrollDirection::Horizontal) {
		return getSize().x / contentsSize.x;
	} else {
		return getSize().y / contentsSize.y;
	}
}

void UIScrollPane::setScrollWheelEnabled(bool enabled)
{
	scrollWheelEnabled = enabled;
}

bool UIScrollPane::isScrollWheelEnabled() const
{
	return scrollWheelEnabled;
}

void UIScrollPane::refresh(bool force)
{
	if (!scrollHorizontal) {
		clipSize.x = getSize().x;
		scrollPos.x = 0;
	}
	if (!scrollVertical) {
		clipSize.y = getSize().y;
		scrollPos.y = 0;
	}
	contentsSize = UIWidget::getLayoutMinimumSize(false);

	setMouseClip(getRect(), force);
	scrollTo(getScrollPosition());
}

void UIScrollPane::drawChildren(UIPainter& painter) const
{
	auto p = painter.withClip(getRect());
	UIWidget::drawChildren(p);
}

Vector2f UIScrollPane::getLayoutMinimumSize(bool force) const
{
	auto size = UIWidget::getLayoutMinimumSize(false);
	if (scrollHorizontal) {
		size.x = std::min(size.x, clipSize.x);
	}
	if (scrollVertical) {
		size.y = std::min(size.y, clipSize.y);
	}
	return size;
}

bool UIScrollPane::canInteractWithMouse() const
{
	return true;
}

void UIScrollPane::onLayout()
{
	refresh();
}

void UIScrollPane::onMouseWheel(const UIEvent& event)
{
	if (scrollWheelEnabled) {
		auto baseScroll = scrollSpeed * event.getVectorData();
		if (!scrollVertical && std::abs(baseScroll.x) < 0.01f) {
			baseScroll.x = -baseScroll.y;
		}
		const auto delta = baseScroll * Vector2f(scrollHorizontal ? 1.0f : 0.0f, scrollVertical ? -1.0f : 0.0f);

		if (delta.squaredLength() > 0.00001f) {
			scrollBy(delta);
		}
	}
}

Vector2f UIScrollPane::getBasePosition(const String& widgetId)
{
	auto widget = tryGetWidget(widgetId);
	if (widget) {
		return widget->getPosition() + scrollPos - getPosition();
	} else {
		return Vector2f();
	}
}

Vector2f UIScrollPane::getLayoutOriginPosition() const
{
	return getPosition() - scrollPos.floor();
}

void UIScrollPane::scrollToShow(Rect4f rect, bool center, bool continuous)
{
	targetScrollTo = {};

	auto size = getSize();

	float maxX = rect.getLeft();
	float minX = rect.getRight() - size.x;
	float maxY = rect.getTop();
	float minY = rect.getBottom() - size.y;
	const auto target = center ? (rect.getCenter() - 0.5f * size) : scrollPos;
	auto dst = Vector2f(clamp(target.x, minX, maxX), clamp(target.y, minY, maxY));

	if (continuous) {
		const auto maxPixelsPerSecond = 600.0f;
		const auto maxDelta = static_cast<float>(lastDeltaT) * maxPixelsPerSecond;
		dst.x = clamp(dst.x, scrollPos.x - maxDelta, scrollPos.x + maxDelta);
		dst.y = clamp(dst.y, scrollPos.y - maxDelta, scrollPos.y + maxDelta);
		scrollTo(dst);
	} else if (alwaysSmooth && !center) {
		targetScrollTo = clampScrollPos(dst);
	} else {
		scrollTo(dst);
	}
}

float UIScrollPane::getScrollSpeed() const
{
	return scrollSpeed;
}

void UIScrollPane::setRelativeScroll(float position, UIScrollDirection direction)
{
	int axis = direction == UIScrollDirection::Horizontal ? 0 : 1;
	auto target = scrollPos;
	target[axis] = position * contentsSize[axis];
	scrollTo(target);
}

std::optional<float> UIScrollPane::getMaxChildWidth() const
{
	if (scrollHorizontal) {
		return std::optional<float>();
	} else {
		return getSize().x;
	}
}

bool UIScrollPane::ignoreClip() const
{
	return true;
}

void UIScrollPane::onChildrenAdded()
{
	refresh(true);
}

void UIScrollPane::onChildrenRemoved()
{
	refresh(true);
}

bool UIScrollPane::canChildrenInteractWithMouseAt(Vector2f pos) const
{
	return getRect().contains(pos);
}
