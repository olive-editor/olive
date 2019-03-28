/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#include "effect.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMessageBox>
#include <QOpenGLContext>
#include <QDir>
#include <QPainter>
#include <QtMath>
#include <QMenu>
#include <QApplication>
#include <QFileDialog>

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "ui/collapsiblewidget.h"
#include "panels/project.h"
#include "undo/undo.h"
#include "timeline/sequence.h"
#include "timeline/clip.h"
#include "panels/timeline.h"
#include "panels/effectcontrols.h"
#include "panels/grapheditor.h"
#include "global/debug.h"
#include "global/path.h"
#include "ui/mainwindow.h"
#include "global/math.h"
#include "project/clipboard.h"
#include "global/config.h"
#include "transition.h"
#include "undo/undostack.h"

#include "effects/internal/transformeffect.h"
#include "effects/internal/texteffect.h"
#include "effects/internal/timecodeeffect.h"
#include "effects/internal/solideffect.h"
#include "effects/internal/audionoiseeffect.h"
#include "effects/internal/toneeffect.h"
#include "effects/internal/volumeeffect.h"
#include "effects/internal/paneffect.h"
#include "effects/internal/shakeeffect.h"
#include "effects/internal/cornerpineffect.h"
#include "effects/internal/vsthost.h"
#include "effects/internal/fillleftrighteffect.h"
#include "effects/internal/frei0reffect.h"
#include "effects/internal/richtexteffect.h"

QVector<EffectMeta> effects;

EffectPtr Effect::Create(Clip* c, const EffectMeta* em) {
  if (em->internal >= 0 && em->internal < EFFECT_INTERNAL_COUNT) {
    // must be an internal effect
    switch (em->internal) {
    case EFFECT_INTERNAL_TRANSFORM: return std::make_shared<TransformEffect>(c, em);
    case EFFECT_INTERNAL_TEXT: return std::make_shared<TextEffect>(c, em);
    case EFFECT_INTERNAL_TIMECODE: return std::make_shared<TimecodeEffect>(c, em);
    case EFFECT_INTERNAL_SOLID: return std::make_shared<SolidEffect>(c, em);
    case EFFECT_INTERNAL_NOISE: return std::make_shared<AudioNoiseEffect>(c, em);
    case EFFECT_INTERNAL_VOLUME: return std::make_shared<VolumeEffect>(c, em);
    case EFFECT_INTERNAL_PAN: return std::make_shared<PanEffect>(c, em);
    case EFFECT_INTERNAL_TONE: return std::make_shared<ToneEffect>(c, em);
    case EFFECT_INTERNAL_SHAKE: return std::make_shared<ShakeEffect>(c, em);
    case EFFECT_INTERNAL_CORNERPIN: return std::make_shared<CornerPinEffect>(c, em);
    case EFFECT_INTERNAL_FILLLEFTRIGHT: return std::make_shared<FillLeftRightEffect>(c, em);
    case EFFECT_INTERNAL_VST: return std::make_shared<VSTHost>(c, em);
#ifndef NOFREI0R
    case EFFECT_INTERNAL_FREI0R: return std::make_shared<Frei0rEffect>(c, em);
#endif
    case EFFECT_INTERNAL_RICHTEXT: return std::make_shared<RichTextEffect>(c, em);
    }
  } else if (!em->filename.isEmpty()) {
    // load effect from file
    return std::make_shared<Effect>(c, em);
  } else {
    qCritical() << "Invalid effect data";
    QMessageBox::critical(olive::MainWindow,
                          QCoreApplication::translate("Effect", "Invalid effect"),
                          QCoreApplication::translate("Effect", "No candidate for effect '%1'. This effect may be corrupt. Try reinstalling it or Olive.").arg(em->name));
  }
  return nullptr;
}

const EffectMeta* Effect::GetInternalMeta(int internal_id, int type) {
  for (int i=0;i<effects.size();i++) {
    if (effects.at(i).internal == internal_id && effects.at(i).type == type) {
      return &effects.at(i);
    }
  }
  return nullptr;
}

Effect::Effect(Clip* c, const EffectMeta *em) :
  parent_clip(c),
  meta(em),
  flags_(0),
  glslProgram(nullptr),
  texture(nullptr),
  isOpen(false),
  bound(false),
  iterations(1),
  enabled_(true),
  expanded_(true)
{
  if (em != nullptr) {
    // set up UI from effect file
    name = em->name;

    if (!em->filename.isEmpty() && em->internal == -1) {
      QFile effect_file(em->filename);
      if (effect_file.open(QFile::ReadOnly)) {
        QXmlStreamReader reader(&effect_file);

        while (!reader.atEnd()) {
          if (reader.name() == "row" && reader.isStartElement()) {
            QString row_name;
            const QXmlStreamAttributes& attributes = reader.attributes();
            for (int i=0;i<attributes.size();i++) {
              const QXmlStreamAttribute& attr = attributes.at(i);
              if (attr.name() == "name") {
                row_name = attr.value().toString();
              }
            }
            if (!row_name.isEmpty()) {
              EffectRow* row = new EffectRow(this, row_name);
              while (!reader.atEnd() && !(reader.name() == "row" && reader.isEndElement())) {
                reader.readNext();
                if (reader.name() == "field" && reader.isStartElement()) {
                  int type = -1;
                  QString id;

                  // get field type
                  const QXmlStreamAttributes& attributes = reader.attributes();
                  for (int i=0;i<attributes.size();i++) {
                    const QXmlStreamAttribute& attr = attributes.at(i);
                    if (attr.name() == "type") {
                      QString comp = attr.value().toString().toUpper();
                      if (comp == "DOUBLE") {
                        type = EffectField::EFFECT_FIELD_DOUBLE;
                      } else if (comp == "BOOL") {
                        type = EffectField::EFFECT_FIELD_BOOL;
                      } else if (comp == "COLOR") {
                        type = EffectField::EFFECT_FIELD_COLOR;
                      } else if (comp == "COMBO") {
                        type = EffectField::EFFECT_FIELD_COMBO;
                      } else if (comp == "FONT") {
                        type = EffectField::EFFECT_FIELD_FONT;
                      } else if (comp == "STRING") {
                        type = EffectField::EFFECT_FIELD_STRING;
                      } else if (comp == "FILE") {
                        type = EffectField::EFFECT_FIELD_FILE;
                      }
                    } else if (attr.name() == "id") {
                      id = attr.value().toString();
                    }
                  }

                  if (id.isEmpty()) {
                    qCritical() << "Couldn't load field from" << em->filename << "- ID cannot be empty.";
                  } else {
                    EffectField* field = nullptr;

                    switch (type) {
                    case EffectField::EFFECT_FIELD_DOUBLE:
                    {

                      DoubleField* double_field = new DoubleField(row, id);

                      for (int i=0;i<attributes.size();i++) {
                        const QXmlStreamAttribute& attr = attributes.at(i);
                        if (attr.name() == "default") {
                          double_field->SetDefault(attr.value().toDouble());
                        } else if (attr.name() == "min") {
                          double_field->SetMinimum(attr.value().toDouble());
                        } else if (attr.name() == "max") {
                          double_field->SetMaximum(attr.value().toDouble());
                        }
                      }

                      field = double_field;
                    }
                      break;
                    case EffectField::EFFECT_FIELD_COLOR:
                    {
                      QColor color;

                      field = new ColorField(row, id);

                      for (int i=0;i<attributes.size();i++) {
                        const QXmlStreamAttribute& attr = attributes.at(i);
                        if (attr.name() == "r") {
                          color.setRed(attr.value().toInt());
                        } else if (attr.name() == "g") {
                          color.setGreen(attr.value().toInt());
                        } else if (attr.name() == "b") {
                          color.setBlue(attr.value().toInt());
                        } else if (attr.name() == "rf") {
                          color.setRedF(attr.value().toDouble());
                        } else if (attr.name() == "gf") {
                          color.setGreenF(attr.value().toDouble());
                        } else if (attr.name() == "bf") {
                          color.setBlueF(attr.value().toDouble());
                        } else if (attr.name() == "hex") {
                          color.setNamedColor(attr.value().toString());
                        }
                      }

                      field->SetValueAt(0, color);
                    }
                      break;
                    case EffectField::EFFECT_FIELD_STRING:
                      field = new StringField(row, id);
                      for (int i=0;i<attributes.size();i++) {
                        const QXmlStreamAttribute& attr = attributes.at(i);
                        if (attr.name() == "default") {
                          field->SetValueAt(0, attr.value().toString());
                        }
                      }
                      break;
                    case EffectField::EFFECT_FIELD_BOOL:
                      field = new BoolField(row, id);
                      for (int i=0;i<attributes.size();i++) {
                        const QXmlStreamAttribute& attr = attributes.at(i);
                        if (attr.name() == "default") {
                          field->SetValueAt(0, attr.value() == "1");
                        }
                      }
                      break;
                    case EffectField::EFFECT_FIELD_COMBO:
                    {
                      ComboField* combo_field = new ComboField(row, id);
                      int combo_default_index = 0;
                      for (int i=0;i<attributes.size();i++) {
                        const QXmlStreamAttribute& attr = attributes.at(i);
                        if (attr.name() == "default") {
                          combo_default_index = attr.value().toInt();
                          break;
                        }
                      }
                      int combo_item_count = 0;
                      while (!reader.atEnd() && !(reader.name() == "field" && reader.isEndElement())) {
                        reader.readNext();
                        if (reader.name() == "option" && reader.isStartElement()) {
                          reader.readNext();
                          combo_field->AddItem(reader.text().toString(), combo_item_count);
                          combo_item_count++;
                        }
                      }
                      combo_field->SetValueAt(0, combo_default_index);
                      field = combo_field;
                    }
                      break;
                    case EffectField::EFFECT_FIELD_FONT:
                      field = new FontField(row, id);
                      for (int i=0;i<attributes.size();i++) {
                        const QXmlStreamAttribute& attr = attributes.at(i);
                        if (attr.name() == "default") {
                          field->SetValueAt(0, attr.value().toString());
                        }
                      }
                      break;
                    case EffectField::EFFECT_FIELD_FILE:
                      field = new FileField(row, id);
                      for (int i=0;i<attributes.size();i++) {
                        const QXmlStreamAttribute& attr = attributes.at(i);
                        if (attr.name() == "filename") {
                          field->SetValueAt(0, attr.value().toString());
                        }
                      }
                      break;
                    }
                  }
                }
              }
            }
          } else if (reader.name() == "shader" && reader.isStartElement()) {
            SetFlags(Flags() | ShaderFlag);
            const QXmlStreamAttributes& attributes = reader.attributes();
            for (int i=0;i<attributes.size();i++) {
              const QXmlStreamAttribute& attr = attributes.at(i);
              if (attr.name() == "vert") {
                vertPath = attr.value().toString();
              } else if (attr.name() == "frag") {
                fragPath = attr.value().toString();
              } else if (attr.name() == "iterations") {
                setIterations(attr.value().toInt());
              }
            }
          }/* else if (reader.name() == "superimpose" && reader.isStartElement()) {
            enable_superimpose = true;
            const QXmlStreamAttributes& attributes = reader.attributes();
            for (int i=0;i<attributes.size();i++) {
              const QXmlStreamAttribute& attr = attributes.at(i);
              if (attr.name() == "script") {
                QFile script_file(get_effects_dir() + "/" + attr.value().toString());
                if (script_file.open(QFile::ReadOnly)) {
                  script = script_file.readAll();
                } else {
                  qCritical() << "Failed to open superimpose script file for" << em->filename;
                  enable_superimpose = false;
                }
                break;
              }
            }
          }*/
          reader.readNext();
        }

        effect_file.close();
      } else {
        qCritical() << "Failed to open effect file" << em->filename;
      }
    }
  }
}

Effect::~Effect() {
  if (isOpen) {
    close();
  }

  // Clear graph editor if it's using one of these rows
  if (panel_graph_editor != nullptr) {
    for (int i=0;i<row_count();i++) {
      if (row(i) == panel_graph_editor->get_row()) {
        panel_graph_editor->set_row(nullptr);
        break;
      }
    }
  }
}

void Effect::AddRow(EffectRow *row)
{
  row->setParent(this);
  rows.append(row);
}

void Effect::copy_field_keyframes(EffectPtr e) {
  for (int i=0;i<rows.size();i++) {
    EffectRow* row = rows.at(i);
    EffectRow* copy_row = e->rows.at(i);
    copy_row->SetKeyframingInternal(row->IsKeyframing());
    for (int j=0;j<row->FieldCount();j++) {
      // Get field from this (the source) effect
      EffectField* field = row->Field(j);

      // Get field from the destination effect
      EffectField* copy_field = copy_row->Field(j);

      // Copy keyframes between effects
      copy_field->keyframes = field->keyframes;

      // Copy persistet data between effects
      copy_field->persistent_data_ = field->persistent_data_;
    }
  }
}

EffectRow* Effect::row(int i) {
  return rows.at(i);
}

int Effect::row_count() {
  return rows.size();
}

EffectGizmo *Effect::add_gizmo(int type) {
  EffectGizmo* gizmo = new EffectGizmo(this, type);
  gizmos.append(gizmo);
  return gizmo;
}

EffectGizmo *Effect::gizmo(int i) {
  return gizmos.at(i);
}

int Effect::gizmo_count() {
  return gizmos.size();
}

void Effect::refresh() {}

void Effect::FieldChanged() {
  update_ui(false);
}

void Effect::delete_self() {
  olive::UndoStack.push(new EffectDeleteCommand(this));
  update_ui(true);
}

void Effect::move_up() {
  int index_of_effect = parent_clip->IndexOfEffect(this);
  if (index_of_effect == 0) {
    return;
  }

  MoveEffectCommand* command = new MoveEffectCommand();
  command->clip = parent_clip;
  command->from = index_of_effect;
  command->to = command->from - 1;
  olive::UndoStack.push(command);
  panel_effect_controls->Reload();
  panel_sequence_viewer->viewer_widget->frame_update();
}

void Effect::move_down() {
  int index_of_effect = parent_clip->IndexOfEffect(this);
  if (index_of_effect == parent_clip->effects.size()-1) {
    return;
  }

  MoveEffectCommand* command = new MoveEffectCommand();
  command->clip = parent_clip;
  command->from = index_of_effect;
  command->to = command->from + 1;
  olive::UndoStack.push(command);
  panel_effect_controls->Reload();
  panel_sequence_viewer->viewer_widget->frame_update();
}

void Effect::save_to_file() {
  // save effect settings to file
  QString file = QFileDialog::getSaveFileName(olive::MainWindow,
                                              tr("Save Effect Settings"),
                                              QString(),
                                              tr("Effect XML Settings %1").arg("(*.xml)"));

  // if the user picked a file
  if (!file.isEmpty()) {

    // ensure file ends with .xml extension
    if (!file.endsWith(".xml", Qt::CaseInsensitive)) {
      file.append(".xml");
    }

    QFile file_handle(file);
    if (file_handle.open(QFile::WriteOnly)) {

      file_handle.write(save_to_string());

      file_handle.close();
    } else {
      QMessageBox::critical(olive::MainWindow,
                            tr("Save Settings Failed"),
                            tr("Failed to open \"%1\" for writing.").arg(file),
                            QMessageBox::Ok);
    }
  }
}

void Effect::load_from_file() {
  // load effect settings from file
  QString file = QFileDialog::getOpenFileName(olive::MainWindow,
                                              tr("Load Effect Settings"),
                                              QString(),
                                              tr("Effect XML Settings %1").arg("(*.xml)"));

  // if the user picked a file
  if (!file.isEmpty()) {
    QFile file_handle(file);
    if (file_handle.open(QFile::ReadOnly)) {

      olive::UndoStack.push(new SetEffectData(this, file_handle.readAll()));

      file_handle.close();

      update_ui(false);
    } else {
      QMessageBox::critical(olive::MainWindow,
                            tr("Load Settings Failed"),
                            tr("Failed to open \"%1\" for reading.").arg(file),
                            QMessageBox::Ok);
    }
  }
}

bool Effect::AlwaysUpdate()
{
  return false;
}

bool Effect::IsEnabled() {
  return enabled_;
}

bool Effect::IsExpanded()
{
  return expanded_;
}

void Effect::SetExpanded(bool e)
{
  expanded_ = e;
}

void Effect::SetEnabled(bool b) {
  enabled_ = b;
  emit EnabledChanged(b);
}

void Effect::load(QXmlStreamReader& stream) {
  int row_count = 0;

  QString tag = stream.name().toString();

  while (!stream.atEnd() && !(stream.name() == tag && stream.isEndElement())) {
    stream.readNext();
    if (stream.name() == "row" && stream.isStartElement()) {
      if (row_count < rows.size()) {
        EffectRow* row = rows.at(row_count);

        while (!stream.atEnd() && !(stream.name() == "row" && stream.isEndElement())) {
          stream.readNext();

          // read field
          if (stream.name() == "field" && stream.isStartElement()) {
            int field_number = -1;

            // match field using ID
            for (int k=0;k<stream.attributes().size();k++) {
              const QXmlStreamAttribute& attr = stream.attributes().at(k);
              if (attr.name() == "id") {
                for (int l=0;l<row->FieldCount();l++) {
                  if (row->Field(l)->id() == attr.value()) {
                    field_number = l;
                    break;
                  }
                }
                break;
              }
            }

            if (field_number > -1) {
              EffectField* field = row->Field(field_number);

              // get current field value
              for (int k=0;k<stream.attributes().size();k++) {
                const QXmlStreamAttribute& attr = stream.attributes().at(k);
                if (attr.name() == "value") {
                  field->persistent_data_ = field->ConvertStringToValue(attr.value().toString());
                  break;
                }
              }

              while (!stream.atEnd() && !(stream.name() == "field" && stream.isEndElement())) {
                stream.readNext();

                // read keyframes
                if (stream.name() == "key" && stream.isStartElement()) {
                  row->SetKeyframingInternal(true);

                  EffectKeyframe key;
                  for (int k=0;k<stream.attributes().size();k++) {
                    const QXmlStreamAttribute& attr = stream.attributes().at(k);
                    if (attr.name() == "value") {
                      key.data = field->ConvertStringToValue(attr.value().toString());
                    } else if (attr.name() == "frame") {
                      key.time = attr.value().toLong();
                    } else if (attr.name() == "type") {
                      key.type = attr.value().toInt();
                    } else if (attr.name() == "prehx") {
                      key.pre_handle_x = attr.value().toDouble();
                    } else if (attr.name() == "prehy") {
                      key.pre_handle_y = attr.value().toDouble();
                    } else if (attr.name() == "posthx") {
                      key.post_handle_x = attr.value().toDouble();
                    } else if (attr.name() == "posthy") {
                      key.post_handle_y = attr.value().toDouble();
                    }
                  }
                  field->keyframes.append(key);
                }
              }

              field->Changed();

            }
          }
        }

      } else {
        qCritical() << "Too many rows for effect" << id << ". Project might be corrupt. (Got" << row_count << ", expected <" << rows.size()-1 << ")";
      }
      row_count++;
    } else if (stream.isStartElement()) {
      custom_load(stream);
    }
  }
}

void Effect::custom_load(QXmlStreamReader &) {}

void Effect::save(QXmlStreamWriter& stream) {
  stream.writeAttribute("name", meta->category + "/" + meta->name);
  stream.writeAttribute("enabled", QString::number(IsEnabled()));

  for (int i=0;i<rows.size();i++) {
    EffectRow* row = rows.at(i);
    if (row->IsSavable()) {
      stream.writeStartElement("row"); // row
      for (int j=0;j<row->FieldCount();j++) {
        EffectField* field = row->Field(j);

        if (!field->id().isEmpty()) {
          stream.writeStartElement("field"); // field
          stream.writeAttribute("id", field->id());
          stream.writeAttribute("value", field->ConvertValueToString(field->persistent_data_));
          for (int k=0;k<field->keyframes.size();k++) {
            const EffectKeyframe& key = field->keyframes.at(k);
            stream.writeStartElement("key");
            stream.writeAttribute("value", field->ConvertValueToString(key.data));
            stream.writeAttribute("frame", QString::number(key.time));
            stream.writeAttribute("type", QString::number(key.type));
            stream.writeAttribute("prehx", QString::number(key.pre_handle_x));
            stream.writeAttribute("prehy", QString::number(key.pre_handle_y));
            stream.writeAttribute("posthx", QString::number(key.post_handle_x));
            stream.writeAttribute("posthy", QString::number(key.post_handle_y));
            stream.writeEndElement(); // key
          }
          stream.writeEndElement(); // field
        }
      }
      stream.writeEndElement(); // row
    }
  }
}

void Effect::load_from_string(const QByteArray &s) {
  // clear existing keyframe data
  for (int i=0;i<rows.size();i++) {
    EffectRow* row = rows.at(i);
    row->SetKeyframingInternal(false);
    for (int j=0;j<row->FieldCount();j++) {
      EffectField* field = row->Field(j);
      field->keyframes.clear();
    }
  }

  // write settings with xml writer
  QXmlStreamReader stream(s);

  while (!stream.atEnd()) {
    stream.readNext();

    // find the effect opening tag
    if (stream.name() == "effect" && stream.isStartElement()) {

      // check the name to see if it matches this effect
      const QXmlStreamAttributes& attributes = stream.attributes();
      for (int i=0;i<attributes.size();i++) {
        const QXmlStreamAttribute& attr = attributes.at(i);
        if (attr.name() == "name") {
          if (get_meta_from_name(attr.value().toString()) == meta) {
            // pass off to standard loading function
            load(stream);
          } else {
            QMessageBox::critical(olive::MainWindow,
                                  tr("Load Settings Failed"),
                                  tr("This settings file doesn't match this effect."),
                                  QMessageBox::Ok);
          }
          break;
        }
      }

      // we've found what we're looking for
      break;
    }
  }
}

QByteArray Effect::save_to_string() {
  QByteArray save_data;

  // write settings to string with xml writer
  QXmlStreamWriter stream(&save_data);

  stream.writeStartDocument();

  stream.writeStartElement("effect");

  // pass off to standard saving function
  save(stream);

  stream.writeEndElement(); // effect

  stream.writeEndDocument();

  return save_data;
}

bool Effect::is_open() {
  return isOpen;
}

void Effect::validate_meta_path() {
  if (!meta->path.isEmpty() || (vertPath.isEmpty() && fragPath.isEmpty())) return;
  QList<QString> effects_paths = get_effects_paths();
  const QString& test_fn = vertPath.isEmpty() ? fragPath : vertPath;
  for (int i=0;i<effects_paths.size();i++) {
    if (QFileInfo::exists(effects_paths.at(i) + "/" + test_fn)) {
      for (int j=0;j<effects.size();j++) {
        if (&effects.at(j) == meta) {
          effects[j].path = effects_paths.at(i);
          return;
        }
      }
      return;
    }
  }
}

void Effect::open() {
  if (isOpen) {
    qWarning() << "Tried to open an effect that was already open";
    close();
  }
  if (olive::CurrentRuntimeConfig.shaders_are_enabled && (Flags() & ShaderFlag)) {
    if (QOpenGLContext::currentContext() == nullptr) {
      qWarning() << "No current context to create a shader program for - will retry next repaint";
    } else {
      glslProgram = new QOpenGLShaderProgram();
      validate_meta_path();
      bool glsl_compiled = true;
      if (!vertPath.isEmpty()) {
        if (glslProgram->addShaderFromSourceFile(QOpenGLShader::Vertex, meta->path + "/" + vertPath)) {
          qInfo() << "Vertex shader added successfully";
        } else {
          glsl_compiled = false;
          qWarning() << "Vertex shader could not be added";
        }
      }
      if (!fragPath.isEmpty()) {
        if (glslProgram->addShaderFromSourceFile(QOpenGLShader::Fragment, meta->path + "/" + fragPath)) {
          qInfo() << "Fragment shader added successfully";
        } else {
          glsl_compiled = false;
          qWarning() << "Fragment shader could not be added";
        }
      }
      if (glsl_compiled) {
        if (glslProgram->link()) {
          qInfo() << "Shader program linked successfully";
        } else {
          qWarning() << "Shader program failed to link";
        }
      }
      isOpen = true;
    }
  } else {
    isOpen = true;
  }
}

void Effect::close() {
  if (!isOpen) {
    qWarning() << "Tried to close an effect that was already closed";
  }
  delete_texture();
  if (glslProgram != nullptr) {
    delete glslProgram;
    glslProgram = nullptr;
  }
  isOpen = false;
}

bool Effect::is_glsl_linked() {
  return glslProgram != nullptr && glslProgram->isLinked();
}

void Effect::startEffect() {
  if (!isOpen) {
    open();
    qWarning() << "Tried to start a closed effect - opening";
  }
  if (olive::CurrentRuntimeConfig.shaders_are_enabled
      && (Flags() & Effect::ShaderFlag)
      && glslProgram->isLinked()) {
    bound = glslProgram->bind();
  }
}

void Effect::endEffect() {
  if (bound) {
    glslProgram->release();
  }
  bound = false;
}

int Effect::Flags()
{
  return flags_;
}

void Effect::SetFlags(int flags)
{
  flags_ = flags;
}

int Effect::getIterations() {
  return iterations;
}

void Effect::setIterations(int i) {
  iterations = i;
}

void Effect::process_image(double, uint8_t *, uint8_t *, int){}

EffectPtr Effect::copy(Clip *c) {
  EffectPtr copy = Effect::Create(c, meta);
  copy->SetEnabled(IsEnabled());
  copy_field_keyframes(copy);
  return copy;
}

void Effect::process_shader(double timecode, GLTextureCoords&, int iteration) {
  glslProgram->setUniformValue("resolution", parent_clip->media_width(), parent_clip->media_height());
  glslProgram->setUniformValue("time", GLfloat(timecode));
  glslProgram->setUniformValue("iteration", iteration);

  for (int i=0;i<rows.size();i++) {
    EffectRow* row = rows.at(i);
    for (int j=0;j<row->FieldCount();j++) {
      EffectField* field = row->Field(j);
      if (!field->id().isEmpty()) {
        switch (field->type()) {
        case EffectField::EFFECT_FIELD_DOUBLE:
        {
          DoubleField* double_field = static_cast<DoubleField*>(field);
          glslProgram->setUniformValue(double_field->id().toUtf8().constData(),
                                       GLfloat(double_field->GetDoubleAt(timecode)));
        }
          break;
        case EffectField::EFFECT_FIELD_COLOR:
        {
          ColorField* color_field = static_cast<ColorField*>(field);
          glslProgram->setUniformValue(
                color_field->id().toUtf8().constData(),
                GLfloat(color_field->GetColorAt(timecode).redF()),
                GLfloat(color_field->GetColorAt(timecode).greenF()),
                GLfloat(color_field->GetColorAt(timecode).blueF())
                );
        }
          break;
        case EffectField::EFFECT_FIELD_BOOL:
          glslProgram->setUniformValue(field->id().toUtf8().constData(), field->GetValueAt(timecode).toBool());
          break;
        case EffectField::EFFECT_FIELD_COMBO:
          glslProgram->setUniformValue(field->id().toUtf8().constData(), field->GetValueAt(timecode).toInt());
          break;

          // can you even send a string to a uniform value?
        case EffectField::EFFECT_FIELD_STRING:
        case EffectField::EFFECT_FIELD_FONT:
        case EffectField::EFFECT_FIELD_FILE:
        case EffectField::EFFECT_FIELD_UI:
          break;
        }
      }
    }
  }
}

void Effect::process_coords(double, GLTextureCoords&, int) {}

GLuint Effect::process_superimpose(double timecode) {
  bool dimensions_changed = false;
  bool redrew_image = false;

  int width = parent_clip->media_width();
  int height = parent_clip->media_height();

  if (width != img.width() || height != img.height()) {
    img = QImage(width, height, QImage::Format_RGBA8888_Premultiplied);
    dimensions_changed = true;
  }

  if (valueHasChanged(timecode) || dimensions_changed || AlwaysUpdate()) {
    redraw(timecode);
    redrew_image = true;
  }

  if (texture == nullptr || texture->width() != img.width() || texture->height() != img.height()) {

    delete_texture();

    texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setSize(img.width(), img.height());
    texture->setFormat(QOpenGLTexture::RGBA8_UNorm);
    texture->setMipLevels(texture->maximumMipLevels());
    texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    texture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8);

    redrew_image = true;

  }

  if (redrew_image) {
    texture->setData(0, QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, img.constBits());
  }

  return texture->textureId();
}

void Effect::process_audio(double, double, quint8*, int, int) {}

void Effect::gizmo_draw(double, GLTextureCoords &) {}

void Effect::gizmo_move(EffectGizmo* gizmo, int x_movement, int y_movement, double timecode, bool done) {
  // Loop through each gizmo to find `gizmo`
  for (int i=0;i<gizmos.size();i++) {

    if (gizmos.at(i) == gizmo) {

      // If (!done && gizmo_dragging_actions_.isEmpty()), that means the drag just started and we're going to save
      // the current state of the attach fields' keyframes in KeyframeDataChange objects to make the changes undoable
      // by the user later.
      if (!done && gizmo_dragging_actions_.isEmpty()) {
        if (gizmo->x_field1 != nullptr) {
          gizmo_dragging_actions_.append(new KeyframeDataChange(gizmo->x_field1));
        }
        if (gizmo->y_field1 != nullptr) {
          gizmo_dragging_actions_.append(new KeyframeDataChange(gizmo->y_field1));
        }
        if (gizmo->x_field2 != nullptr) {
          gizmo_dragging_actions_.append(new KeyframeDataChange(gizmo->x_field2));
        }
        if (gizmo->y_field2 != nullptr) {
          gizmo_dragging_actions_.append(new KeyframeDataChange(gizmo->y_field2));
        }
      }

      // Update the field values
      if (gizmo->x_field1 != nullptr) {
        gizmo->x_field1->SetValueAt(timecode,
                                    gizmo->x_field1->GetDoubleAt(timecode) + x_movement*gizmo->x_field_multi1);
      }
      if (gizmo->y_field1 != nullptr) {
        gizmo->y_field1->SetValueAt(timecode,
                                    gizmo->y_field1->GetDoubleAt(timecode) + y_movement*gizmo->y_field_multi1);
      }
      if (gizmo->x_field2 != nullptr) {
        gizmo->x_field2->SetValueAt(timecode,
                                    gizmo->x_field2->GetDoubleAt(timecode) + x_movement*gizmo->x_field_multi2);
      }
      if (gizmo->y_field2 != nullptr) {
        gizmo->y_field2->SetValueAt(timecode,
                                    gizmo->y_field2->GetDoubleAt(timecode) + y_movement*gizmo->y_field_multi2);
      }

      // If (done && !gizmo_dragging_actions_.isEmpty()), that means the drag just ended and we're going to save
      // the new state of the attach fields' keyframes in KeyframeDataChange objects to make the changes undoable
      // by the user later.
      if (done && !gizmo_dragging_actions_.isEmpty()) {

        // Store all the KeyframeDataChange objects into a ComboAction to send to the undo stack (makes them all
        // undoable together rather than having to be undone individually).
        ComboAction* ca = new ComboAction();

        for (int j=0;j<gizmo_dragging_actions_.size();j++) {
          // Set the current state of the keyframes as the "new" keyframes (the old values were set earlier when the
          // KeyframeDataChange object was constructed).
          gizmo_dragging_actions_.at(j)->SetNewKeyframes();

          // Add this KeyframeDataChange object to the ComboAction
          ca->append(gizmo_dragging_actions_.at(j));
        }

        olive::UndoStack.push(ca);

        gizmo_dragging_actions_.clear();
      }
      break;
    }
  }
}

void Effect::gizmo_world_to_screen() {
  GLfloat view_val[16];
  GLfloat projection_val[16];
  glGetFloatv(GL_MODELVIEW_MATRIX, view_val);
  glGetFloatv(GL_PROJECTION_MATRIX, projection_val);

  QMatrix4x4 view_matrix(view_val);
  QMatrix4x4 projection_matrix(projection_val);

  for (int i=0;i<gizmos.size();i++) {
    EffectGizmo* g = gizmos.at(i);

    for (int j=0;j<g->get_point_count();j++) {
      QVector4D screen_pos = QVector4D(g->world_pos[j].x(), g->world_pos[j].y(), 0, 1.0) * (view_matrix * projection_matrix);

      int adjusted_sx1 = qRound(((screen_pos.x()*0.5f)+0.5f)*parent_clip->sequence->width);
      int adjusted_sy1 = qRound((1.0f-((screen_pos.y()*0.5f)+0.5f))*parent_clip->sequence->height);

      g->screen_pos[j] = QPoint(adjusted_sx1, adjusted_sy1);
    }
  }
}

bool Effect::are_gizmos_enabled() {
  return (gizmos.size() > 0);
}

void Effect::redraw(double) {
  /*
  // run javascript
  QPainter p(&img);
  painter_wrapper.img = &img;
  painter_wrapper.painter = &p;

  jsEngine.globalObject().setProperty("painter", wrapper_obj);
  jsEngine.globalObject().setProperty("width", parent_clip->media_width());
  jsEngine.globalObject().setProperty("height", parent_clip->media_height());

  for (int i=0;i<rows.size();i++) {
    EffectRow* row = rows.at(i);
    for (int j=0;j<row->fieldCount();j++) {
      EffectField* field = row->field(j);
      if (!field->id.isEmpty()) {
        switch (field->type) {
        case EFFECT_FIELD_DOUBLE:
          jsEngine.globalObject().setProperty(field->id, field->get_double_value(timecode));
          break;
        case EFFECT_FIELD_COLOR:
          jsEngine.globalObject().setProperty(field->id, field->get_color_value(timecode).name());
          break;
        case EFFECT_FIELD_STRING:
          jsEngine.globalObject().setProperty(field->id, field->get_string_value(timecode));
          break;
        case EFFECT_FIELD_BOOL:
          jsEngine.globalObject().setProperty(field->id, field->get_bool_value(timecode));
          break;
        case EFFECT_FIELD_COMBO:
          jsEngine.globalObject().setProperty(field->id, field->get_combo_index(timecode));
          break;
        case EFFECT_FIELD_FONT:
          jsEngine.globalObject().setProperty(field->id, field->get_font_name(timecode));
          break;
        }
      }
    }
  }

  jsEngine.evaluate(script);
  */
}

bool Effect::valueHasChanged(double timecode) {
  if (cachedValues.size() == 0) {

    for (int i=0;i<row_count();i++) {
      EffectRow* crow = row(i);
      for (int j=0;j<crow->FieldCount();j++) {
        cachedValues.append(crow->Field(j)->GetValueAt(timecode));
      }
    }
    return true;

  } else {

    bool changed = false;
    int index = 0;
    for (int i=0;i<row_count();i++) {
      EffectRow* crow = row(i);
      for (int j=0;j<crow->FieldCount();j++) {
        EffectField* field = crow->Field(j);
        if (cachedValues.at(index) != field->GetValueAt(timecode)) {
          changed = true;
        }
        cachedValues[index] = field->GetValueAt(timecode);
        index++;
      }
    }
    return changed;

  }
}

void Effect::delete_texture() {
  delete texture;
  texture = nullptr;
}

const EffectMeta* get_meta_from_name(const QString& input) {
  int split_index = input.indexOf('/');
  QString category;
  if (split_index > -1) {
    category = input.left(split_index);
  }
  QString name = input.mid(split_index + 1);

  for (int j=0;j<effects.size();j++) {
    if (effects.at(j).name == name
        && (effects.at(j).category == category
            || category.isEmpty())) {
      return &effects.at(j);
    }
  }
  return nullptr;
}

qint16 mix_audio_sample(qint16 a, qint16 b) {
  qint32 mixed_sample = static_cast<qint32>(a) + static_cast<qint32>(b);
  mixed_sample = qMax(qMin(mixed_sample, static_cast<qint32>(INT16_MAX)), static_cast<qint32>(INT16_MIN));
  return static_cast<qint16>(mixed_sample);
}
