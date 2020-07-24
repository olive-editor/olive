#ifndef EXPORTADVANCEDVIDEODIALOG_H
#define EXPORTADVANCEDVIDEODIALOG_H

#include <QComboBox>
#include <QDialog>

#include "widget/slider/integerslider.h"

OLIVE_NAMESPACE_ENTER

class ExportAdvancedVideoDialog : public QDialog
{
  Q_OBJECT
public:
  ExportAdvancedVideoDialog(const QList<QString>& pix_fmts,
                            QWidget* parent = nullptr);

  int threads() const
  {
    return static_cast<int>(thread_slider_->GetValue());
  }

  void set_threads(int t)
  {
    thread_slider_->SetValue(t);
  }

  QString pix_fmt() const
  {
    return pixel_format_combobox_->currentText();
  }

  void set_pix_fmt(const QString& s)
  {
    pixel_format_combobox_->setCurrentText(s);
  }

private:
  IntegerSlider* thread_slider_;

  QComboBox* pixel_format_combobox_;

};

OLIVE_NAMESPACE_EXIT

#endif // EXPORTADVANCEDVIDEODIALOG_H
