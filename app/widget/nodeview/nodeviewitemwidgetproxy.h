#ifndef NODEVIEWITEMWIDGETPROXY_H
#define NODEVIEWITEMWIDGETPROXY_H

#include <QWidget>

/**
 * @brief A proxy object to allow NodeViewItem access to CSS functions
 *
 * QGraphicsItems can't take Q_PROPERTYs for CSS stylesheet input, but QWidgets can. This is a hack to allow CSS
 * properties to be set from CSS and then read by NodeViewItem.
 */
class NodeViewItemWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(QColor titlebarColor READ TitleBarColor WRITE SetTitleBarColor DESIGNABLE true)
  Q_PROPERTY(QColor borderColor READ BorderColor WRITE SetBorderColor DESIGNABLE true)
public:
  NodeViewItemWidget();

  QColor TitleBarColor();
  void SetTitleBarColor(QColor color);

  QColor BorderColor();
  void SetBorderColor(QColor color);
private:
  QColor title_bar_color_;
  QColor border_color_;
};

#endif // NODEVIEWITEMWIDGETPROXY_H
