#include "conform.h"

#include "codec/decoder.h"

ConformTask::ConformTask(AudioStreamPtr stream, const AudioRenderingParams& params) :
  stream_(stream),
  params_(params)
{
}

void ConformTask::Action()
{
  if (stream_->footage()->decoder().isEmpty()) {
    emit Failed(QStringLiteral("Stream has no decoder"));
  } else {
    DecoderPtr decoder = Decoder::CreateFromID(stream_->footage()->decoder());

    decoder->set_stream(stream_);

    connect(decoder.get(), &Decoder::IndexProgress, this, &ConformTask::ProgressChanged);

    decoder->Open();
    decoder->Conform(params_, &IsCancelled());
    decoder->Close();

    emit Succeeded();
  }
}
