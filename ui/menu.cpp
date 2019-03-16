#include "menu.h"

#include "global/global.h"
#include "global/config.h"

Menu::Menu(QWidget *parent) :
  QMenu(parent)
{
  if (olive::CurrentConfig.use_native_menu_styling) {
    OliveGlobal::SetNativeStyling(this);
  }
}

Menu::Menu(const QString &title, QWidget *parent) :
  QMenu(title, parent)
{
  if (olive::CurrentConfig.use_native_menu_styling) {
    OliveGlobal::SetNativeStyling(this);
  }
}
