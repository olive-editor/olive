#include "nodeviewitemwidgetproxy.h"

NodeViewItemWidget::NodeViewItemWidget()
{
}

QColor NodeViewItemWidget::TitleBarColor()
{
  return title_bar_color_;
}

void NodeViewItemWidget::SetTitleBarColor(QColor color)
{
  title_bar_color_ = color;
}

QColor NodeViewItemWidget::BorderColor()
{
  return border_color_;
}

void NodeViewItemWidget::SetBorderColor(QColor color)
{
  border_color_ = color;
}
