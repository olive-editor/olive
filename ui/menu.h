#ifndef MENU_H
#define MENU_H

#include <QMenu>

class Menu : public QMenu
{
public:
  Menu(QWidget* parent = nullptr);
  Menu(const QString &title, QWidget *parent = nullptr);
};

#endif // MENU_H
