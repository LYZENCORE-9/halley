#include "halley/ui/behaviours/ui_fade_behaviour.h"
#include "halley/ui/ui_widget.h"
using namespace Halley;

UIFadeBehaviour::UIFadeBehaviour(Time delay, Time length, InterpolationCurve curve)
	: delay(delay)
	, length(length)
	, curve(std::move(curve))
{
}

void UIFadeBehaviour::init()
{
	getWidget()->setDynamicValue("alpha", ConfigNode(0.0f));
}

void UIFadeBehaviour::update(Time time)
{
	curTime += time;
	const auto t = curve.evaluate(static_cast<float>(clamp((curTime - delay) / std::max(length, 0.01), 0.0, 1.0)));
	getWidget()->setDynamicValue("alpha", ConfigNode(t));
}

bool UIFadeBehaviour::isAlive() const
{
	return curTime <= delay + length;
}

void UIFadeBehaviour::restart()
{
	curTime = 0;
}
