#ifndef EXPORTAUDIOTAB_H
#define EXPORTAUDIOTAB_H

#include <QComboBox>
#include <QWidget>

class ExportAudioTab : public QWidget
{
public:
  ExportAudioTab(QWidget* parent = nullptr);

  QComboBox* codec_combobox() const;
  QComboBox* sample_rate_combobox() const;

private:
  QComboBox* codec_combobox_;
  QComboBox* sample_rate_combobox_;
};

#endif // EXPORTAUDIOTAB_H
