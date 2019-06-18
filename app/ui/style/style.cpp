#include "style.h"

#include <QApplication>
#include <QPalette>
#include <QStyleFactory>

void olive::style::AppSetDefault()
{
  qApp->setStyle(QStyleFactory::create("Fusion"));

  // Set palette for Fusion
  QPalette palette;

  palette.setColor(QPalette::Window, QColor(53,53,53));
  palette.setColor(QPalette::WindowText, Qt::white);
  palette.setColor(QPalette::Base, QColor(25,25,25));
  palette.setColor(QPalette::AlternateBase, QColor(53,53,53));
  palette.setColor(QPalette::ToolTipBase, QColor(25,25,25));
  palette.setColor(QPalette::ToolTipText, Qt::white);
  palette.setColor(QPalette::Text, Qt::white);
  palette.setColor(QPalette::Button, QColor(53,53,53));
  palette.setColor(QPalette::ButtonText, Qt::white);
  palette.setColor(QPalette::BrightText, Qt::red);
  palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
  palette.setColor(QPalette::Link, QColor(42, 130, 218));
  palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
  palette.setColor(QPalette::HighlightedText, Qt::white);

  qApp->setPalette(palette);

  // set default CSS
  QString stylesheet = "QPushButton::checked { background: rgb(25, 25, 25); }";

  // Windows menus have the option of being native, so we may not need this CSS
  bool set_separator_color = false;
#ifndef Q_OS_WIN
  //set_separator_color = !olive::config.use_native_menu_styling;
  set_separator_color = true;
#endif
  if (set_separator_color) {
    stylesheet.append("QMenu::separator { background: #303030; }");
  }

  qApp->setStyleSheet(stylesheet);
}

void olive::style::WidgetSetNative(QWidget *w)
{
#ifdef Q_OS_WIN
  w->setStyleSheet("");
  w->setPalette(w->style()->standardPalette());
  w->setStyle(QStyleFactory::create("windowsvista"));
#else
  Q_UNUSED(w)
#endif
}
