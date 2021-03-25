#ifndef SEQUENCEDIALOGPARAMETERTAB_H
#define SEQUENCEDIALOGPARAMETERTAB_H

#include <QComboBox>
#include <QList>
#include <QSpinBox>

#include "project/item/sequence/sequence.h"
#include "sequencepreset.h"
#include "widget/slider/integerslider.h"
#include "widget/standardcombos/standardcombos.h"
#include "widget/videoparamedit/videoparamedit.h"

namespace olive {

class SequenceDialogParameterTab : public QWidget
{
  Q_OBJECT
public:
  SequenceDialogParameterTab(Sequence* sequence, QWidget* parent = nullptr);

  int GetSelectedVideoWidth() const
  {
    return video_section_->GetWidth();
  }

  int GetSelectedVideoHeight() const
  {
    return video_section_->GetHeight();
  }

  rational GetSelectedVideoFrameRate() const
  {
    return video_section_->GetFrameRate();
  }

  rational GetSelectedVideoPixelAspect() const
  {
    return video_section_->GetPixelAspectRatio();
  }

  VideoParams::Interlacing GetSelectedVideoInterlacingMode() const
  {
    return video_section_->GetInterlaceMode();
  }

  int GetSelectedAudioSampleRate() const
  {
    return audio_sample_rate_field_->GetSampleRate();
  }

  uint64_t GetSelectedAudioChannelLayout() const
  {
    return audio_channels_field_->GetChannelLayout();
  }

  int GetSelectedPreviewResolution() const
  {
    return preview_resolution_field_->GetDivider();
  }

  VideoParams::Format GetSelectedPreviewFormat() const
  {
    return preview_format_field_->GetPixelFormat();
  }

public slots:
  void PresetChanged(const SequencePreset& preset);

signals:
  void SaveParametersAsPreset(const SequencePreset& preset);

private:
  VideoParamEdit* video_section_;

  SampleRateComboBox* audio_sample_rate_field_;

  ChannelLayoutComboBox* audio_channels_field_;

  VideoDividerComboBox* preview_resolution_field_;

  QLabel* preview_resolution_label_;

  PixelFormatComboBox* preview_format_field_;

private slots:
  void SavePresetClicked();

  void UpdatePreviewResolutionLabel();

};

}

#endif // SEQUENCEDIALOGPARAMETERTAB_H
