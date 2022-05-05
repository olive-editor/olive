/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#ifndef AUDIOPROCESSOR_H
#define AUDIOPROCESSOR_H

#include <inttypes.h>

extern "C" {
#include <libavfilter/avfilter.h>
}

#include "render/audioparams.h"

namespace olive {

class AudioProcessor
{
public:
  AudioProcessor();

  ~AudioProcessor();

  DISABLE_COPY_MOVE(AudioProcessor)

  bool Open(const AudioParams &from, const AudioParams &to, double tempo = 1.0);

  void Close();

  bool IsOpen() const { return filter_graph_; }

  using Buffer = QVector<QByteArray>;
  int Convert(float **in, int nb_in_samples, AudioProcessor::Buffer *output);

  void Flush();

private:
  static AVFilterContext* CreateTempoFilter(AVFilterGraph *graph, AVFilterContext *link, const double& tempo);

  AVFilterGraph* filter_graph_;

  AVFilterContext* buffersrc_ctx_;

  AVFilterContext* buffersink_ctx_;

  AudioParams from_;
  AVSampleFormat from_fmt_;

  AudioParams to_;
  AVSampleFormat to_fmt_;

  AVFrame *in_frame_;

  AVFrame *out_frame_;

};

}

#endif // AUDIOPROCESSOR_H
