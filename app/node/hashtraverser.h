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
  virtual void ProcessVideoFootage(TexturePtr destination, const FootageJob &stream, const rational &input_time) override;

  virtual void ProcessAudioFootage(SampleBufferPtr destination, const FootageJob &stream, const TimeRange &input_time) override;

  virtual void ProcessShader(TexturePtr destination, const Node *node, const TimeRange &range, const ShaderJob& job) override;

  virtual void ProcessSamples(SampleBufferPtr destination, const Node *node, const TimeRange &range, const SampleJob &job) override;

  virtual void ProcessFrameGeneration(TexturePtr destination, const Node *node, const GenerateJob& job) override;

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
