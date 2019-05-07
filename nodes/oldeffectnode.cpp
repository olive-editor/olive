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

#include "oldeffectnode.h"

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
#include "ui/menu.h"
#include "global/math.h"
#include "global/clipboard.h"
#include "global/config.h"
#include "effects/transition.h"
#include "undo/undostack.h"
#include "rendering/shadergenerators.h"
#include "global/timing.h"
#include "nodes/nodes.h"
#include "effects/effectloaders.h"

QVector<OldEffectNodePtr> olive::node_library;

OldEffectNode::OldEffectNode(Clip* c) :
  parent_clip(c),
  flags_(0),
  shader_program_(nullptr),
  texture(0),
  tex_width_(0),
  tex_height_(0),
  isOpen(false),
  bound(false),
  iterations(1),
  enabled_(true),
  expanded_(true),
  texture_ctx(nullptr)
{
}

OldEffectNode::~OldEffectNode() {
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

QString OldEffectNode::category()
{
  return QString();
}

QString OldEffectNode::description()
{
  return QString();
}

bool OldEffectNode::IsCreatable()
{
  return true;
}

void OldEffectNode::AddRow(NodeIO *row)
{
  row->setParent(this);
  rows.append(row);
}

int OldEffectNode::IndexOfRow(NodeIO *row)
{
  return rows.indexOf(row);
}

void OldEffectNode::copy_field_keyframes(OldEffectNodePtr e) {
  for (int i=0;i<rows.size();i++) {
    NodeIO* row = rows.at(i);
    NodeIO* copy_row = e->rows.at(i);
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

NodeIO* OldEffectNode::row(int i) {
  return rows.at(i);
}

int OldEffectNode::row_count() {
  return rows.size();
}

EffectGizmo *OldEffectNode::add_gizmo(int type) {
  EffectGizmo* gizmo = new EffectGizmo(this, type);
  gizmos.append(gizmo);
  return gizmo;
}

EffectGizmo *OldEffectNode::gizmo(int i) {
  return gizmos.at(i);
}

int OldEffectNode::gizmo_count() {
  return gizmos.size();
}

QVector<NodeEdgePtr> OldEffectNode::GetAllEdges()
{
  QVector<NodeEdgePtr> edges;

  for (int i=0;i<row_count();i++) {
    edges.append(row(i)->edges());
  }

  return edges;
}

void OldEffectNode::refresh() {}

void OldEffectNode::FieldChanged() {
  // Update the UI if a field has been modified, but don't both if this effect is inactive
  if (parent_clip != nullptr) {
    update_ui(false);
  }
}

void OldEffectNode::delete_self() {
  olive::undo_stack.push(new EffectDeleteCommand(this));
  update_ui(true);
}

void OldEffectNode::move_up() {
  int index_of_effect = parent_clip->IndexOfEffect(this);
  if (index_of_effect == 0) {
    return;
  }

  MoveEffectCommand* command = new MoveEffectCommand();
  command->clip = parent_clip;
  command->from = index_of_effect;
  command->to = command->from - 1;
  olive::undo_stack.push(command);
  panel_effect_controls->Reload();
  panel_sequence_viewer->viewer_widget()->frame_update();
}

void OldEffectNode::move_down() {
  int index_of_effect = parent_clip->IndexOfEffect(this);
  if (index_of_effect == parent_clip->effects.size()-1) {
    return;
  }

  MoveEffectCommand* command = new MoveEffectCommand();
  command->clip = parent_clip;
  command->from = index_of_effect;
  command->to = command->from + 1;
  olive::undo_stack.push(command);
  panel_effect_controls->Reload();
  panel_sequence_viewer->viewer_widget()->frame_update();
}

void OldEffectNode::save_to_file() {
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

void OldEffectNode::load_from_file() {
  // load effect settings from file
  QString file = QFileDialog::getOpenFileName(olive::MainWindow,
                                              tr("Load Effect Settings"),
                                              QString(),
                                              tr("Effect XML Settings %1").arg("(*.xml)"));

  // if the user picked a file
  if (!file.isEmpty()) {
    QFile file_handle(file);
    if (file_handle.open(QFile::ReadOnly)) {

      olive::undo_stack.push(new SetEffectData(this, file_handle.readAll()));

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

bool OldEffectNode::AlwaysUpdate()
{
  return false;
}

bool OldEffectNode::IsEnabled() {
  return enabled_;
}

bool OldEffectNode::IsExpanded()
{
  return expanded_;
}

void OldEffectNode::SetExpanded(bool e)
{
  expanded_ = e;
}

void OldEffectNode::SetPos(const QPointF &pos)
{
  pos_ = pos;
}

void OldEffectNode::SetEnabled(bool b) {
  enabled_ = b;
  emit EnabledChanged(b);
}

void OldEffectNode::load(QXmlStreamReader& stream) {
  /*
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
  */
}

void OldEffectNode::custom_load(QXmlStreamReader &) {}

void OldEffectNode::save(QXmlStreamWriter& stream) {
  /*
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
  */
}

void OldEffectNode::load_from_string(const QByteArray &s) {
  // clear existing keyframe data
  for (int i=0;i<rows.size();i++) {
    NodeIO* row = rows.at(i);
    row->SetKeyframingInternal(false);
    for (int j=0;j<row->FieldCount();j++) {
      EffectField* field = row->Field(j);
      field->keyframes.clear();
    }
  }

  // write settings with xml writer
  QXmlStreamReader stream(s);

  bool found_id = false;

  while (!stream.atEnd()) {
    stream.readNext();

    // find the effect opening tag
    if (stream.name() == "effect" && stream.isStartElement()) {

      // check the name to see if it matches this effect
      const QXmlStreamAttributes& attributes = stream.attributes();
      for (int i=0;i<attributes.size();i++) {
        const QXmlStreamAttribute& attr = attributes.at(i);
        if (attr.name() == "id") {
          load(stream);
          found_id = true;
          break;
        }
      }

      // we've found what we're looking for
      break;
    }
  }

  if (!found_id) {
    QMessageBox::critical(olive::MainWindow,
                          tr("Load Settings Failed"),
                          tr("This settings file doesn't match this effect."),
                          QMessageBox::Ok);
  }
}

QByteArray OldEffectNode::save_to_string() {
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

bool OldEffectNode::is_open() {
  return isOpen;
}

void OldEffectNode::validate_meta_path() {
  /*
  if (!meta->path.isEmpty() || (shader_vert_path_.isEmpty() && shader_frag_path_.isEmpty())) return;
  QList<QString> effects_paths = get_effects_paths();
  const QString& test_fn = shader_vert_path_.isEmpty() ? shader_frag_path_ : shader_vert_path_;
  for (int i=0;i<effects_paths.size();i++) {
    if (QFileInfo::exists(effects_paths.at(i) + "/" + test_fn)) {
      for (int j=0;j<olive::effects.size();j++) {
        if (&olive::effects.at(j) == meta) {
          olive::effects[j].path = effects_paths.at(i);
          return;
        }
      }
      return;
    }
  }
  */
}

void OldEffectNode::open() {
  /*
  if (isOpen) {
    qWarning() << "Tried to open an effect that was already open";
    close();
  }
  if (olive::runtime_config.shaders_are_enabled && (Flags() & ShaderFlag)) {
    if (QOpenGLContext::currentContext() == nullptr) {
      qWarning() << "No current context to create a shader program for - will retry next repaint";
    } else {
      validate_meta_path();

      QString frag_shader_str;
      QString frag_file_url = QDir(file).filePath(shader_frag_path_);
      QFile frag_file(frag_file_url);
      if (frag_file.open(QFile::ReadOnly)) {
        frag_shader_str = frag_file.readAll();
        frag_file.close();
      } else {
        qWarning() << "Failed to open" << frag_file_url;
      }

      if (!frag_shader_str.isEmpty()) {

        QString shader_func;

        if (!shader_function_name_.isEmpty()) {
          shader_func = shader_function_name_;
        } else {
          shader_func = "process";
        }

        shader_program_ = olive::shader::GetPipeline(shader_func, frag_shader_str);

      }
      isOpen = true;
    }
  } else {
    isOpen = true;
  }
  */
  isOpen = true;
}

void OldEffectNode::close() {
  if (!isOpen) {
    qWarning() << "Tried to close an effect that was already closed";
  }
  delete_texture();
  shader_program_ = nullptr;
  isOpen = false;
}

bool OldEffectNode::is_shader_linked() {
  return shader_program_ != nullptr && shader_program_->isLinked();
}

QOpenGLShaderProgram *OldEffectNode::GetShaderPipeline()
{
  return shader_program_.get();
}

int OldEffectNode::Flags()
{
  return flags_;
}

void OldEffectNode::SetFlags(int flags)
{
  flags_ = flags;
}

int OldEffectNode::getIterations() {
  return iterations;
}

void OldEffectNode::setIterations(int i) {
  iterations = i;
}

const QPointF &OldEffectNode::pos()
{
  return pos_;
}

void OldEffectNode::process_image(double, uint8_t *, uint8_t *, int){}

OldEffectNodePtr OldEffectNode::copy(Clip *c) {
  OldEffectNodePtr copy = Create(c);
  copy->SetEnabled(IsEnabled());
  copy_field_keyframes(copy);
  return copy;
}

void OldEffectNode::process_shader(double timecode, GLTextureCoords&, int iteration) {
  /*
  shader_program_->bind();

  shader_program_->setUniformValue("resolution", parent_clip->media_width(), parent_clip->media_height());
  shader_program_->setUniformValue("time", GLfloat(timecode));
  shader_program_->setUniformValue("iteration", iteration);

  for (int i=0;i<rows.size();i++) {
    EffectRow* row = rows.at(i);

    for (int j=0;j<row->FieldCount();j++) {
      EffectField* field = row->Field(j);
      if (!field->id().isEmpty()) {
        switch (field->type()) {
        case EffectField::EFFECT_FIELD_DOUBLE:
        {
          DoubleField* double_field = static_cast<DoubleField*>(field);
          shader_program_->setUniformValue(double_field->id().toUtf8().constData(),
                                           GLfloat(double_field->GetDoubleAt(timecode)));
        }
          break;
        case EffectField::EFFECT_FIELD_COLOR:
        {
          ColorField* color_field = static_cast<ColorField *>(field);
          shader_program_->setUniformValue(
                color_field->id().toUtf8().constData(),
                GLfloat(color_field->GetColorAt(timecode).redF()),
                GLfloat(color_field->GetColorAt(timecode).greenF()),
                GLfloat(color_field->GetColorAt(timecode).blueF())
                );
        }
          break;
        case EffectField::EFFECT_FIELD_BOOL:
          shader_program_->setUniformValue(field->id().toUtf8().constData(), field->GetValueAt(timecode).toBool());
          break;
        case EffectField::EFFECT_FIELD_COMBO:
          shader_program_->setUniformValue(field->id().toUtf8().constData(), field->GetValueAt(timecode).toInt());
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

  shader_program_->release();
  */
}

void OldEffectNode::process_coords(double, GLTextureCoords&, int) {}

GLuint OldEffectNode::process_superimpose(QOpenGLContext* ctx, double timecode) {
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

  QOpenGLFunctions* f = ctx->functions();

  if (texture == 0 || tex_width_ != img.width() || tex_height_ != img.height()) {
    delete_texture();

    tex_width_ = img.width();
    tex_height_ = img.height();

    // create texture object
    f->glGenTextures(1, &texture);

    f->glBindTexture(GL_TEXTURE_2D, texture);

    // set texture filtering to bilinear
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    f->glTexImage2D(
          GL_TEXTURE_2D, 0, GL_RGBA8, tex_width_, tex_height_, 0, GL_RGBA,  GL_UNSIGNED_BYTE, img.constBits()
          );

    f->glBindTexture(GL_TEXTURE_2D, 0);

    redrew_image = false;
  }

  if (redrew_image) {
    f->glBindTexture(GL_TEXTURE_2D, texture);

    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex_width_, tex_height_,  GL_RGBA, GL_UNSIGNED_BYTE, img.constBits());

    f->glBindTexture(GL_TEXTURE_2D, 0);
  }

  return texture;
}

void OldEffectNode::process_audio(double, double, float **, int, int, int) {}

void OldEffectNode::gizmo_draw(double, GLTextureCoords &) {}

void OldEffectNode::gizmo_move(EffectGizmo* gizmo, int x_movement, int y_movement, double timecode, bool done) {
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

        olive::undo_stack.push(ca);

        gizmo_dragging_actions_.clear();
      }
      break;
    }
  }
}

void OldEffectNode::gizmo_world_to_screen(const QMatrix4x4& matrix, const QMatrix4x4& projection) {
  for (int i=0;i<gizmos.size();i++) {
    EffectGizmo* g = gizmos.at(i);

    for (int j=0;j<g->get_point_count();j++) {

      // Convert the world point from the gizmo into a screen point relative to the sequence's dimensions

      QVector3D screen_pos = g->world_pos.at(j).project(matrix,
                                                        projection,
                                                        QRect(0,
                                                              0,
                                                              parent_clip->track()->sequence()->width,
                                                              parent_clip->track()->sequence()->height));

      g->screen_pos[j] = QPoint(screen_pos.x(), parent_clip->track()->sequence()->height-screen_pos.y());

    }
  }
}

bool OldEffectNode::are_gizmos_enabled() {
  return (gizmos.size() > 0);
}

double OldEffectNode::Now()
{
  return playhead_to_clip_seconds(parent_clip, parent_clip->track()->sequence()->playhead);
}

long OldEffectNode::NowInFrames()
{
  return playhead_to_clip_frame(parent_clip, parent_clip->track()->sequence()->playhead);
}

void OldEffectNode::redraw(double) {
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
        case EffectField::EFFECT_FIELD_DOUBLE:
          jsEngine.globalObject().setProperty(field->id, field->get_double_value(timecode));
          break;
        case EffectField::EFFECT_FIELD_COLOR:
          jsEngine.globalObject().setProperty(field->id, field->get_color_value(timecode).name());
          break;
        case EffectField::EFFECT_FIELD_STRING:
          jsEngine.globalObject().setProperty(field->id, field->get_string_value(timecode));
          break;
        case EffectField::EFFECT_FIELD_BOOL:
          jsEngine.globalObject().setProperty(field->id, field->get_bool_value(timecode));
          break;
        case EffectField::EFFECT_FIELD_COMBO:
          jsEngine.globalObject().setProperty(field->id, field->get_combo_index(timecode));
          break;
        case EffectField::EFFECT_FIELD_FONT:
          jsEngine.globalObject().setProperty(field->id, field->get_font_name(timecode));
          break;
        }
      }
    }
  }

  jsEngine.evaluate(script);
  */
}

bool OldEffectNode::valueHasChanged(double timecode) {
  if (cachedValues.isEmpty()) {

    for (int i=0;i<row_count();i++) {
      NodeIO* crow = row(i);
      for (int j=0;j<crow->FieldCount();j++) {
        cachedValues.append(crow->Field(j)->GetValueAt(timecode));
      }
    }
    return true;

  } else {

    bool changed = false;
    int index = 0;
    for (int i=0;i<row_count();i++) {
      NodeIO* crow = row(i);
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

void OldEffectNode::delete_texture() {
  if (texture_ctx != nullptr) {
    texture_ctx->functions()->glDeleteTextures(1, &texture);
    texture = 0;
    texture_ctx = nullptr;
  }
}

int GetNodeLibraryIndexFromId(const QString& id) {
  for (int i=0;i<olive::node_library.size();i++) {
    if (olive::node_library.at(i)->id() == id) {
      return i;
    }
  }

  return -1;
}

/*
const EffectMeta* Node::GetMetaFromName(const QString& input) {
  int split_index = input.indexOf('/');
  QString category;
  if (split_index > -1) {
    category = input.left(split_index);
  }
  QString name = input.mid(split_index + 1);

  for (int j=0;j<olive::effects.size();j++) {
    if (olive::effects.at(j).name == name
        && (olive::effects.at(j).category == category
            || category.isEmpty())) {
      return &olive::effects.at(j);
    }
  }
  return nullptr;
}
*/
