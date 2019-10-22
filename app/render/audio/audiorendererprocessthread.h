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

#ifndef AUDIORENDERERPROCESSTHREAD_H
#define AUDIORENDERERPROCESSTHREAD_H

#include "audiorendererthreadbase.h"

class AudioRendererProcessor;

class AudioRendererProcessThread : public AudioRendererThreadBase
{
  Q_OBJECT
public:
  AudioRendererProcessThread(AudioRendererProcessor* parent,
                             const AudioRenderingParams &params);

  bool Queue(const NodeDependency &dep, bool wait, bool sibling);

public slots:
  virtual void Cancel() override;

protected:
  virtual void ProcessLoop() override;

signals:
  void RequestSibling(NodeDependency dep);

  void CachedSamples(const QByteArray& samples, const rational& in, const rational& out);

private:
  AudioRendererProcessor* parent_;

  NodeDependency path_;

  QAtomicInt cancelled_;

  bool sibling_;

};

using AudioRendererProcessThreadPtr = std::shared_ptr<AudioRendererProcessThread>;

#endif // AUDIORENDERERPROCESSTHREAD_H
