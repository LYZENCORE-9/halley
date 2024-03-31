/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#include "input_device.h"
#include <halley/support/exception.h>

namespace Halley {
	class InputButtonBase : public InputDevice {
	public:
		InputButtonBase(int nButtons = -1);

		size_t getNumberButtons() override { return buttonDown.size(); }

		bool isAnyButtonPressed() override;
		bool isAnyButtonPressedRepeat() override;
		bool isAnyButtonReleased() override;
		bool isAnyButtonDown() override;

		bool isButtonPressed(InputButton code) override;
		bool isButtonPressedRepeat(InputButton code) override;
		bool isButtonReleased(InputButton code) override;
		bool isButtonDown(InputButton code) override;

		bool isButtonPressed(KeyCode code);
		bool isButtonPressedRepeat(KeyCode code);
		bool isButtonReleased(KeyCode code);
		bool isButtonDown(KeyCode code);

		void clearButton(InputButton code) override;
		void clearButtonPress(InputButton code) override;
		void clearButtonRelease(InputButton code) override;

		String getButtonName(int code) const override;

		void clearPresses() override;

		void onButtonStatus(int code, bool down);

		void setParent(const std::shared_ptr<InputDevice>& parent) override;
		std::shared_ptr<InputDevice> getParent() const override;

	protected:
		Vector<char> buttonPressed;
		Vector<char> buttonPressedRepeat;
		Vector<char> buttonReleased;
		Vector<char> buttonDown;
		bool anyButtonPressed = false;
		bool anyButtonPressedRepeat = false;
		bool anyButtonReleased = false;
		std::weak_ptr<InputDevice> parent;

		void init(int nButtons);

		void onButtonPressed(int code);
		void onButtonReleased(int code);
		virtual void onButtonsCleared();
	};

	typedef std::shared_ptr<InputButtonBase> spInputButtonBase;
}
