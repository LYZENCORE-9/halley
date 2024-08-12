#pragma once
#include "ui_clickable.h"
#include "ui_label.h"
#include "halley/graphics/sprite/sprite.h"

namespace Halley {
	class UIImage;
	
	class UIButton : public UIClickable {
	public:
		UIButton(String id, UIStyle style, std::optional<UISizer> sizer = {});
		UIButton(String id, UIStyle style, LocalisedString label);

		void draw(UIPainter& painter) const override;
		void update(Time t, bool moved) override;
		void onClicked(Vector2f mousePos, KeyMods keyMods) override;
		void onRightClicked(Vector2f mousePos, KeyMods keyMods) override;
		void setInputType(UIInputType uiInput) override;

		bool canInteractWithMouse() const override;
		bool isFocusLocked() const override;

		void onManualControlActivate() override;

		void setCanDoBorderOnly(bool canDo);
		void setPreciseClick(bool enabled);
		bool isPreciseClick() const;

		void setLabel(LocalisedString string);
		const LocalisedString& getLabel() const;
		void setIcon(Sprite icon);

		bool isMouseInside(Vector2f mousePos) const override;

	protected:
		void doSetState(State state) override;
		void onStateChanged(State prev, State next) override;
		void onShortcutPressed() override;

		Sprite sprite;
		UIInputType curInputType = UIInputType::Undefined;
		std::shared_ptr<UILabel> label;
		std::shared_ptr<UIImage> iconImage;
		bool borderOnly = false;
		bool canDoBorderOnly = true;
		bool preciseClick = false;
	};
}
