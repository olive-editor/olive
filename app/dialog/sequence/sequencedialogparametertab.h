#ifndef SEQUENCEDIALOGPARAMETERTAB_H
#define SEQUENCEDIALOGPARAMETERTAB_H

#include <QComboBox>
#include <QList>
#include <QSpinBox>

#include "project/item/sequence/sequence.h"
#include "sequencepreset.h"
#include "widget/slider/integerslider.h"
#include "widget/standardcombos/standardcombos.h"

OLIVE_NAMESPACE_ENTER

class SequenceDialogParameterTab : public QWidget
{
  Q_OBJECT
public:
  SequenceDialogParameterTab(Sequence* sequence, QWidget* parent = nullptr);

  int GetSelectedVideoWidth() const
  {
    return video_width_field_->GetValue();
  }

  int GetSelectedVideoHeight() const
  {
    return video_height_field_->GetValue();
  }

  rational GetSelectedVideoFrameRate() const
  {
    return video_frame_rate_field_->GetFrameRate();
  }

  rational GetSelectedVideoPixelAspect() const
  {
    return video_pixel_aspect_field_->GetPixelAspectRatio();
  }

  VideoParams::Interlacing GetSelectedVideoInterlacingMode() const
  {
    return video_interlaced_field_->GetInterlaceMode();
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

  PixelFormat::Format GetSelectedPreviewFormat() const
  {
    return preview_format_field_->GetPixelFormat();
  }

public slots:
  void PresetChanged(const SequencePreset& preset);

signals:
  void SaveParametersAsPreset(const SequencePreset& preset);

private:
  IntegerSlider* video_width_field_;

  IntegerSlider* video_height_field_;

  FrameRateComboBox* video_frame_rate_field_;

  PixelAspectRatioComboBox* video_pixel_aspect_field_;

  InterlacedComboBox* video_interlaced_field_;

  SampleRateComboBox* audio_sample_rate_field_;

  ChannelLayoutComboBox* audio_channels_field_;

  VideoDividerComboBox* preview_resolution_field_;

  QLabel* preview_resolution_label_;

  PixelFormatComboBox* preview_format_field_;

private slots:
  void SavePresetClicked();

  void UpdatePreviewResolutionLabel();

};

OLIVE_NAMESPACE_EXIT

#endif // SEQUENCEDIALOGPARAMETERTAB_H
