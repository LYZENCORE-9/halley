#include "halley/ui/widgets/ui_dropdown.h"
#include "halley/ui/ui_style.h"
#include "halley/ui/widgets/ui_image.h"
#include "halley/ui/widgets/ui_scroll_pane.h"
#include "halley/ui/widgets/ui_scrollbar.h"
#include "halley/ui/widgets/ui_list.h"
#include "halley/text/i18n.h"
#include "halley/ui/ui_data_bind.h"
#include "halley/input/input_keyboard.h"

using namespace Halley;

UIDropdown::UIDropdown(String id, UIStyle style, Vector<LocalisedString> os, int defaultOption)
	: UIClickable(std::move(id), Vector2f(style.getFloat("minSize"), style.getFloat("minSize")))
	, curOption(defaultOption)
{
	sprite = style.getSprite("normal");
	styles.emplace_back(std::move(style));

	setOptions(std::move(os));
}

void UIDropdown::setSelectedOption(int option)
{
	const int nextOption = clamp(option, 0, static_cast<int>(options.size()) - 1);
	if (curOption != nextOption) {
		curOption = nextOption;
		updateTopLabel();
		sendEvent(UIEvent(UIEventType::DropdownSelectionChanged, getId(), options[curOption].id, curOption));

		if (canNotifyAsDropdown()) {
			if (getDataBindFormat() == UIDataBind::Format::String) {
				notifyDataBind(options[curOption].id);
			} else {
				notifyDataBind(curOption);
			}
		}

		if (dropdownList) {
			dropdownList->setSelectedOption(option);
		}
	}
}

void UIDropdown::setSelectedOption(const String& id)
{
	const auto iter = std::find_if(options.begin(), options.end(), [&] (const auto& o) { return o.id == id; });
	if (iter != options.end()) {
		setSelectedOption(static_cast<int>(iter - options.begin()));
	}
}

void UIDropdown::setSelectedOptionPartialMatch(const String& match)
{
	const auto iter = std::find_if(options.begin(), options.end(), [&] (const auto& o) { return o.label.getString().startsWith(match, false); });
	if (iter != options.end()) {
		int option = static_cast<int>(iter - options.begin());
		setSelectedOption(option);
	}
}

int UIDropdown::getSelectedOption() const
{
	return curOption;
}

String UIDropdown::getSelectedOptionId() const
{
	return curOption >= 0 && curOption < static_cast<int>(options.size()) ? options[curOption].id : "";
}

LocalisedString UIDropdown::getSelectedOptionText() const
{
	return options[curOption].label;
}

int UIDropdown::getNumberOptions() const
{
	return static_cast<int>(options.size());
}

void UIDropdown::setInputButtons(const UIInputButtons& buttons)
{
	inputButtons = buttons;
	if (dropdownList) {
		dropdownList->setInputButtons(buttons);
	}
}

void UIDropdown::updateTopLabel()
{
	const auto& style = styles.at(0);
	label = style.getTextRenderer("label");
	if (options.empty()) {
		icon = Sprite();
	} else {
		label.setText(options[curOption].label);
		icon = options[curOption].icon;
	}
}

void UIDropdown::updateOptionLabels()
{
	updateTopLabel();

	const auto& style = styles.at(0);
	const float iconGap = style.getFloat("iconGap");
	
	float maxExtents = label.getExtents().x;
	for (auto& o: options) {
		const float iconSize = o.icon.hasMaterial() ? o.icon.getScaledSize().x + iconGap : 0;
		const float strSize = label.getExtents(o.label.getString().getUTF32()).x;
		maxExtents = std::max(maxExtents, iconSize + strSize);
	}

	const auto minSizeMargins = style.getBorder("minSizeMargins");
	const auto minSize = Vector2f(maxExtents, 0) + minSizeMargins.xy();
	setMinSize(Vector2f::max(getMinimumSize(), minSize));
}

bool UIDropdown::canNotifyAsDropdown() const
{
	return true;
}

void UIDropdown::setOptions(Vector<LocalisedString> os, int defaultOption)
{
	setOptions({}, std::move(os), defaultOption);
}

void UIDropdown::setOptions(Vector<String> optionIds, int defaultOption)
{
	setOptions(std::move(optionIds), {}, defaultOption);
}

void UIDropdown::setOptions(gsl::span<const String> optionsIds, int defaultOption)
{
	setOptions(Vector<String>(optionsIds.begin(), optionsIds.end()), {}, defaultOption);
}

void UIDropdown::setOptions(const I18N& i18n, const String& i18nPrefix, Vector<String> optionIds, int defaultOption)
{
	setOptions(optionIds, i18n.getVector(i18nPrefix, optionIds), defaultOption);
}

void UIDropdown::setOptions(Vector<String> oIds, Vector<LocalisedString> os, int defaultOption)
{
	Vector<Entry> entries;
	entries.resize(std::max(os.size(), oIds.size()));
	for (size_t i = 0; i < entries.size(); ++i) {
		// Be careful, do label first as id will get moved out
		entries[i].label = os.size() > i ? std::move(os[i]) : LocalisedString::fromUserString(oIds[i]);
		entries[i].id = oIds.size() > i ? std::move(oIds[i]) : toString(i);
	}

	setOptions(std::move(entries), defaultOption);
}

void UIDropdown::setOptions(Vector<Entry> os, int defaultOption)
{
	close();
	options = std::move(os);

	if (options.empty()) {
		options.emplace_back();
	}
	curOption = clamp(curOption, 0, static_cast<int>(options.size() - 1));
	updateOptionLabels();

	if (defaultOption != -1) {
		setSelectedOption(defaultOption);
	}
}

void UIDropdown::onManualControlCycleValue(int delta)
{
	if (isEnabled()) {
		setSelectedOption(modulo(curOption + delta, int(options.size())));
	}
}

void UIDropdown::onManualControlActivate()
{
	if (isEnabled()) {
		focus();
		open();
	}
}

bool UIDropdown::canReceiveFocus() const
{
	return true;
}

void UIDropdown::setNotifyOnHover(bool enabled)
{
	notifyOnHover = enabled;
}

void UIDropdown::draw(UIPainter& painter) const
{
	painter.draw(sprite);
	if (icon.hasMaterial()) {
		painter.draw(icon);
	}
	painter.draw(label);
}

void UIDropdown::drawChildren(UIPainter& painter) const
{
	auto p2 = painter.withNoClip();
	UIClickable::drawChildren(p2);
}

void UIDropdown::update(Time t, bool moved)
{
	setChildLayerAdjustment(openState == OpenState::Closed ? 0 : 1);

	bool optionsUpdated = false;
	for (auto& o: options) {
		if (o.label.checkForUpdates()) {
			optionsUpdated = true;
		}
	}
	if (optionsUpdated) {
		updateOptionLabels();
	}

	if (openState != OpenState::Closed) {
		auto focus = getRoot()->getCurrentFocus();
		if (!focus || (focus != this && !focus->isDescendentOf(*this))) {
			close();
		}
	}

	const auto& style = styles.at(0);
	if (isEnabled()) {
		if (openState == OpenState::OpenDown) {
			sprite = style.getSprite("open");
		} else if (openState == OpenState::OpenUp) {
			sprite = style.getSprite("openUp");
		} else {
			sprite = isMouseOver() ? style.getSprite("hover") : style.getSprite("normal");
		}
	} else {
		sprite = style.getSprite("disabled");
	}

	sprite.setPos(getPosition()).scaleTo(getSize());

	const Vector2f basePos = getPosition() + style.getBorder("labelBorder").xy();
	Vector2f iconOffset;
	if (icon.hasMaterial()) {
		icon.setPosition(basePos);
		iconOffset = Vector2f(style.getFloat("iconGap") + icon.getScaledSize().x, 0.0f);
	}
	label.setAlignment(0.0f).setPosition(basePos + iconOffset);

	if (dropdownWindow) {
		dropdownWindow->setPosition(getPosition() + Vector2f(0.0f, openState == OpenState::OpenDown ? getSize().y : (-dropdownWindow->getSize().y)));
	}

	timeSinceLastKeypress += t;

	UIClickable::update(t, moved);
}

bool UIDropdown::onKeyPress(KeyboardKeyPress key)
{
	if (!dropdownList) {
		// FIXME: The widget isn't in focus if just hovered and not yet clicked.
		// This check blocks updates in that case.
		return false;
	}

	if (timeSinceLastKeypress > 1.5) {
		keypressMatch = "";
	}
	timeSinceLastKeypress = 0.0;

	if (key.key >= KeyCode::A && key.key <= KeyCode::Z) {
		int ch = 'a' + (int(key.key) - int(KeyCode::A));
		keypressMatch += char(ch);
		setSelectedOptionPartialMatch(keypressMatch);
	}

	return false;
}

void UIDropdown::onClicked(Vector2f mousePos, KeyMods keyMods)
{
	if (openState == OpenState::Closed) {
		open();
	} else {
		close();
	}
}

void UIDropdown::doSetState(State state)
{
}

bool UIDropdown::isFocusLocked() const
{
	return openState != OpenState::Closed || UIClickable::isFocusLocked();
}

void UIDropdown::readFromDataBind()
{
	auto data = getDataBind();
	if (data->getFormat() == UIDataBind::Format::String) {
		const auto target = data->getStringData();
		setSelectedOption(target);
		if (getSelectedOptionId() != target) {
			if (canNotifyAsDropdown()) {
				notifyDataBind(getSelectedOptionId(), true);
			}
		}
	} else {
		setSelectedOption(data->getIntData());
	}
}

void UIDropdown::open()
{
	const auto& style = styles.at(0);
	if (openState == OpenState::Closed) {
		const auto standardHeight = style.getFloat("height");
		const auto rootRect = getRoot()->getRect();
		const auto distanceFromBottom = rootRect.getBottom() - getRect().getBottom() - 5.0f;
		const auto distanceFromTop = getRect().getTop() - rootRect.getTop() - 5.0f;

		openState = distanceFromBottom >= standardHeight ? OpenState::OpenDown : OpenState::OpenUp;
		const auto height = openState == OpenState::OpenDown ? standardHeight : std::min(standardHeight, distanceFromTop);

		dropdownList = std::make_shared<UIList>(getId() + "_list", style.getSubStyle("listStyle"));
		int i = 0;
		for (const auto& o: options) {
			if (o.icon.hasMaterial()) {
				dropdownList->addTextIconItem(toString(i++), o.label, o.icon);
			} else {
				dropdownList->addTextItem(toString(i++), o.label);
			}
		}
		dropdownList->setSelectedOption(curOption);
		dropdownList->setInputButtons(inputButtons);
		getRoot()->setFocus(dropdownList);

		scrollPane = std::make_shared<UIScrollPane>(getId() + "_pane", Vector2f(0, height), UISizer(UISizerType::Vertical, 0));
		scrollPane->add(dropdownList);

		auto scrollBar = std::make_shared<UIScrollBar>(getId() + "_vbar", UIScrollDirection::Vertical, style.getSubStyle("scrollbarStyle"));
		scrollBar->setScrollPane(*scrollPane);
		scrollBar->setAlwaysShow(false);

		dropdownWindow = std::make_shared<UIImage>(style.getSprite(openState == OpenState::OpenDown ? "background" : "backgroundUp"), UISizer(UISizerType::Horizontal), style.getBorder("innerBorder"));
		dropdownWindow->add(scrollPane, 1);
		dropdownWindow->add(scrollBar);
		dropdownWindow->setMinSize(Vector2f(getSize().x, getSize().y));
		addChild(dropdownWindow);

		dropdownList->setHandle(UIEventType::ListAccept, [=] (const UIEvent& event)
		{
			setSelectedOption(event.getIntData());
			close();
		});

		dropdownList->setHandle(UIEventType::ListCancel, [=] (const UIEvent& event)
		{
			close();
		});

		dropdownList->setHandle(UIEventType::ListHoveredChanged, [=] (const UIEvent& event)
		{
			const int idx = event.getIntData();
			const auto clampedIdx = clamp(idx, 0, static_cast<int>(options.size()) - 1);
			if (idx == -1) {
				sendEvent(UIEvent(UIEventType::DropdownHoveredChanged, getId(), getSelectedOptionId(), getSelectedOption()));
			} else {
				sendEvent(UIEvent(UIEventType::DropdownHoveredChanged, getId(), options.at(clampedIdx).id, idx));
			}

			if (notifyOnHover && canNotifyAsDropdown()) {
				if (getDataBindFormat() == UIDataBind::Format::String) {
					notifyDataBind(idx == -1 ? getSelectedOptionId() : options.at(clampedIdx).id);
				} else {
					notifyDataBind(idx == -1 ? getSelectedOption() : idx);
				}
			}
		});

		sendEvent(UIEvent(UIEventType::DropdownOpened, getId(), getSelectedOptionId(), curOption));

		forceLayout();
		auto sz = dropdownList->getSize();
		scrollPane->setScrollSpeed(ceil(2 * sz.y / options.size()));
		scrollPane->update(0, false);

		playSound(style.getString("openSound"));
	}
}

void UIDropdown::close()
{
	if (openState != OpenState::Closed) {
		openState = OpenState::Closed;

		scrollPane->destroy();
		scrollPane.reset();
		dropdownList->destroy();
		dropdownList.reset();
		dropdownWindow->destroy();
		dropdownWindow.reset();

		sendEvent(UIEvent(UIEventType::DropdownHoveredChanged, getId(), getSelectedOptionId(), curOption));
		sendEvent(UIEvent(UIEventType::DropdownClosed, getId(), getSelectedOptionId(), curOption));
		playSound(styles.at(0).getString("closeSound"));

		focus();
	}
}
