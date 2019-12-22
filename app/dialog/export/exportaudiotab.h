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
  QComboBox* channel_layout_combobox() const;

  void set_sample_rate(int rate);
  void set_channel_layout(uint64_t layout);

private:
  QComboBox* codec_combobox_;
  QComboBox* sample_rate_combobox_;
  QComboBox* channel_layout_combobox_;

  QList<int> sample_rates_;
  QList<uint64_t> channel_layouts_;
};

#endif // EXPORTAUDIOTAB_H
