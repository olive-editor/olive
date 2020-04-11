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

#ifndef AUDIORENDERWORKER_H
#define AUDIORENDERWORKER_H

#include "renderworker.h"

OLIVE_NAMESPACE_ENTER

class AudioRenderWorker : public RenderWorker
{
  Q_OBJECT
public:
  AudioRenderWorker(QHash<Node*, Node*>* copy_map, QObject* parent = nullptr);

  void SetParameters(const AudioRenderingParams& audio_params);

signals:
  void ConformUnavailable(StreamPtr stream, TimeRange range, rational stream_time, AudioRenderingParams params);

protected:
  virtual bool InitInternal() override;

  virtual void CloseInternal() override;

  virtual NodeValueTable RenderBlock(const TrackOutput *track, const TimeRange& range) override;

  const AudioRenderingParams& audio_params() const;

private:
  AudioRenderingParams audio_params_;

  QHash<Node*, Node*>* copy_map_;

};

OLIVE_NAMESPACE_EXIT

#endif // AUDIORENDERWORKER_H
