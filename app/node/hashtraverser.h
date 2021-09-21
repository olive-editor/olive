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

#ifndef HASHTRAVERSER_H
#define HASHTRAVERSER_H

#include "traverser.h"

namespace olive {

class HashTraverser : public NodeTraverser
{
public:
  HashTraverser();

  QByteArray GetHash(const Node *node, const Node::ValueHint &hint, const VideoParams &params, const TimeRange &range);

protected:
  virtual TexturePtr ProcessVideoFootage(const FootageJob &stream, const rational &input_time) override;

  virtual SampleBufferPtr ProcessAudioFootage(const FootageJob &stream, const TimeRange &input_time);

  virtual TexturePtr ProcessShader(const Node *node, const TimeRange &range, const ShaderJob& job) override;

  virtual SampleBufferPtr ProcessSamples(const Node *node, const TimeRange &range, const SampleJob &job);

  virtual TexturePtr ProcessFrameGeneration(const Node *node, const GenerateJob& job) override;

private:
  void HashGenerateJob(const Node *node, const GenerateJob *job);

  void HashFootageJob();

  template <typename T>
  void Hash(T value);

  void Hash(const QByteArray &array);

  void Hash(const QString &string);

  void HashNodeValue(const NodeValue &value);

  QCryptographicHash hash_;

  QHash<void*, QByteArray> texture_ids_;

};

}

#endif // HASHTRAVERSER_H
