#pragma once

#include "../ui_widget.h"
#include "halley/graphics/sprite/sprite.h"
#include "halley/graphics/text/text_renderer.h"
#include "ui_clickable.h"

namespace Halley {
	class I18N;
	class UIStyle;
	class UIValidator;
	class UIList;
	class UIScrollPane;

	class UIDropdown : public UIClickable {
	public:
		class Entry {
		public:
			String id;
			LocalisedString label;
			Sprite icon;
			int value = 0;

			Entry() = default;
			Entry(String id, LocalisedString label, Sprite icon = Sprite(), int value = 0)
				: id(std::move(id))
				, label(std::move(label))
				, icon(std::move(icon))
				, value(value)
			{}
			Entry(String id, const String& label, Sprite icon = Sprite(), int value = 0)
				: id(std::move(id))
				, label(LocalisedString::fromUserString(label))
				, icon(std::move(icon))
				, value(value)
			{}
		};
		
		UIDropdown(String id, UIStyle style, Vector<LocalisedString> options = {}, int defaultOption = 0);

		virtual void setSelectedOption(int option);
		virtual void setSelectedOption(const String& id);
		void setSelectedOptionPartialMatch(const String& match);
		int getSelectedOption() const;
		String getSelectedOptionId() const;
		LocalisedString getSelectedOptionText() const;
		int getNumberOptions() const;

		void setInputButtons(const UIInputButtons& buttons) override;
		void setOptions(Vector<LocalisedString> options, int defaultOption = -1);
		void setOptions(Vector<String> optionsIds, int defaultOption = -1);
		void setOptions(gsl::span<const String> optionsIds, int defaultOption = -1);
		void setOptions(Vector<String> optionIds, Vector<LocalisedString> options, int defaultOption = -1);
		void setOptions(Vector<Entry> options, int defaultOption = -1);
		void setOptions(const I18N& i18n, const String& i18nPrefix, Vector<String> optionIds, int defaultOption = -1);

		void onManualControlCycleValue(int delta) override;
		void onManualControlActivate() override;

		bool canReceiveFocus() const override;

		void setNotifyOnHover(bool enabled);

	protected:		
		Vector<Entry> options;
		
		TextRenderer label;
		Sprite icon;
		std::shared_ptr<UIList> dropdownList;
		
		int curOption = 0;

		String keypressMatch;
		Time timeSinceLastKeypress;

		void draw(UIPainter& painter) const override;
		void drawChildren(UIPainter& painter) const override;
		void update(Time t, bool moved) override;

		bool onKeyPress(KeyboardKeyPress key) override;
		void onClicked(Vector2f mousePos, KeyMods keyMods) override;
		void doSetState(State state) override;

		bool isFocusLocked() const override;

		void readFromDataBind() override;

		virtual void updateTopLabel();
		virtual void updateOptionLabels();

		virtual bool canNotifyAsDropdown() const;

	private:
		enum class OpenState : uint8_t {
			Closed,
			OpenDown,
			OpenUp
		};
		
		Sprite sprite;
		UIInputButtons inputButtons;
		std::shared_ptr<UIWidget> dropdownWindow;
		std::shared_ptr<UIScrollPane> scrollPane;
		
		OpenState openState = OpenState::Closed;

		bool notifyOnHover = false;

		void open();
		void close();
    };
}
