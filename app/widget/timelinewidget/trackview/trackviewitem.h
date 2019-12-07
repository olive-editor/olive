#ifndef TRACKVIEWITEM_H
#define TRACKVIEWITEM_H

#include <QLabel>
#include <QPushButton>
#include <QWidget>

class TrackViewItem : public QWidget
{
  Q_OBJECT
public:
  TrackViewItem(Qt::Alignment alignment = Qt::AlignTop,
                QWidget* parent = nullptr);

private:
  QPushButton* CreateMSLButton(const QString &text, const QColor &checked_color) const;

  Qt::Alignment alignment_;

  QLabel* label_;

  QPushButton* mute_button_;
  QPushButton* solo_button_;
  QPushButton* lock_button_;

};

#endif // TRACKVIEWITEM_H
