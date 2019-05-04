#include "waveform.h"

#include <QtMath>

#include "global/config.h"

int ConvertSampleToHeight(qint8 signed_sample, int channel_height) {
  int half_channel_height = channel_height >> 1;
  return (half_channel_height) + qRound(((double(signed_sample) / 128.0)) * half_channel_height);
}

void olive::ui::DrawWaveform(Clip* clip,
                              const FootageStream* ms,
                              long media_length,
                              QPainter *p,
                              const QRect& clip_rect,
                              int waveform_start,
                              int waveform_limit,
                              double zoom) {

  int divider = ms->audio_channels;

  int channel_height = clip_rect.height()/ms->audio_channels;

  int last_waveform_index = -1;

  for (int i=waveform_start;i<waveform_limit;i++) {
    int waveform_index = qFloor((((clip->clip_in() + (double(i)/zoom))/media_length) * ms->audio_preview.size())/divider)*divider;

    if (clip->reversed()) {
      waveform_index = ms->audio_preview.size() - waveform_index - (ms->audio_channels * 2);
    }

    if (last_waveform_index < 0) last_waveform_index = waveform_index;

    for (int j=0;j<ms->audio_channels;j++) {
      int bottom = clip_rect.top()+channel_height*(j+1);

      int offset_range_start = last_waveform_index+(j*2);
      int offset_range_end = waveform_index+(j*2);
      int offset_range_min = qMin(offset_range_start, offset_range_end);
      int offset_range_max = qMax(offset_range_start, offset_range_end);

      // Break if we're about to draw from an index that doesn't exist
      if (offset_range_min+1 >= ms->audio_preview.size()) {
        break;
      }

      int min = ConvertSampleToHeight(ms->audio_preview.at(offset_range_min), channel_height);
      int max = ConvertSampleToHeight(ms->audio_preview.at(offset_range_min+1), channel_height);

      if ((offset_range_max + 1) < ms->audio_preview.size()) {

        // for waveform drawings, we get the maximum below 0 and maximum above 0 for this waveform range
        for (int k=offset_range_min+2;k<=offset_range_max;k+=2) {
          min = ConvertSampleToHeight(ms->audio_preview.at(k), channel_height);
          max = ConvertSampleToHeight(ms->audio_preview.at(k+1), channel_height);
        }

        // draw waveforms
        if (olive::config.rectified_waveforms) {
          p->drawLine(clip_rect.left()+i, bottom, clip_rect.left()+i, bottom - (max - min));
        } else {
          p->drawLine(clip_rect.left()+i, bottom - min, clip_rect.left()+i, bottom - max);
        }
      }
    }
    last_waveform_index = waveform_index;
  }
}
