#ifndef TRACKVIEWITEM_H
#define TRACKVIEWITEM_H

#include <QPushButton>
#include <QStackedWidget>
#include <QWidget>

#include "node/output/track/track.h"
#include "widget/clickablelabel/clickablelabel.h"
#include "widget/focusablelineedit/focusablelineedit.h"

class TrackViewItem : public QWidget
{
  Q_OBJECT
public:
  TrackViewItem(TrackOutput* track,
                QWidget* parent = nullptr);

private:
  QPushButton* CreateMSLButton(const QString &text, const QColor &checked_color) const;

  QStackedWidget* stack_;

  ClickableLabel* label_;
  FocusableLineEdit* line_edit_;

  QPushButton* mute_button_;
  QPushButton* solo_button_;
  QPushButton* lock_button_;

  TrackOutput* track_;

private slots:
  void LabelClicked();

  void LineEditConfirmed();

  void LineEditCancelled();

};

#endif // TRACKVIEWITEM_H
