#include "halley/ui/widgets/ui_image.h"

using namespace Halley;

UIImage::UIImage(Sprite s, std::optional<UISizer> sizer, Vector4f innerBorder)
	: UIWidget("", {}, std::move(sizer), innerBorder)
{
	setSprite(s);
}

UIImage::UIImage(String id, Sprite s, std::optional<UISizer> sizer, Vector4f innerBorder)
	: UIWidget(std::move(id), {}, std::move(sizer), innerBorder)
{
	setSprite(std::move(s));
}

void UIImage::draw(UIPainter& painter) const
{
	for (auto& b: getBehaviours()) {
		b->onParentAboutToDraw();
	}
	
	if (sprite.hasMaterial()) {
		if (layerAdjustment != 0 || clip) {
			auto p2 = painter.withAdjustedLayer(layerAdjustment);
			if (clip) {
				p2 = p2.withClip(*clip + (isLocalClip ? getPosition() : Vector2f()));
			}
			p2.draw(sprite);
		} else {
			painter.draw(sprite);
		}
	}
	drawing = 2;
}

void UIImage::update(Time t, bool moved)
{
	if (moved || dirty) {
		Vector2f basePos = getPosition();
		Vector2f imgBaseSize = sprite.getSize() + topLeftBorder + bottomRightBorder;
		if (useClipForPos && sprite.getClip()) {
			auto c = sprite.getClip().value();
			basePos -= c.getTopLeft();
			imgBaseSize = Vector2f::min(c.getSize(), imgBaseSize);
		}
		sprite
			.setPos(basePos)
			.setScale(getSize() / imgBaseSize);
		dirty = false;
	}
	if (drawing > 0) {
		--drawing;
	}
}

void UIImage::setSprite(Sprite s)
{
	sprite = std::move(s);

	const auto b = sprite.getOuterBorder();
	topLeftBorder = Vector2f(b.xy());
	bottomRightBorder = Vector2f(b.zw());
	const auto c = sprite.getClip();
	
	const auto spriteSize = (sprite.getSize() + topLeftBorder + bottomRightBorder) * sprite.getScale();
	//sprite.setAbsolutePivot(-topLeftBorder + sprite.getAbsolutePivot());
	
	if (c) {
		setMinSize(Vector2f::min(spriteSize, c->getSize()));
	} else {
		setMinSize(spriteSize);
	}
	dirty = true;
}

Sprite& UIImage::getSprite()
{
	return sprite;
}

const Sprite& UIImage::getSprite() const
{
	return sprite;
}

void UIImage::setUseClipForPos(bool useClip)
{
	useClipForPos = useClip;
}

void UIImage::setLayerAdjustment(int adjustment)
{
	layerAdjustment = adjustment;
}

void UIImage::setWorldClip(std::optional<Rect4f> wc)
{
	clip = wc;
	isLocalClip = false;
}

void UIImage::setLocalClip(std::optional<Rect4f> c)
{
	clip = c;
	isLocalClip = true;
}

void UIImage::setSelectable(Colour4f normalColour, Colour4f selColour)
{
	setHandle(UIEventType::SetSelected, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite.setColour(selColour);
		} else {
			sprite.setColour(normalColour);
		}

		sendEventDown(event, false);
	});
}

void UIImage::setSelectable(Sprite normalSprite, Sprite selectedSprite)
{
	setHandle(UIEventType::SetSelected, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite = selectedSprite;
		} else {
			sprite = normalSprite;
		}
		dirty = true;

		sendEventDown(event, false);
	});
}

void UIImage::setDisablable(Colour4f normalColour, Colour4f disabledColour)
{
	setHandle(UIEventType::SetEnabled, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite.setColour(normalColour);
		} else {
			sprite.setColour(disabledColour);
		}

		sendEventDown(event, false);
	});
}

void UIImage::setHoverableSelectable(Colour4f normalColour, Colour4f hoverColour, Colour4f selColour)
{
	setHandle(UIEventType::SetSelected, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite.setColour(selColour);
		} else {
			sprite.setColour(normalColour);
		}

		sendEventDown(event, false);
	});

	setHandle(UIEventType::SetHovered, [=] (const UIEvent& event)
	{
		const bool hovered = event.getBoolData();
		const bool selected = event.getBoolData2();
		if (!selected) {
			if (hovered) {
				sprite.setColour(hoverColour);
			} else {
				sprite.setColour(normalColour);
			}
		}

		sendEventDown(event, false);
	});
}

bool UIImage::isDrawing() const
{
	return drawing > 0;
}

bool UIImage::isMouseInside(Vector2f mousePos) const
{
	return UIWidget::isMouseInside(mousePos) && sprite.hasPointVisible(mousePos);
}

void UIImage::setDynamicValue(std::string_view key, ConfigNode value)
{
	if (key == "alpha") {
		sprite.setAlpha(value.asFloat(1.0f));
	}
}

UIImageVisibleBehaviour::UIImageVisibleBehaviour(Callback onVisible, Callback onInvisible)
	: onVisible(std::move(onVisible))
	, onInvisible(std::move(onInvisible))
{}

void UIImageVisibleBehaviour::update(Time time)
{
	const auto image = dynamic_cast<UIImage*>(getWidget());
	if (!image) {
		return;
	}
	
	setParentVisible(image->isDrawing());	
}

void UIImageVisibleBehaviour::onParentAboutToDraw()
{
	setParentVisible(true);
}

void UIImageVisibleBehaviour::setParentVisible(bool curVisible)
{
	if (curVisible != visible) {
		visible = curVisible;

		const auto image = dynamic_cast<UIImage*>(getWidget());
		if (!image) {
			return;
		}

		if (visible) {
			if (onVisible) {
				onVisible(*image);
			}
		} else {
			if (onInvisible) {
				onInvisible(*image);
			}
		}
	}
}

void UIImage::setHoverable(Colour4f normalColour, Colour4f selColour)
{
	setHandle(UIEventType::SetHovered, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite.setColour(selColour);
		} else {
			sprite.setColour(normalColour);
		}

		sendEventDown(event, false);
	});
}

void UIImage::setHoverable(Sprite normalSprite, Sprite selectedSprite)
{
	setHandle(UIEventType::SetHovered, [=] (const UIEvent& event)
	{
		if (event.getBoolData()) {
			sprite = selectedSprite;
		} else {
			sprite = normalSprite;
		}
		dirty = true;

		sendEventDown(event, false);
	});
}


UIPulseSpriteBehaviour::UIPulseSpriteBehaviour(Colour4f colour, Time period, Time startTime)
	: colour(colour)
	, curTime(startTime)
	, period(period)
	, waitingForInitialColour(true)
{
}

UIPulseSpriteBehaviour::UIPulseSpriteBehaviour(Colour4f col0, Colour col1, Time period, Time startTime)
	: initialColour(col0)
	, colour(col1)
	, curTime(startTime)
	, period(period)
{
}

void UIPulseSpriteBehaviour::init()
{
	if (waitingForInitialColour) {
		auto* image = dynamic_cast<UIImage*>(getWidget());
		if (image) {
			initialColour = image->getSprite().getColour();
		}
		waitingForInitialColour = false;
	}
}

void UIPulseSpriteBehaviour::update(Time time)
{
	curTime = std::fmod(curTime + time, period);
	auto* image = dynamic_cast<UIImage*>(getWidget());
	if (image) {
		const auto t = curTime / period;
		const auto col = lerp(colour, initialColour, std::cos(static_cast<float>(t * 2.0 * pi())) * 0.5f + 0.5f);
		image->getSprite().setColour(col);
	}
}
