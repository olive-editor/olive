/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#include "config.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>
#include <QXmlStreamWriter>

#include "codec/exportformat.h"
#include "common/autoscroll.h"
#include "common/filefunctions.h"
#include "common/xmlutils.h"
#include "core.h"
#include "timeline/timelinecommon.h"
#include "ui/colorcoding.h"
#include "ui/style/style.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

Config Config::current_config_;

Config::Config()
{
  SetDefaults();
}

QString Config::GetConfigFilePath()
{
  return QDir(FileFunctions::GetConfigurationLocation()).filePath(QStringLiteral("config.xml"));
}

Config &Config::Current()
{
  return current_config_;
}

void Config::SetDefaults()
{
  config_map_.clear();

  OLIVE_CONFIG("Style") = StyleManager::kDefaultStyle;
  OLIVE_CONFIG("TimecodeDisplay") = Timecode::kTimecodeDropFrame;
  OLIVE_CONFIG("DefaultStillLength") = rational(2);
  OLIVE_CONFIG("HoverFocus") = false;
  OLIVE_CONFIG("AudioScrubbing") = true;
  OLIVE_CONFIG("AutorecoveryEnabled") = true;
  OLIVE_CONFIG("AutorecoveryInterval") = 1;
  OLIVE_CONFIG("AutorecoveryMaximum") = 20;
  OLIVE_CONFIG("DiskCacheSaveInterval") = 10000;
  OLIVE_CONFIG("Language") = QString();
  OLIVE_CONFIG("ScrollZooms") = false;
  OLIVE_CONFIG("EnableSeekToImport") = false;
  OLIVE_CONFIG("EditToolAlsoSeeks") = false;
  OLIVE_CONFIG("EditToolSelectsLinks") = false;
  OLIVE_CONFIG("EnableDragFilesToTimeline") = true;
  OLIVE_CONFIG("InvertTimelineScrollAxes") = true;
  OLIVE_CONFIG("SelectAlsoSeeks") = false;
  OLIVE_CONFIG("PasteSeeks") = true;
  OLIVE_CONFIG("SeekAlsoSelects") = false;
  OLIVE_CONFIG("SetNameWithMarker") = false;
  OLIVE_CONFIG("AutoSeekToBeginning") = true;
  OLIVE_CONFIG("DropFileOnMediaToReplace") = false;
  OLIVE_CONFIG("AddDefaultEffectsToClips") = true;
  OLIVE_CONFIG("AutoscaleByDefault") = false;
  OLIVE_CONFIG("Autoscroll") = AutoScroll::kPage;
  OLIVE_CONFIG("AutoSelectDivider") = true;
  OLIVE_CONFIG("SetNameWithMarker") = false;
  OLIVE_CONFIG("RectifiedWaveforms") = true;
  OLIVE_CONFIG("DropWithoutSequenceBehavior") = ImportTool::kDWSAsk;
  OLIVE_CONFIG("Loop") = false;
  OLIVE_CONFIG("SplitClipsCopyNodes") = true;
  OLIVE_CONFIG("UseGradients") = true;
  OLIVE_CONFIG("AutoMergeTracks") = true;
  OLIVE_CONFIG("UseSliderLadders") = true;
  OLIVE_CONFIG("ShowWelcomeDialog") = true;
  OLIVE_CONFIG("ShowClipWhileDragging") = true;
  OLIVE_CONFIG("StopPlaybackOnLastFrame") = false;
  OLIVE_CONFIG("UseLegacyColorInInputTab") = false;
  OLIVE_CONFIG("ReassocLinToNonLin") = false;
  OLIVE_CONFIG("PreviewNonFloatDontAskAgain") = false;
  OLIVE_CONFIG("UseGLFinish") = false;

  OLIVE_CONFIG("TimelineThumbnailMode") = Timeline::kThumbnailInOut;
  OLIVE_CONFIG("TimelineWaveformMode") = Timeline::kWaveformsEnabled;

  OLIVE_CONFIG("DefaultVideoTransition") = QStringLiteral("org.olivevideoeditor.Olive.crossdissolve");
  OLIVE_CONFIG("DefaultAudioTransition") = QStringLiteral("org.olivevideoeditor.Olive.crossdissolve");
  OLIVE_CONFIG("DefaultTransitionLength") = rational(1);

  OLIVE_CONFIG("DefaultSubtitleSize") = 48;
  OLIVE_CONFIG("DefaultSubtitleFamily") = QString();
  OLIVE_CONFIG("DefaultSubtitleWeight") = QFont::Bold;
  OLIVE_CONFIG("AntialiasSubtitles") = true;

  OLIVE_CONFIG("AutoCacheDelay") = 1000;

  OLIVE_CONFIG("CatColor0") = ColorCoding::kRed;
  OLIVE_CONFIG("CatColor1") = ColorCoding::kMaroon;
  OLIVE_CONFIG("CatColor2") = ColorCoding::kOrange;
  OLIVE_CONFIG("CatColor3") = ColorCoding::kBrown;
  OLIVE_CONFIG("CatColor4") = ColorCoding::kYellow;
  OLIVE_CONFIG("CatColor5") = ColorCoding::kOlive;
  OLIVE_CONFIG("CatColor6") = ColorCoding::kLime;
  OLIVE_CONFIG("CatColor7") = ColorCoding::kGreen;
  OLIVE_CONFIG("CatColor8") = ColorCoding::kCyan;
  OLIVE_CONFIG("CatColor9") = ColorCoding::kTeal;
  OLIVE_CONFIG("CatColor10") = ColorCoding::kBlue;
  OLIVE_CONFIG("CatColor11") = ColorCoding::kNavy;

  OLIVE_CONFIG("AudioOutput") = QString();
  OLIVE_CONFIG("AudioInput") = QString();

  OLIVE_CONFIG("AudioOutputSampleRate") = 48000;
  OLIVE_CONFIG("AudioOutputChannelLayout") = AudioChannelLayout::STEREO.toString();
  OLIVE_CONFIG("AudioOutputSampleFormat") = SampleFormat(SampleFormat::S16).to_string();

  OLIVE_CONFIG("AudioRecordingFormat") = ExportFormat::kFormatWAV;
  OLIVE_CONFIG("AudioRecordingCodec") = ExportCodec::kCodecPCM;
  OLIVE_CONFIG("AudioRecordingSampleRate") = 48000;
  OLIVE_CONFIG("AudioRecordingChannelLayout") = AudioChannelLayout::STEREO.toString();
  OLIVE_CONFIG("AudioRecordingSampleFormat") = SampleFormat(SampleFormat::S16).to_string();
  OLIVE_CONFIG("AudioRecordingBitRate") = 320;

  OLIVE_CONFIG("DiskCacheBehind") = rational(0);
  OLIVE_CONFIG("DiskCacheAhead") = rational(60);

  OLIVE_CONFIG("DefaultSequenceWidth") = 1920;
  OLIVE_CONFIG("DefaultSequenceHeight") = 1080;
  OLIVE_CONFIG("DefaultSequencePixelAspect") = rational(1);
  OLIVE_CONFIG("DefaultSequenceFrameRate") = rational(1001, 30000);
  OLIVE_CONFIG("DefaultSequenceInterlacing") = VideoParams::kInterlaceNone;
  OLIVE_CONFIG("DefaultSequenceAutoCache2") = false;
  OLIVE_CONFIG("DefaultSequenceAudioFrequency") = 48000;
  OLIVE_CONFIG("DefaultSequenceAudioLayout") = AudioChannelLayout::STEREO.toString();

  // Online/offline settings
  OLIVE_CONFIG("OnlinePixelFormat") = PixelFormat::F32;
  OLIVE_CONFIG("OfflinePixelFormat") = PixelFormat::F16;

  OLIVE_CONFIG("MarkerColor") = ColorCoding::kLime;
}

void Config::Load()
{
  // Reset to defaults
  current_config_.SetDefaults();

  QFile config_file(GetConfigFilePath());

  if (!config_file.exists()) {
    return;
  }

  if (!config_file.open(QFile::ReadOnly)) {
    qWarning() << "Failed to load application settings. This session will use defaults.";
    return;
  }

  QXmlStreamReader reader(&config_file);

  QString config_version;

  while (XMLReadNextStartElement(&reader)) {
    if (reader.name() == QStringLiteral("Configuration")) {
      while (XMLReadNextStartElement(&reader)) {
        QString key = reader.name().toString();
        QString value = reader.readElementText();

        if (key == QStringLiteral("Version")) {
          config_version = value;

          if (!value.contains(".")) {
            qDebug() << "CONFIG: This is a 0.1.x config file, upconvert";
          }
        } else if (key == QStringLiteral("DefaultSequenceFrameRate") && !config_version.contains('.')) {
          // 0.1.x stored this value as a float while we now use rationals, we'll use a heuristic to find the closest
          // supported rational
          qDebug() << "  CONFIG: Finding closest match to" << value;

          double config_fr = value.toDouble();

          const QVector<rational>& supported_frame_rates = VideoParams::kSupportedFrameRates;

          rational match = supported_frame_rates.first();
          double match_diff = qAbs(match.toDouble() - config_fr);

          for (int i=1;i<supported_frame_rates.size();i++) {
            double diff = qAbs(supported_frame_rates.at(i).toDouble() - config_fr);

            if (diff < match_diff) {
              match = supported_frame_rates.at(i);
              match_diff = diff;
            }
          }

          qDebug() << "  CONFIG: Closest match was" << match.toDouble();

          current_config_[key] = match.flipped();
        } else {
          value_t &v = current_config_[key];
          v = value_t(value).converted(v.type());
        }
      }

      //reader.skipCurrentElement();
    } else {
      reader.skipCurrentElement();
    }
  }

  if (reader.hasError()) {
    QMessageBox::critical(Core::instance()->main_window(),
                          QCoreApplication::translate("Config", "Error loading settings"),
                          QCoreApplication::translate("Config", "Failed to load application settings. This session will "
                                                                "use defaults.\n\n%1").arg(reader.errorString()),
                          QMessageBox::Ok);
    current_config_.SetDefaults();
  }

  config_file.close();
}

void Config::Save()
{
  QString real_filename = GetConfigFilePath();
  QString temp_filename = FileFunctions::GetSafeTemporaryFilename(real_filename);

  QFile config_file(temp_filename);

  if (!config_file.open(QFile::WriteOnly)) {
    QMessageBox::critical(Core::instance()->main_window(),
                          QCoreApplication::translate("Config", "Error saving settings"),
                          QCoreApplication::translate("Config", "Failed to save application settings. The application "
                                                                "may lack write permissions for this location."),
                          QMessageBox::Ok);
    return;
  }

  QXmlStreamWriter writer(&config_file);
  writer.setAutoFormatting(true);

  writer.writeStartDocument();

  writer.writeStartElement("Configuration");

  // Anything after the hyphen is considered "unimportant" information
  writer.writeTextElement("Version", QCoreApplication::applicationVersion().split('-').first());

  for (auto it = current_config_.config_map_.cbegin(); it != current_config_.config_map_.cend(); it++) {
    writer.writeTextElement(it.key(), it.value().converted(TYPE_STRING).toString());
  }

  writer.writeEndElement(); // Configuration

  writer.writeEndDocument();

  config_file.close();

  if (!FileFunctions::RenameFileAllowOverwrite(temp_filename, real_filename)) {
    qWarning() << QStringLiteral("Failed to overwrite \"%1\". Config has been saved as \"%2\" instead.")
                  .arg(real_filename, temp_filename);
  }
}

value_t Config::operator[](const QString &key) const
{
  return config_map_[key];
}

value_t &Config::operator[](const QString &key)
{
  return config_map_[key];
}

}
