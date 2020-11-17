/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include "exportformat.h"

namespace olive {

QString ExportFormat::GetName(olive::ExportFormat::Format f)
{
  switch (f) {
  case kFormatDNxHD:
    return tr("DNxHD");
  case kFormatMatroska:
    return tr("Matroska Video");
  case kFormatMPEG4:
    return tr("MPEG-4 Video");
  case kFormatOpenEXR:
    return tr("OpenEXR");
  case kFormatPNG:
    return tr("PNG");
  case kFormatTIFF:
    return tr("TIFF");
  case kFormatQuickTime:
    return tr("QuickTime");
  case kFormatCount:
    break;
  }

  return tr("Unknown");
}

QString ExportFormat::GetExtension(ExportFormat::Format f)
{
  switch (f) {
  case kFormatDNxHD:
    return QStringLiteral("mxf");
  case kFormatMatroska:
    return QStringLiteral("mkv");
  case kFormatMPEG4:
    return QStringLiteral("mp4");
  case kFormatOpenEXR:
    return QStringLiteral("exr");
  case kFormatPNG:
    return QStringLiteral("png");
  case kFormatTIFF:
    return QStringLiteral("tiff");
  case kFormatQuickTime:
    return QStringLiteral("mov");
  case kFormatCount:
    break;
  }

  return QString();
}

QString ExportFormat::GetEncoder(ExportFormat::Format f)
{
  switch (f) {
  case kFormatDNxHD:
  case kFormatMatroska:
  case kFormatQuickTime:
  case kFormatMPEG4:
    return QStringLiteral("ffmpeg");
  case kFormatOpenEXR:
  case kFormatPNG:
  case kFormatTIFF:
    return QStringLiteral("oiio");
  case kFormatCount:
    break;
  }

  return QString();
}

QList<ExportCodec::Codec> ExportFormat::GetVideoCodecs(ExportFormat::Format f)
{
  switch (f) {
  case kFormatDNxHD:
    return {ExportCodec::kCodecDNxHD};
  case kFormatMatroska:
    return {ExportCodec::kCodecH264, ExportCodec::kCodecH265};
  case kFormatMPEG4:
    return {ExportCodec::kCodecH264, ExportCodec::kCodecH265};
  case kFormatOpenEXR:
    return {ExportCodec::kCodecOpenEXR};
  case kFormatPNG:
    return {ExportCodec::kCodecPNG};
  case kFormatTIFF:
    return {ExportCodec::kCodecTIFF};
  case kFormatQuickTime:
    return {ExportCodec::kCodecH264, ExportCodec::kCodecH265, ExportCodec::kCodecProRes};
  case kFormatCount:
    break;
  }

  return {};
}

QList<ExportCodec::Codec> ExportFormat::GetAudioCodecs(ExportFormat::Format f)
{
  switch (f) {
  case kFormatDNxHD:
    return {ExportCodec::kCodecPCM};
  case kFormatMatroska:
    return {ExportCodec::kCodecAAC, ExportCodec::kCodecMP2, ExportCodec::kCodecMP3, ExportCodec::kCodecPCM};
  case kFormatMPEG4:
    return {ExportCodec::kCodecAAC, ExportCodec::kCodecMP2, ExportCodec::kCodecMP3, ExportCodec::kCodecPCM};
  case kFormatQuickTime:
    return {ExportCodec::kCodecAAC, ExportCodec::kCodecMP2, ExportCodec::kCodecMP3, ExportCodec::kCodecPCM};
  case kFormatOpenEXR:
  case kFormatPNG:
  case kFormatTIFF:
    return {};
  case kFormatCount:
    break;
  }

  return {};
}

}
