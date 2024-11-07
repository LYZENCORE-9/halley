#include "halley/diagnostics/stats_view.h"


#include "halley/api/core_api.h"
#include "halley/api/halley_api.h"
#include "halley/graphics/camera.h"
#include "halley/graphics/render_context.h"
#include "halley/graphics/render_target/render_target.h"

using namespace Halley;

ScreenOverlay::ScreenOverlay()
{
	screenSize = Vector2f(1280, 720);
}

void ScreenOverlay::draw(RenderContext& context)
{
	const auto viewPort = Rect4f(context.getDefaultRenderTarget().getViewPort());
	screenSize = viewPort.getSize();

	const auto camera = Camera(screenSize * 0.5f);

	context.with(camera).bind([&](Painter& painter) {
		paint(painter);
	});
}

void ScreenOverlay::update(Time t)
{
}

Vector2f ScreenOverlay::getScreenSize() const
{
	return screenSize;
}


StatsView::StatsView(Resources& resources, const HalleyAPI& api)
	: resources(resources)
	, api(api)
{}

void StatsView::update(Time t)
{
	if (input) {
		input->setEnabled(isInputActive());
	}
}

void StatsView::draw(RenderContext& context)
{
	ScreenOverlay::draw(context);
}

void StatsView::setActive(bool active)
{
	this->active = active;
	if (input) {
		input->setEnabled(isInputActive());
	}
}

bool StatsView::isActive() const
{
	return active;
}

void StatsView::setWorld(const World* world)
{
	this->world = world;
}

void StatsView::setInput(std::shared_ptr<InputVirtual> input, const StatsViewControls& controls)
{
	Vector<int> axes = { controls.xAxis, controls.yAxis };
	Vector<int> buttons = { controls.accept, controls.cancel, controls.prevTab, controls.nextTab };
	this->input = std::make_shared<InputExclusive>(input, InputPriority::Maximum, axes, buttons);
	this->input->setEnabled(isInputActive());
}

String StatsView::formatTime(int64_t ns) const
{
	const int64_t us = (ns + 500) / 1000;
	return toString(us / 1000) + "." + toString(us % 1000, 10, 3);
}

bool StatsView::isInputActive() const
{
	return isActive();
}
