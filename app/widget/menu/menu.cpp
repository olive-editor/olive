#include "menu.h"

#include "ui/style/style.h"

Menu::Menu(QMenuBar *bar, const QObject* receiver, const char* member) :
  QMenu(bar)
{
  SetStyling();

  bar->addMenu(this);

  if (receiver != nullptr) {
    connect(this, SIGNAL(aboutToShow()), receiver, member);
  }
}

Menu::Menu(Menu *menu, const QObject *receiver, const char *member)
{
  SetStyling();

  menu->addMenu(this);

  if (receiver != nullptr) {
    connect(this, SIGNAL(aboutToShow()), receiver, member);
  }
}

QAction *Menu::AddItem(const QString &id,
                       const QObject *receiver,
                       const char *member,
                       const QString &key)
{
  QAction* a = CreateItem(this, id, receiver, member, key);

  addAction(a);

  return a;
}

QAction *Menu::CreateItem(QObject* parent,
                          const QString &id,
                          const QObject *receiver,
                          const char *member,
                          const QString &key)
{
  QAction* a = new QAction(parent);

  a->setProperty("id", id);

  if (!key.isEmpty()) {
    a->setShortcut(key);
    a->setProperty("keydefault", key);
  }

  connect(a, SIGNAL(triggered(bool)), receiver, member);

  return a;
}

void Menu::SetStyling()
{
  //if (olive::config.use_native_menu_styling) {
    olive::style::WidgetSetNative(this);
  //}
}
