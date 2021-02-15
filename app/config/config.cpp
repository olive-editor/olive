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
#include "ui/style/style.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

Config Config::current_config_;

Config::Config()
{
  SetDefaults();
}

void Config::SetEntryInternal(const QString &key, NodeValue::Type type, const QVariant &data)
{
  config_map_[key] = {type, data};
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
  SetEntryInternal(QStringLiteral("Style"), NodeValue::kText, StyleManager::kDefaultStyle);
  SetEntryInternal(QStringLiteral("TimecodeDisplay"), NodeValue::kInt, Timecode::kTimecodeDropFrame);
  SetEntryInternal(QStringLiteral("DefaultStillLength"), NodeValue::kRational, QVariant::fromValue(rational(2)));
  SetEntryInternal(QStringLiteral("HoverFocus"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("AudioScrubbing"), NodeValue::kBoolean, true);
  SetEntryInternal(QStringLiteral("AutorecoveryInterval"), NodeValue::kInt, 1);
  SetEntryInternal(QStringLiteral("DiskCacheSaveInterval"), NodeValue::kInt, 10000);
  SetEntryInternal(QStringLiteral("Language"), NodeValue::kText, QString());
  SetEntryInternal(QStringLiteral("ScrollZooms"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("EnableSeekToImport"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("EditToolAlsoSeeks"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("EditToolSelectsLinks"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("EnableDragFilesToTimeline"), NodeValue::kBoolean, true);
  SetEntryInternal(QStringLiteral("InvertTimelineScrollAxes"), NodeValue::kBoolean, true);
  SetEntryInternal(QStringLiteral("SelectAlsoSeeks"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("PasteSeeks"), NodeValue::kBoolean, true);
  SetEntryInternal(QStringLiteral("SelectAlsoSeeks"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("SetNameWithMarker"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("AutoSeekToBeginning"), NodeValue::kBoolean, true);
  SetEntryInternal(QStringLiteral("DropFileOnMediaToReplace"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("AddDefaultEffectsToClips"), NodeValue::kBoolean, true);
  SetEntryInternal(QStringLiteral("AutoscaleByDefault"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("Autoscroll"), NodeValue::kInt, AutoScroll::kPage);
  SetEntryInternal(QStringLiteral("AutoSelectDivider"), NodeValue::kBoolean, true);
  SetEntryInternal(QStringLiteral("SetNameWithMarker"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("RectifiedWaveforms"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("DropWithoutSequenceBehavior"), NodeValue::kInt, ImportTool::kDWSAsk);
  SetEntryInternal(QStringLiteral("Loop"), NodeValue::kBoolean, false);
  SetEntryInternal(QStringLiteral("SplitClipsCopyNodes"), NodeValue::kBoolean, true);
  SetEntryInternal(QStringLiteral("UseGradients"), NodeValue::kBoolean, true);
  SetEntryInternal(QStringLiteral("AutoMergeTracks"), NodeValue::kBoolean, true);

  SetEntryInternal(QStringLiteral("AutoCacheDelay"), NodeValue::kInt, 1000);

  SetEntryInternal(QStringLiteral("CatColor0"), NodeValue::kInt, 0);
  SetEntryInternal(QStringLiteral("CatColor1"), NodeValue::kInt, 1);
  SetEntryInternal(QStringLiteral("CatColor2"), NodeValue::kInt, 2);
  SetEntryInternal(QStringLiteral("CatColor3"), NodeValue::kInt, 3);
  SetEntryInternal(QStringLiteral("CatColor4"), NodeValue::kInt, 4);
  SetEntryInternal(QStringLiteral("CatColor5"), NodeValue::kInt, 5);
  SetEntryInternal(QStringLiteral("CatColor6"), NodeValue::kInt, 6);
  SetEntryInternal(QStringLiteral("CatColor7"), NodeValue::kInt, 7);
  SetEntryInternal(QStringLiteral("CatColor8"), NodeValue::kInt, 8);
  SetEntryInternal(QStringLiteral("CatColor9"), NodeValue::kInt, 9);
  SetEntryInternal(QStringLiteral("CatColor10"), NodeValue::kInt, 10);
  SetEntryInternal(QStringLiteral("CatColor11"), NodeValue::kInt, 11);

  SetEntryInternal(QStringLiteral("AudioOutput"), NodeValue::kText, QString());
  SetEntryInternal(QStringLiteral("AudioInput"), NodeValue::kText, QString());

  SetEntryInternal(QStringLiteral("DiskCacheBehind"), NodeValue::kRational, QVariant::fromValue(rational(1)));
  SetEntryInternal(QStringLiteral("DiskCacheAhead"), NodeValue::kRational, QVariant::fromValue(rational(5)));

  SetEntryInternal(QStringLiteral("DefaultSequenceWidth"), NodeValue::kInt, 1920);
  SetEntryInternal(QStringLiteral("DefaultSequenceHeight"), NodeValue::kInt, 1080);
  SetEntryInternal(QStringLiteral("DefaultSequencePixelAspect"), NodeValue::kRational, QVariant::fromValue(rational(1)));
  SetEntryInternal(QStringLiteral("DefaultSequenceFrameRate"), NodeValue::kRational, QVariant::fromValue(rational(1001, 30000)));
  SetEntryInternal(QStringLiteral("DefaultSequenceInterlacing"), NodeValue::kInt, VideoParams::kInterlaceNone);
  SetEntryInternal(QStringLiteral("DefaultSequenceAudioFrequency"), NodeValue::kInt, 48000);
  SetEntryInternal(QStringLiteral("DefaultSequenceAudioLayout"), NodeValue::kInt, QVariant::fromValue(static_cast<int64_t>(AV_CH_LAYOUT_STEREO)));

  // Online/offline settings
  SetEntryInternal(QStringLiteral("OnlinePixelFormat"), NodeValue::kInt, VideoParams::kFormatFloat32);
  SetEntryInternal(QStringLiteral("OfflinePixelFormat"), NodeValue::kInt, VideoParams::kFormatFloat16);
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

          current_config_[key] = QVariant::fromValue(match.flipped());
        } else {
          current_config_[key] = NodeValue::StringToValue(current_config_.GetConfigEntryType(key), value, false);
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

  QMapIterator<QString, ConfigEntry> iterator(current_config_.config_map_);
  while (iterator.hasNext()) {
    iterator.next();

    QString value = NodeValue::ValueToString(iterator.value().type, iterator.value().data, false);

    if (iterator.value().type == NodeValue::kNone) {
      qWarning() << "Config key" << iterator.key() << "had null type and was discarded";
    } else {
      writer.writeTextElement(iterator.key(), value);
    }
  }

  writer.writeEndElement(); // Configuration

  writer.writeEndDocument();

  config_file.close();

  if (!FileFunctions::RenameFileAllowOverwrite(temp_filename, real_filename)) {
    qWarning() << QStringLiteral("Failed to overwrite \"%1\". Config has been saved as \"%2\" instead.")
                  .arg(real_filename, temp_filename);
  }
}

QVariant Config::operator[](const QString &key) const
{
  return config_map_[key].data;
}

QVariant &Config::operator[](const QString &key)
{
  return config_map_[key].data;
}

NodeValue::Type Config::GetConfigEntryType(const QString &key) const
{
  return config_map_[key].type;
}

}
