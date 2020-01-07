#ifndef TEMPOPROCESSOR_H
#define TEMPOPROCESSOR_H

#include <inttypes.h>

extern "C" {
#include <libavfilter/avfilter.h>
}

#include "render/audioparams.h"

class TempoProcessor
{
public:
  TempoProcessor();

  bool IsOpen() const;

  const double& GetSpeed() const;

  bool Open(const AudioRenderingParams& params, const double &speed);

  void Push(const char *data, int length);

  int Pull(char* data, int max_length);

  void Close();

private:
  static AVFilterContext* CreateTempoFilter(AVFilterGraph *graph, AVFilterContext *link, const double& tempo);

  AVFilterGraph* filter_graph_;

  AVFilterContext* buffersrc_ctx_;

  AVFilterContext* buffersink_ctx_;

  AVFrame* processed_frame_;
  int processed_frame_byte_index_;
  int processed_frame_max_bytes_;

  AudioRenderingParams params_;

  int64_t timestamp_;

  double speed_;

  bool open_;

  bool flushed_;
};

#endif // TEMPOPROCESSOR_H
