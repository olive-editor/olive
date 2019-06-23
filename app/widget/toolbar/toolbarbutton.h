#ifndef TOOLBARBUTTON_H
#define TOOLBARBUTTON_H

#include <QPushButton>

#include "tool/tool.h"

class ToolbarButton : public QPushButton
{
public:
  ToolbarButton(QWidget* parent, const QIcon& icon, const olive::tool::Tool& tool);

  const olive::tool::Tool& tool();
private:
  olive::tool::Tool tool_;
};

#endif // TOOLBARBUTTON_H
