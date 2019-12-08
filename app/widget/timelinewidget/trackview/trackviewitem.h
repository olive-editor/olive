#ifndef TRACKVIEWITEM_H
#define TRACKVIEWITEM_H

#include <QPushButton>
#include <QStackedWidget>
#include <QWidget>

#include "widget/clickablelabel/clickablelabel.h"
#include "widget/focusablelineedit/focusablelineedit.h"

class TrackViewItem : public QWidget
{
  Q_OBJECT
public:
  TrackViewItem(const QString& name,
                Qt::Alignment alignment = Qt::AlignTop,
                QWidget* parent = nullptr);

signals:
  void NameChanged(const QString& name);

private:
  QPushButton* CreateMSLButton(const QString &text, const QColor &checked_color) const;

  Qt::Alignment alignment_;

  QStackedWidget* stack_;

  ClickableLabel* label_;
  FocusableLineEdit* line_edit_;

  QPushButton* mute_button_;
  QPushButton* solo_button_;
  QPushButton* lock_button_;

private slots:
  void LabelClicked();

  void LineEditConfirmed();

  void LineEditCancelled();

};

#endif // TRACKVIEWITEM_H
