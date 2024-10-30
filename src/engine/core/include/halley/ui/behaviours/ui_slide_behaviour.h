#pragma once

#include "../ui_behaviour.h"
#include "halley/maths/interpolation_curve.h"

namespace Halley {
	class UIAnchor;

	class UISlideBehaviour final : public UIBehaviour {
	public:
		UISlideBehaviour(Time delay, Time length, Vector2f offset, InterpolationCurve curve);

		void init() override;
		void update(Time time) override;

		bool isAlive() const override;

	private:
		Time delay;
		Time length;
		Vector2f offset;
		InterpolationCurve curve;

		Time curTime = 0;
	};
}
