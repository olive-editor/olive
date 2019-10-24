#include "audio.h"

AudioInput::AudioInput()
{
  samples_output_ = new NodeOutput("samples_out");
  samples_output_->set_data_type(NodeInput::kSamples);
  AddParameter(samples_output_);
}

QString AudioInput::Name()
{
  return tr("Audio Input");
}

QString AudioInput::id()
{
  return "org.olivevideoeditor.Olive.audioinput";
}

QString AudioInput::Category()
{
  return tr("Input");
}

QString AudioInput::Description()
{
  return tr("Import an audio footage stream.");
}

QVariant AudioInput::Value(NodeOutput *output, const rational &in, const rational &out)
{
  if (output == samples_output_) {
    // Make sure decoder is set up
    if (!SetupDecoder()) {
      return 0;
    }

    // Retrieve audio samples from decoder
    frame_ = decoder_->Retrieve(in, out - in);

    QByteArray samples;
    samples.resize(frame_->audio_params().samples_to_bytes(frame_->sample_count()));
    memcpy(samples.data(), frame_->data(), static_cast<size_t>(samples.size()));
    return samples;
  }

  return 0;
}
