#pragma once

#include "../ui_widget.h"
#include "halley/graphics/sprite/animation_player.h"

namespace Halley
{
	class AnimationPlayer;

	class UIAnimation : public UIWidget
	{
	public:
		UIAnimation(String id, Vector2f size, Vector2f animationOffset, AnimationPlayer animation);
		UIAnimation(String id, Vector2f size, std::optional<UISizer> sizer, Vector2f animationOffset, AnimationPlayer animation);

		AnimationPlayer& getPlayer();
		const AnimationPlayer& getPlayer() const;
		Sprite& getSprite();
		const Sprite& getSprite() const;

		Vector2f getOffset() const;
		void setOffset(Vector2f offset);

		void setColour(Colour4f colour);
		Colour4f getColour() const;

		bool isMouseInside(Vector2f mousePos) const override;

	protected:
		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;

	private:
		Vector2f offset;
		AnimationPlayer animation;
		Sprite sprite;
		Colour4f colour;
	};
}
