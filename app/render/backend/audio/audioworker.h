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

#ifndef AUDIOWORKER_H
#define AUDIOWORKER_H

#include "../audiorenderworker.h"

OLIVE_NAMESPACE_ENTER

class AudioWorker : public AudioRenderWorker
{
public:
  AudioWorker(DecoderCache* decoder_cache, QHash<Node*, Node*>* copy_map, QObject* parent = nullptr);

protected:
  virtual void FrameToValue(DecoderPtr decoder, StreamPtr stream, const TimeRange &range, NodeValueTable* table) override;

  virtual void RunNodeAccelerated(const Node *node, const TimeRange& range, const NodeValueDatabase& input_params, NodeValueTable* output_params) override;

private:

};

OLIVE_NAMESPACE_EXIT

#endif // AUDIOWORKER_H
