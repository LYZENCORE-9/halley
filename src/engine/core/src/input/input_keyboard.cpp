#include <halley/input/input_keyboard.h>
#include "halley/input/input_keys.h"
#include "halley/input/text_input_capture.h"
#include "halley/input/text_input_data.h"
using namespace Halley;

KeyboardKeyPress::KeyboardKeyPress(KeyCode key, KeyMods mod)
	: key(key)
	, mod(mod)
{
}

bool KeyboardKeyPress::operator==(const KeyboardKeyPress& other) const
{
	return key == other.key && mod == other.mod;
}

bool KeyboardKeyPress::is(KeyCode key, KeyMods mod) const
{
	return this->key == key && this->mod == mod;
}

bool KeyboardKeyPress::isPrintable() const
{
	const auto code = static_cast<int>(key);
	return ((code >= 32 && code < 128) || code == '\n') && (mod == KeyMods::None || mod == KeyMods::Shift);
}

InputKeyboard::InputKeyboard(int nButtons, std::shared_ptr<IClipboard> clipboard)
	: InputButtonBase(nButtons)
	, clipboard(std::move(clipboard))
{
}

TextInputCapture InputKeyboard::captureText(TextInputData& textInputData, SoftwareKeyboardData data)
{
	return TextInputCapture(textInputData, std::move(data), makeTextInputCapture());
}

void InputKeyboard::onKeyPressed(KeyCode code, KeyMods mods)
{
	const auto key = KeyboardKeyPress(code, mods);
	
	if (!sendKeyPress(key)) {
		keyPresses.push_back(key);
		onButtonPressed(static_cast<int>(code));
	}
}

void InputKeyboard::onKeyReleased(KeyCode code, KeyMods mods)
{
	onButtonReleased(static_cast<int>(code));
}

KeyMods InputKeyboard::getKeyMods()
{
	uint8_t result = 0;

	if (isButtonDown(KeyCode::LCtrl) || isButtonDown(KeyCode::RCtrl)) {
		result |= static_cast<uint8_t>(KeyMods::Ctrl);
	}
	if (isButtonDown(KeyCode::LAlt) || isButtonDown(KeyCode::RAlt)) {
		result |= static_cast<uint8_t>(KeyMods::Alt);
	}
	if (isButtonDown(KeyCode::LShift) || isButtonDown(KeyCode::RShift)) {
		result |= static_cast<uint8_t>(KeyMods::Shift);
	}
	if (isButtonDown(KeyCode::LMod) || isButtonDown(KeyCode::RMod)) {
		result |= static_cast<uint8_t>(KeyMods::Mod);
	}

	return static_cast<KeyMods>(result);
}

std::string_view InputKeyboard::getName() const
{
	return "Keyboard";
}

gsl::span<const KeyboardKeyPress> InputKeyboard::getPendingKeys() const
{
	return { keyPresses };
}

void InputKeyboard::onTextEntered(const char* text)
{
	const auto str = String(text).getUTF32();

	if (str.size() == 1 && str[0] == ' ' && (isButtonDown(KeyCode::LCtrl) || isButtonDown(KeyCode::RCtrl))) {
		// Workaround SDL bug
		return;
	}

	for (const auto& c: captures) {
		c->onTextEntered(str);
	}
}

bool InputKeyboard::sendKeyPress(KeyboardKeyPress chr)
{
	for (const auto& c: captures) {
		const bool handled = c->onKeyPress(chr, clipboard.get());
		if (handled) {
			return true;
		}
	}

	return false;
}

void InputKeyboard::onButtonsCleared()
{
	keyPresses.clear();
}

std::unique_ptr<ITextInputCapture> InputKeyboard::makeTextInputCapture()
{
	auto ptr = std::make_unique<StandardTextInputCapture>(*this);
	addCapture(ptr.get());
	return ptr;
}

void InputKeyboard::addCapture(ITextInputCapture* capture)
{
	captures.insert(capture);
}

void InputKeyboard::removeCapture(ITextInputCapture* capture)
{
	captures.erase(capture);
}

InputType InputKeyboard::getInputType() const
{
	return InputType::Keyboard;
}
