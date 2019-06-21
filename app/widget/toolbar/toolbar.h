#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QPushButton>

class Toolbar : public QWidget
{
public:
  Toolbar(QWidget* parent);

protected:
  virtual void changeEvent(QEvent* e) override;

private:
  void Retranslate();

  QPushButton* btn_pointer_tool;
  QPushButton* btn_edit_tool;
  QPushButton* btn_ripple_tool;
  QPushButton* btn_razor_tool;
  QPushButton* btn_slip_tool;
  QPushButton* btn_slide_tool;
  QPushButton* btn_hand_tool;
  QPushButton* btn_transition_tool;
  QPushButton* btn_snapping_toggle;
  QPushButton* btn_zoom_in;
  QPushButton* btn_zoom_out;
  QPushButton* btn_record;
  QPushButton* btn_add;
};

#endif // TOOLBAR_H
