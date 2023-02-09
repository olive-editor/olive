#include "humanstrings.h"

#include <QCoreApplication>

namespace olive {

QString HumanStrings::SampleRateToString(const int &sample_rate)
{
  return QCoreApplication::translate("AudioParams", "%1 Hz").arg(sample_rate);
}

QString HumanStrings::ChannelLayoutToString(const uint64_t &layout)
{
  switch (layout) {
  case AV_CH_LAYOUT_MONO:
    return QCoreApplication::translate("AudioParams", "Mono");
  case AV_CH_LAYOUT_STEREO:
    return QCoreApplication::translate("AudioParams", "Stereo");
  case AV_CH_LAYOUT_2_1:
    return QCoreApplication::translate("AudioParams", "2.1");
  case AV_CH_LAYOUT_5POINT1:
    return QCoreApplication::translate("AudioParams", "5.1");
  case AV_CH_LAYOUT_7POINT1:
    return QCoreApplication::translate("AudioParams", "7.1");
  default:
    return QCoreApplication::translate("AudioParams", "Unknown (0x%1)").arg(layout, 1, 16);
  }
}

QString HumanStrings::FormatToString(const SampleFormat &f)
{
  switch (f) {
  case SampleFormat::U8:
    return QCoreApplication::translate("AudioParams", "Unsigned 8-bit (Packed)");
  case SampleFormat::S16:
    return QCoreApplication::translate("AudioParams", "Signed 16-bit (Packed)");
  case SampleFormat::S32:
    return QCoreApplication::translate("AudioParams", "Signed 32-bit (Packed)");
  case SampleFormat::S64:
    return QCoreApplication::translate("AudioParams", "Signed 64-bit (Packed)");
  case SampleFormat::F32:
    return QCoreApplication::translate("AudioParams", "Float 32-bit (Packed)");
  case SampleFormat::F64:
    return QCoreApplication::translate("AudioParams", "Float 64-bit (Packed)");
  case SampleFormat::U8P:
    return QCoreApplication::translate("AudioParams", "Unsigned 8-bit (Planar)");
  case SampleFormat::S16P:
    return QCoreApplication::translate("AudioParams", "Signed 16-bit (Planar)");
  case SampleFormat::S32P:
    return QCoreApplication::translate("AudioParams", "Signed 32-bit (Planar)");
  case SampleFormat::S64P:
    return QCoreApplication::translate("AudioParams", "Signed 64-bit (Planar)");
  case SampleFormat::F32P:
    return QCoreApplication::translate("AudioParams", "Float 32-bit (Planar)");
  case SampleFormat::F64P:
    return QCoreApplication::translate("AudioParams", "Float 64-bit (Planar)");

  case SampleFormat::INVALID:
  case SampleFormat::COUNT:
    break;
  }

  return QCoreApplication::translate("AudioParams", "Unknown (0x%1)").arg(f, 1, 16);
}

}
