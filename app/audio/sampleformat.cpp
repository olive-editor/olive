#include "sampleformat.h"

#include "core.h"

QString SampleFormat::GetSampleFormatName(const SampleFormat::Format &f)
{
  switch (f) {
  case SAMPLE_FMT_U8:
    return tr("Unsigned 8-bit");
  case SAMPLE_FMT_S16:
    return tr("Signed 16-bit");
  case SAMPLE_FMT_S32:
    return tr("Signed 32-bit");
  case SAMPLE_FMT_S64:
    return tr("Signed 64-bit");
  case SAMPLE_FMT_FLT:
    return tr("32-bit Float");
  case SAMPLE_FMT_DBL:
    return tr("64-bit Float");
  case SAMPLE_FMT_COUNT:
  case SAMPLE_FMT_INVALID:
    break;
  }

  return tr("Invalid");
}

SampleFormat::Format SampleFormat::GetConfiguredFormatForMode(RenderMode::Mode /*mode*/)
{
  //return static_cast<SampleFormat::Format>(Core::GetPreferenceForRenderMode(mode, QStringLiteral("SampleFormat")).toInt());
  return SAMPLE_FMT_FLT;
}

void SampleFormat::SetConfiguredFormatForMode(RenderMode::Mode mode, SampleFormat::Format format)
{
  Core::SetPreferenceForRenderMode(mode, QStringLiteral("SampleFormat"), format);
}
