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

#include "hashtraverser.h"

#include <QUuid>

#include "common/filefunctions.h"

namespace olive {

#define super NodeTraverser

HashTraverser::HashTraverser() :
  hash_(QCryptographicHash::Sha1) // Appears to be the fastest hashing algorithm
{
}

QByteArray HashTraverser::GetHash(const Node *node, const Node::ValueHint &hint, const VideoParams &params, const TimeRange &range)
{
  // Reset hash
  hash_.reset();
  texture_ids_.clear();

  // Set params throughout traverser
  SetCacheVideoParams(params);

  // Embed video parameters into this hash
  Hash(params.effective_width());
  Hash(params.effective_height());
  Hash(params.format());
  Hash(params.interlacing());
  //Hash(reference);

  // Our overrides will generate a hash from this
  NodeValueTable table = GenerateTable(node, hint, range);
  NodeValue final_value = GenerateRowValueElement(hint, NodeValue::kTexture, &table);
  HashNodeValue(final_value);

  // Return the hash
  return hash_.result();
}

void HashTraverser::ProcessVideoFootage(TexturePtr destination, const FootageJob &stream, const rational &input_time)
{
  Hash(FileFunctions::GetUniqueFileIdentifier(stream.filename()));
  Hash(stream.loop_mode());
  Hash(stream.video_params().stream_index());
  Hash(stream.video_params().colorspace());
  Hash(stream.video_params().premultiplied_alpha());
  Hash(GetCacheVideoParams().divider());
  Hash(stream.video_params().video_type() == VideoParams::kVideoTypeStill ? 0 : input_time);
  Hash(stream.video_params().video_type());

  texture_ids_.insert(destination.get(), hash_.result());
}

void HashTraverser::ProcessAudioFootage(SampleBuffer &destination, const FootageJob &stream, const TimeRange &input_time)
{
}

void HashTraverser::ProcessShader(TexturePtr destination, const Node *node, const TimeRange &range, const ShaderJob &job)
{
  HashGenerateJob(node, &job);

  Hash(job.GetShaderID());
  Hash(job.GetIterativeInput());
  Hash(job.GetIterationCount());

  for (auto it=job.GetInterpolationMap().cbegin(); it!=job.GetInterpolationMap().cend(); it++) {
    Hash(it.key());
    Hash(it.value());
  }

  texture_ids_.insert(destination.get(), hash_.result());
}

void HashTraverser::ProcessColorTransform(TexturePtr destination, const Node *node, const ColorTransformJob &job)
{
  Hash(job.GetColorProcessor()->id());
  texture_ids_.insert(destination.get(), hash_.result());
}

void HashTraverser::ProcessSamples(SampleBuffer &destination, const Node *node, const TimeRange &range, const SampleJob &job)
{
}

void HashTraverser::ProcessFrameGeneration(TexturePtr destination, const Node *node, const GenerateJob &job)
{
  HashGenerateJob(node, &job);

  texture_ids_.insert(destination.get(), hash_.result());
}

void HashTraverser::HashGenerateJob(const Node *node, const GenerateJob *job)
{
  Hash(node->id());
  Hash(job->GetAlphaChannelRequired());

  for (auto it=job->GetValues().cbegin(); it!=job->GetValues().cend(); it++) {
    Hash(it.key());
    HashNodeValue(it.value());
  }
}

void HashTraverser::Hash(const QByteArray &array)
{
  hash_.addData(array);
}

void HashTraverser::Hash(const QString &string)
{
  hash_.addData(string.toUtf8());
}

void HashTraverser::HashNodeValue(const NodeValue &value)
{
  NodeValue::Type value_type = value.type();

  if (value_type == NodeValue::kSamples || value_type == NodeValue::kTexture) {
    QByteArray id_for_buffer;
    if (value_type == NodeValue::kTexture) {
      TexturePtr texture = value.toTexture();
      id_for_buffer = texture_ids_.value(texture.get());
    }

    if (!id_for_buffer.isEmpty()) {
      Hash(id_for_buffer);
    }
  } else {
    Hash(NodeValue::ValueToBytes(value));
  }
}

template<typename T>
void HashTraverser::Hash(T value)
{
  hash_.addData(reinterpret_cast<const char*>(&value), sizeof(value));
}

}
