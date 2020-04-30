#ifndef EXPORTADVANCEDVIDEODIALOG_H
#define EXPORTADVANCEDVIDEODIALOG_H

#include <QDialog>

#include "render/backend/exportparams.h"
#include "widget/slider/integerslider.h"

OLIVE_NAMESPACE_ENTER

class ExportAdvancedVideoDialog : public QDialog
{
  Q_OBJECT
public:
  ExportAdvancedVideoDialog(QWidget* parent = nullptr);

  int threads() const;
  void set_threads(int t);

private:
  IntegerSlider* thread_slider_;

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTADVANCEDVIDEODIALOG_H
