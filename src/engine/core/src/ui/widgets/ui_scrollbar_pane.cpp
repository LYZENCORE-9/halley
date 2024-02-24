#include "halley/ui/widgets/ui_scrollbar_pane.h"

using namespace Halley;

UISizer makeSizer(bool scrollHorizontal, bool scrollVertical)
{
	if (scrollHorizontal && scrollVertical) {
		auto sizer = UISizer(UISizerType::Grid, 1, 2);
		sizer.setColumnProportions({{ 1.0f, 0.0f }});
		sizer.setRowProportions({{ 1.0f, 0.0f }});
		return sizer;
	} else if (scrollHorizontal) {
		return UISizer(UISizerType::Vertical);
	} else {
		return UISizer(UISizerType::Horizontal);
	}
}

UIScrollBarPane::UIScrollBarPane(String id, Vector2f clipSize, UIStyle style, UISizer&& sizer, bool scrollHorizontal, bool scrollVertical, bool alwaysShow, Vector2f minSize, float scrollSpeed, bool smoothGoTo)
	: UIWidget(id, minSize, makeSizer(scrollHorizontal, scrollVertical))
{
	styles.emplace_back(style);
	pane = std::make_shared<UIScrollPane>(id + "_pane", clipSize, std::move(sizer), scrollHorizontal, scrollVertical, scrollSpeed, smoothGoTo);
	UIWidget::add(pane, 1);

	if (scrollVertical) {
		vBar = std::make_shared<UIScrollBar>(id + "_vbar", UIScrollDirection::Vertical, style, alwaysShow);
		vBar->setScrollPane(*pane);
		UIWidget::add(vBar, 0, style.getBorder("verticalBorder", Vector4f()));
	}
	if (scrollHorizontal) {
		hBar = std::make_shared<UIScrollBar>(id + "_hbar", UIScrollDirection::Horizontal, style, alwaysShow);
		hBar->setScrollPane(*pane);
		UIWidget::add(hBar, 0, style.getBorder("horizontalBorder", Vector4f()));
	}
}

std::shared_ptr<UIScrollPane> UIScrollBarPane::getPane() const
{
	return pane;
}

void UIScrollBarPane::add(std::shared_ptr<IUIElement> element, float proportion, Vector4f border, int fillFlags, size_t insertPos)
{
	pane->add(element, proportion, border, fillFlags, insertPos);
}

void UIScrollBarPane::addSpacer(float size)
{
	pane->addSpacer(size);
}

void UIScrollBarPane::addStretchSpacer(float proportion)
{
	pane->addStretchSpacer(proportion);
}

void UIScrollBarPane::setAlwaysShow(bool alwaysShow)
{
	if (hBar) {
		hBar->setAlwaysShow(alwaysShow);
	}
	if (vBar) {
		vBar->setAlwaysShow(alwaysShow);
	}
}
