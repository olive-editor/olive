/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "common/autoscroll.h"
#include "common/filefunctions.h"
#include "common/xmlutils.h"
#include "core.h"
#include "window/mainwindow/mainwindow.h"

OLIVE_NAMESPACE_ENTER

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
  config_map_["TimecodeDisplay"] = Timecode::kTimecodeDropFrame;
  config_map_["DefaultStillLength"] = QVariant::fromValue(rational(2));
  config_map_["HoverFocus"] = false;
  config_map_["AudioScrubbing"] = true;
  config_map_["AutorecoveryInterval"] = 1;
  config_map_["Language"] = "en_US";
  config_map_["ScrollZooms"] = false;
  config_map_["EnableSeekToImport"] = false;
  config_map_["EditToolAlsoSeeks"] = false;
  config_map_["EditToolSelectsLinks"] = false;
  config_map_["EnableDragFilesToTimeline"] = true;
  config_map_["InvertTimelineScrollAxes"] = true;
  config_map_["SelectAlsoSeeks"] = false;
  config_map_["PasteSeeks"] = true;
  config_map_["SelectAlsoSeeks"] = false;
  config_map_["SetNameWithMarker"] = false;
  config_map_["AutoSeekToBeginning"] = true;
  config_map_["DropFileOnMediaToReplace"] = false;
  config_map_["AddDefaultEffectsToClips"] = true;
  config_map_["AutoscaleByDefault"] = false;
  config_map_["Autoscroll"] = AutoScroll::kPage;
  config_map_["AutoSelectDivider"] = true;
  config_map_["SetNameWithMarker"] = false;
  config_map_["RectifiedWaveforms"] = false;
  config_map_["DropWithoutSequenceBehavior"] = TimelineWidget::kDWSAsk;
  config_map_["Loop"] = false;

  config_map_["AutoCacheInterval"] = 250;

  config_map_["NodeCatColor0"] = QVariant::fromValue(Color(0.75f, 0.75f, 0.75f));
  config_map_["NodeCatColor1"] = QVariant::fromValue(Color(0.25f, 0.25f, 0.25f));
  config_map_["NodeCatColor2"] = QVariant::fromValue(Color(0.75f, 0.75f, 0.25f));
  config_map_["NodeCatColor3"] = QVariant::fromValue(Color(0.75f, 0.25f, 0.75f));
  config_map_["NodeCatColor4"] = QVariant::fromValue(Color(0.25f, 0.75f, 0.75f));
  config_map_["NodeCatColor5"] = QVariant::fromValue(Color(0.50f, 0.50f, 0.50f));
  config_map_["NodeCatColor6"] = QVariant::fromValue(Color(0.25f, 0.75f, 0.25f));
  config_map_["NodeCatColor7"] = QVariant::fromValue(Color(0.25f, 0.25f, 0.75f));
  config_map_["NodeCatColor8"] = QVariant::fromValue(Color(0.75f, 0.25f, 0.25f));

  config_map_["AudioOutput"] = QString();
  config_map_["AudioInput"] = QString();

  config_map_["DiskCacheBehind"] = QVariant::fromValue(rational(1));
  config_map_["DiskCacheAhead"] = QVariant::fromValue(rational(5));

  config_map_["DefaultSequenceWidth"] = 1920;
  config_map_["DefaultSequenceHeight"] = 1080;
  config_map_["DefaultSequenceFrameRate"] = QVariant::fromValue(rational(1001, 30000));
  config_map_["DefaultSequenceAudioFrequency"] = 48000;
  config_map_["DefaultSequenceAudioLayout"] = QVariant::fromValue(static_cast<uint64_t>(AV_CH_LAYOUT_STEREO));
  config_map_["DefaultSequencePreviewFormat"] = PixelFormat::PIX_FMT_RGBA16F;

  // Online/offline settings
  config_map_["OnlinePixelFormat"] = PixelFormat::PIX_FMT_RGBA32F;
  config_map_["OfflinePixelFormat"] = PixelFormat::PIX_FMT_RGBA16F;
  config_map_["OnlineOCIOMethod"] = ColorManager::kOCIOAccurate;
  config_map_["OfflineOCIOMethod"] = ColorManager::kOCIOFast;
}

void Config::Load()
{
  QFile config_file(GetConfigFilePath());

  if (!config_file.exists()) {
    return;
  }

  if (!config_file.open(QFile::ReadOnly)) {
    qWarning() << "Failed to load application settings. This session will use defaults.";
    return;
  }

  // Reset to defaults
  current_config_.SetDefaults();

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

          QList<rational> supported_frame_rates = Core::SupportedFrameRates();

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

          current_config_[key] = QVariant::fromValue(match.flipped());
        } else {
          current_config_[key] = value;
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
  QFile config_file(GetConfigFilePath());

  if (!config_file.open(QFile::WriteOnly)) {
    QMessageBox::critical(Core::instance()->main_window(),
                          QCoreApplication::translate("Config", "Error saving settings"),
                          QCoreApplication::translate("Config", "Failed to save application settings. The application "
                                                                "may lack write permissions to this location."),
                          QMessageBox::Ok);
    return;
  }

  QXmlStreamWriter writer(&config_file);
  writer.setAutoFormatting(true);

  writer.writeStartDocument();

  writer.writeStartElement("Configuration");

  // Anything after the hyphen is considered "unimportant" information
  writer.writeTextElement("Version", QCoreApplication::applicationVersion().split('-').first());

  QMapIterator<QString, QVariant> iterator(current_config_.config_map_);
  while (iterator.hasNext()) {
    iterator.next();
    writer.writeTextElement(iterator.key(), iterator.value().toString());
  }

  writer.writeEndElement(); // Configuration

  writer.writeEndDocument();

  config_file.close();
}

QVariant Config::operator[](const QString &key) const
{
  return config_map_[key];
}

QVariant &Config::operator[](const QString &key)
{
  return config_map_[key];
}

OLIVE_NAMESPACE_EXIT
