#ifndef SEQUENCEDIALOGPARAMETERTAB_H
#define SEQUENCEDIALOGPARAMETERTAB_H

#include <QComboBox>
#include <QList>
#include <QSpinBox>

#include "project/item/sequence/sequence.h"
#include "sequencepreset.h"

OLIVE_NAMESPACE_ENTER

class SequenceDialogParameterTab : public QWidget
{
  Q_OBJECT
public:
  SequenceDialogParameterTab(Sequence* sequence, QWidget* parent = nullptr);

  int GetSelectedVideoWidth() const;

  int GetSelectedVideoHeight() const;

  const rational& GetSelectedVideoFrameRate() const;

  int GetSelectedAudioSampleRate() const;

  uint64_t GetSelectedAudioChannelLayout() const;

public slots:
  void PresetChanged(const SequencePreset& preset);

signals:
  void SaveParametersAsPreset(const SequencePreset& preset);

private:
  QSpinBox* video_width_field_;

  QSpinBox* video_height_field_;

  QComboBox* video_frame_rate_field_;

  QComboBox* audio_sample_rate_field_;

  QComboBox* audio_channels_field_;

  QList<rational> frame_rate_list_;

  QList<int> sample_rate_list_;

  QList<uint64_t> channel_layout_list_;

private slots:
  void SavePresetClicked();

};

OLIVE_NAMESPACE_EXIT

#endif // SEQUENCEDIALOGPARAMETERTAB_H
