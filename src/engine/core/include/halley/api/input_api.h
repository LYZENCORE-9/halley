#pragma once

#include <functional>
#include "halley/maths/vector2.h"
#include "halley/input/input_device.h"
#include "halley/maths/colour.h"
#include "halley/data_structures/maybe.h"

namespace Halley
{
	class InputJoystick;
	class InputKeyboard;
	class InputTouch;

	class InputControllerData {
	public:
		Colour colour;
		String name;
	};

	enum class MouseCursorMode {
		Arrow,
		IBeam,
		Wait,
		Crosshair,
		WaitArrow,
		SizeNWSE,
		SizeNESW,
		SizeWE,
		SizeNS,
		SizeAll,
		No,
		Hand
	};

	class InputAPI
	{
	public:
		virtual ~InputAPI() {}
		
		virtual size_t getNumberOfKeyboards() const = 0;
		virtual std::shared_ptr<InputKeyboard> getKeyboard(int id = 0) const = 0;

		virtual size_t getNumberOfJoysticks() const = 0;
		virtual std::shared_ptr<InputDevice> getJoystick(int id = 0) const = 0;

		virtual size_t getNumberOfMice() const = 0;
		virtual std::shared_ptr<InputDevice> getMouse(int id = 0) const = 0;

		virtual Vector<std::shared_ptr<InputTouch>> getNewTouchEvents() = 0;
		virtual Vector<std::shared_ptr<InputTouch>> getTouchEvents() = 0;

		virtual void setMouseTrap(bool shouldBeTrapped) {}
		virtual void setMouseCursorPos(Vector2i pos) {}
		virtual void setMouseCursorMode(std::optional<MouseCursorMode> mode) {}
		virtual void setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction) = 0;

		virtual Future<bool> requestControllerSetup(int minControllers, int maxControllers, std::optional<Vector<InputControllerData>> controllerData = {})
		{
			Promise<bool> promise;
			promise.setValue(true);
			return promise.getFuture();
		}
	};
}
