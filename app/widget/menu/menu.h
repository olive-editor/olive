#ifndef WIDGETMENU_H
#define WIDGETMENU_H

#include <QMenuBar>
#include <QMenu>

class Menu : public QMenu
{
public:
  Menu(QMenuBar* bar, const QObject* receiver = nullptr, const char* member = nullptr);
  Menu(Menu* bar, const QObject* receiver = nullptr, const char* member = nullptr);

  QAction* AddItem(const QString& id,
                   const QObject* receiver,
                   const char* member,
                   const QString &key = QString());

  static QAction* CreateItem(QObject* parent,
                             const QString& id,
                             const QObject* receiver,
                             const char* member,
                             const QString& key = QString());

  static void SetBooleanAction(QAction* a, bool *boolean);

private:
  void SetStyling();
};

#endif // WIDGETMENU_H
