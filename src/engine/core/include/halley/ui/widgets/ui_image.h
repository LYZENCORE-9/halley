#pragma once
#include "../ui_behaviour.h"
#include "../ui_widget.h"
#include "halley/graphics/sprite/sprite.h"

namespace Halley {
	class UIImage : public UIWidget {
	public:
		explicit UIImage(Sprite sprite, std::optional<UISizer> sizer = {}, Vector4f innerBorder = {});
		explicit UIImage(String id, Sprite sprite, std::optional<UISizer> sizer = {}, Vector4f innerBorder = {});

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		void setSprite(Sprite sprite);
		Sprite& getSprite();
		const Sprite& getSprite() const;

		void setUseClipForPos(bool useClip);

		void setLayerAdjustment(int adjustment);
		void setWorldClip(std::optional<Rect4f> worldClip);
		void setLocalClip(std::optional<Rect4f> localClip);
		void setHoverable(Colour4f normalColour, Colour4f hoverColour);
		void setHoverable(Sprite normalSprite, Sprite hoverSprite);
		void setSelectable(Colour4f normalColour, Colour4f selColour);
		void setSelectable(Sprite normalSprite, Sprite selectedSprite);
		void setDisablable(Colour4f normalColour, Colour4f disabledColour);
		void setHoverableSelectable(Colour4f normalColour, Colour4f hoverColour, Colour4f selColour);

		bool isDrawing() const;
		bool isMouseInside(Vector2f mousePos) const override;

		void setDynamicValue(std::string_view key, ConfigNode value) override;

	private:
		Sprite sprite;
		Vector2f topLeftBorder;
		Vector2f bottomRightBorder;
		int layerAdjustment = 0;
		bool dirty = true;
		bool useClipForPos = true;
		bool isLocalClip = false;
		mutable uint8_t drawing = 0;
		std::optional<Rect4f> clip;
	};

	class UIImageVisibleBehaviour : public UIBehaviour {
	public:
		using Callback = std::function<void(UIImage&)>;

		UIImageVisibleBehaviour(Callback onVisible, Callback onInvisible);
		
		void update(Time time) override;
		void onParentAboutToDraw() override;
	
	private:
		Callback onVisible;
		Callback onInvisible;
		bool visible = false;

		void setParentVisible(bool visible);
	};

	class UIPulseSpriteBehaviour : public UIBehaviour {
	public:
		UIPulseSpriteBehaviour(Colour4f colour, Time period, Time startTime = 0.0);
		UIPulseSpriteBehaviour(Colour4f col0, Colour col1, Time period, Time startTime = 0.0);

		void init() override;
		void update(Time time) override;

	private:
		Colour4f initialColour;
		Colour4f colour;
		Time curTime = 0;
		Time period = 0;
		bool waitingForInitialColour = false;
	};
}
