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
#include <QXmlStreamWriter>

#include "core.h"
#include "common/filefunctions.h"

Config Config::current_config_;

Config::Config()
{
  SetDefaults();
}

QString Config::GetConfigFilePath()
{
  return QDir(GetConfigurationLocation()).filePath("config.xml");
}

Config &Config::Current()
{
  return current_config_;
}

void Config::SetDefaults()
{
  config_map_.clear();
  config_map_["TimecodeDisplay"] = olive::kTimecodeFrames;
  config_map_["DefaultStillLength"] = QVariant::fromValue(rational(2));
}

void Config::Load()
{
  QFile config_file(GetConfigFilePath());

  if (!config_file.exists()) {
    return;
  }

  if (!config_file.open(QFile::ReadOnly)) {
    qWarning() << QCoreApplication::translate("Config", "Failed to load application settings. This session will use "
                                                        "defaults.");
    return;
  }

  // Reset to defaults
  current_config_.SetDefaults();

  QXmlStreamReader reader(&config_file);
  while (!reader.atEnd()) {
    reader.readNext();

    if (!reader.isStartElement()) {
      continue;
    }

    QString key = reader.name().toString();

    reader.readNext();
    QString value = reader.text().toString();

    if (key == "Configuration") {
      // First element, ignore
    } else if (key == "Version") {
      if (!value.contains(".")) {
        qDebug() << "This is a 0.1.x config file, upconvert";
      }
    } else {
      current_config_[key] = value;
    }
  }

  if (reader.hasError()) {
    QMessageBox::critical(olive::core.main_window(),
                          QCoreApplication::translate("Config", "Error loading settings"),
                          QCoreApplication::translate("Config", "Failed to load application settings. This session will "
                                                                "use defaults."),
                          QMessageBox::Ok);
    current_config_.SetDefaults();
  }

  config_file.close();
}

void Config::Save()
{
  QFile config_file(GetConfigFilePath());

  if (!config_file.open(QFile::WriteOnly)) {
    QMessageBox::critical(olive::core.main_window(),
                          QCoreApplication::translate("Config", "Error saving settings"),
                          QCoreApplication::translate("Config", "Failed to save application settings. The application "
                                                                "may lack write permissions to this location."),
                          QMessageBox::Ok);
    return;
  }

  QXmlStreamWriter writer(&config_file);

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
