#ifndef EXPORTVIDEOTAB_H
#define EXPORTVIDEOTAB_H

#include <QComboBox>
#include <QWidget>

class ExportVideoTab : public QWidget
{
public:
  ExportVideoTab(QWidget* parent = nullptr);

  QComboBox* codec_combobox() const;

private:
  QWidget* SetupResolutionSection();
  QWidget* SetupColorSection();
  QWidget* SetupCodecSection();

  QComboBox* codec_combobox_;
  QComboBox* frame_rate_combobox_;
};

#endif // EXPORTVIDEOTAB_H
