#include "toolbarbutton.h"

ToolbarButton::ToolbarButton(QWidget *parent, const QIcon &icon, const olive::tool::Tool &tool) :
  QPushButton(parent),
  tool_(tool)
{
  setCheckable(true);
  setIcon(icon);
}

const olive::tool::Tool &ToolbarButton::tool()
{
  return tool_;
}
