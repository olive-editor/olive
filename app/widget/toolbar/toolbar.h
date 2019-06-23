#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QPushButton>

#include "widget/flowlayout/flowlayout.h"

class Toolbar : public QWidget
{
public:
  Toolbar(QWidget* parent);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();

  QPushButton *CreateToolButton(const QIcon& icon);

  FlowLayout* layout_;

  QPushButton* btn_pointer_tool_;
  QPushButton* btn_edit_tool_;
  QPushButton* btn_ripple_tool_;
  QPushButton* btn_razor_tool_;
  QPushButton* btn_slip_tool_;
  QPushButton* btn_slide_tool_;
  QPushButton* btn_hand_tool_;
  QPushButton* btn_transition_tool_;
  QPushButton* btn_snapping_toggle_;
  QPushButton* btn_zoom_tool_;
  QPushButton* btn_record_;
  QPushButton* btn_add_;
};

#endif // TOOLBAR_H
