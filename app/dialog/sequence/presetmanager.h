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

#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QObject>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/define.h"
#include "common/filefunctions.h"
#include "common/xmlutils.h"

OLIVE_NAMESPACE_ENTER

class Preset
{
public:
  Preset() = default;

  virtual ~Preset(){}

  const QString& GetName() const
  {
    return name_;
  }

  void SetName(const QString& s)
  {
    name_ = s;
  }

  virtual void Load(QXmlStreamReader* reader) = 0;

  virtual void Save(QXmlStreamWriter* writer) const = 0;

private:
  QString name_;

};

using PresetPtr = std::shared_ptr<Preset>;

template <typename T>
class PresetManager
{
public:
  PresetManager(QWidget* parent, const QString& preset_name) :
    preset_name_(preset_name),
    parent_(parent)
  {
    // Load custom preset data from file
    QFile preset_file(GetCustomPresetFilename());
    if (preset_file.open(QFile::ReadOnly)) {
      QXmlStreamReader reader(&preset_file);

      while (XMLReadNextStartElement(&reader)) {
        if (reader.name() == QStringLiteral("presets")) {
          while (XMLReadNextStartElement(&reader)) {
            if (reader.name() == QStringLiteral("preset")) {
              PresetPtr p = std::make_shared<T>();

              p->Load(&reader);

              custom_preset_data_.append(p);
            } else {
              reader.skipCurrentElement();
            }
          }
        } else {
          reader.skipCurrentElement();
        }
      }

      preset_file.close();
    }
  }

  ~PresetManager()
  {
    // Save custom presets to disk
    QFile preset_file(GetCustomPresetFilename());
    if (preset_file.open(QFile::WriteOnly)) {
      QXmlStreamWriter writer(&preset_file);
      writer.setAutoFormatting(true);

      writer.writeStartDocument();

      writer.writeStartElement(QStringLiteral("presets"));

      foreach (PresetPtr p, custom_preset_data_) {
        writer.writeStartElement(QStringLiteral("preset"));

        p->Save(&writer);

        writer.writeEndElement(); // preset
      }

      writer.writeEndElement(); // presets

      writer.writeEndDocument();

      preset_file.close();
    }
  }

  QString GetPresetName(QString start) const
  {
    bool ok;

    forever {
      start = QInputDialog::getText(parent_,
                                    QCoreApplication::translate("PresetManager", "Save Preset"),
                                    QCoreApplication::translate("PresetManager", "Set preset name:"),
                                    QLineEdit::Normal,
                                    start,
                                    &ok);

      if (!ok) {
        // Dialog cancelled - leave function entirely
        return QString();
      }

      if (start.isEmpty()) {
        // No preset name entered, start loop over
        QMessageBox::critical(parent_,
                              QCoreApplication::translate("PresetManager", "Invalid preset name"),
                              QCoreApplication::translate("PresetManager", "You must enter a preset name"),
                              QMessageBox::Ok);
      } else {
        break;
      }
    }

    return start;
  }

  bool SavePreset(PresetPtr preset)
  {
    QString preset_name;
    int existing_preset;

    forever {
      preset_name = GetPresetName(preset_name);

      if (preset_name.isEmpty()) {
        // Dialog cancelled - leave function entirely
        return false;
      }

      existing_preset = -1;
      for (int i=0; i<custom_preset_data_.size(); i++) {
        if (custom_preset_data_.at(i)->GetName() == preset_name) {
          existing_preset = i;
          break;
        }
      }

      if (existing_preset == -1
          || QMessageBox::question(parent_,
                                   QCoreApplication::translate("PresetManager", "Preset exists"),
                                   QCoreApplication::translate("PresetManager",
                                                               "A preset with this name already exists. "
                                                               "Would you like to replace it?")) == QMessageBox::Yes) {
        break;
      }
    }

    preset->SetName(preset_name);

    if (existing_preset >= 0) {
      custom_preset_data_.replace(existing_preset, preset);
      return false;
    } else {
      custom_preset_data_.append(preset);
      return true;
    }
  }

  QString GetCustomPresetFilename() const
  {
    return QDir(FileFunctions::GetConfigurationLocation()).filePath(preset_name_);
  }

  PresetPtr GetPreset(int index)
  {
    return custom_preset_data_.at(index);
  }

  void DeletePreset(int index)
  {
    custom_preset_data_.removeAt(index);
  }

  int GetNumberOfPresets() const
  {
    return custom_preset_data_.size();
  }

  const QVector<PresetPtr>& GetPresetData() const
  {
    return custom_preset_data_;
  }

private:
  QVector<PresetPtr> custom_preset_data_;

  QString preset_name_;

  QWidget* parent_;

};

OLIVE_NAMESPACE_EXIT

#endif // PRESETMANAGER_H
