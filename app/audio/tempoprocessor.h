/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef TEMPOPROCESSOR_H
#define TEMPOPROCESSOR_H

#ifdef __MINGW32__
#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO
#endif
#endif

#include <inttypes.h>

extern "C" {
#include <libavfilter/avfilter.h>
}

#include "render/audioparams.h"

OLIVE_NAMESPACE_ENTER

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

OLIVE_NAMESPACE_EXIT

#endif // TEMPOPROCESSOR_H
