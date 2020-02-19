#include "index.h"

#include "codec/decoder.h"

IndexTask::IndexTask(StreamPtr stream) :
  stream_(stream)
{
  SetTitle(tr("Indexing %1:%2").arg(stream_->footage()->filename(), QString::number(stream_->index())));
}

void IndexTask::Action()
{
  if (stream_->footage()->decoder().isEmpty()) {
    emit Failed(QStringLiteral("Stream has no decoder"));
  } else {
    DecoderPtr decoder = Decoder::CreateFromID(stream_->footage()->decoder());

    decoder->set_stream(stream_);

    connect(decoder.get(), &Decoder::IndexProgress, this, &IndexTask::ProgressChanged);

    decoder->Open();
    decoder->Index(&IsCancelled());
    decoder->Close();

    emit Succeeded();
  }
}
