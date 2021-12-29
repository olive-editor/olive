/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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

#include "project.h"

#include <QDir>
#include <QFileInfo>

#include "common/xmlutils.h"
#include "core.h"
#include "dialog/progress/progress.h"
#include "node/factory.h"
#include "render/diskmanager.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

Project::Project() :
  is_modified_(false),
  autorecovery_saved_(true)
{
  // Generate UUID for this project
  RegenerateUuid();

  // Folder root for project
  root_ = new Folder();
  root_->setParent(this);
  root_->SetLabel(tr("Root"));
  root_->SetCanBeDeleted(false);

  // Adds a color manager "node" to this project so that it synchronizes
  color_manager_ = new ColorManager();
  color_manager_->setParent(this);
  color_manager_->SetCanBeDeleted(false);
  AddDefaultNode(color_manager_);

  // Same with project settings
  settings_ = new ProjectSettingsNode();
  settings_->setParent(this);
  settings_->SetCanBeDeleted(false);
  AddDefaultNode(settings_);

  connect(color_manager(), &ColorManager::ValueChanged,
          this, &Project::ColorManagerValueChanged);
}

void Project::Load(QXmlStreamReader *reader, MainWindowLayoutInfo* layout, uint version, const QAtomicInt* cancelled)
{
  XMLNodeData xml_node_data;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("root")) {

      root_->Load(reader, xml_node_data, version, cancelled);

    } else if (reader->name() == QStringLiteral("layout")) {

      // Since the main window's functions have to occur in the GUI thread (and we're likely
      // loading in a secondary thread), we load all necessary data into a separate struct so we
      // can continue loading and queue it with the main window so it can handle the data
      // appropriately in its own thread.

      *layout = MainWindowLayoutInfo::fromXml(reader, xml_node_data);

    } else if (reader->name() == QStringLiteral("uuid")) {

      uuid_ = QUuid::fromString(reader->readElementText());

    } else if (reader->name() == QStringLiteral("nodes")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("node")) {
          bool is_root = false;
          bool is_cm = false;
          bool is_settings = false;
          QString id;

          {
            XMLAttributeLoop(reader, attr) {
              if (attr.name() == QStringLiteral("id")) {
                id = attr.value().toString();
              } else if (attr.name() == QStringLiteral("root") && attr.value() == QStringLiteral("1")) {
                is_root = true;
              } else if (attr.name() == QStringLiteral("cm") && attr.value() == QStringLiteral("1")) {
                is_cm = true;
              } else if (attr.name() == QStringLiteral("settings") && attr.value() == QStringLiteral("1")) {
                is_settings = true;
              }
            }
          }

          if (id.isEmpty()) {
            qWarning() << "Failed to load node with empty ID";
            reader->skipCurrentElement();
          } else {
            Node* node;

            if (is_root) {
              node = root_;
            } else if (is_cm) {
              node = color_manager_;
            } else if (is_settings) {
              node = settings_;
            } else {
              node = NodeFactory::CreateFromID(id);
            }

            if (!node) {
              qWarning() << "Failed to find node with ID" << id;
              reader->skipCurrentElement();
            } else {
              node->Load(reader, xml_node_data, version, cancelled);
              node->setParent(this);
            }
          }
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("positions")) {

      while (XMLReadNextStartElement(reader)) {

        if (reader->name() == QStringLiteral("context")) {

          quintptr context_ptr = 0;
          XMLAttributeLoop(reader, attr) {
            if (attr.name() == QStringLiteral("ptr")) {
              context_ptr = attr.value().toULongLong();
              break;
            }
          }

          Node *context = xml_node_data.node_ptrs.value(context_ptr);

          if (!context) {
            qWarning() << "Failed to find pointer for context";
            reader->skipCurrentElement();
          } else {
            while (XMLReadNextStartElement(reader)) {
              if (reader->name() == QStringLiteral("node")) {
                quintptr node_ptr;
                Node::Position node_pos;

                if (LoadPosition(reader, &node_ptr, &node_pos)) {
                  Node *node = xml_node_data.node_ptrs.value(node_ptr);

                  if (node) {
                    context->SetNodePositionInContext(node, node_pos);
                  } else {
                    qWarning() << "Failed to find pointer for node position";
                    reader->skipCurrentElement();
                  }
                }
              } else {
                reader->skipCurrentElement();
              }
            }
          }

        } else {

          reader->skipCurrentElement();

        }

      }

    } else {

      // Skip this
      reader->skipCurrentElement();

    }
  }

  // Make connections
  xml_node_data.PostConnect(version);
}

void Project::Save(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("uuid"), uuid_.toString());

  writer->writeStartElement(QStringLiteral("nodes"));

  foreach (Node* node, nodes()) {
    writer->writeStartElement(QStringLiteral("node"));

    if (node == root_) {
      writer->writeAttribute(QStringLiteral("root"), QStringLiteral("1"));
    } else if (node == color_manager_) {
      writer->writeAttribute(QStringLiteral("cm"), QStringLiteral("1"));
    } else if (node == settings_) {
      writer->writeAttribute(QStringLiteral("settings"), QStringLiteral("1"));
    }

    writer->writeAttribute(QStringLiteral("id"), node->id());

    node->Save(writer);

    writer->writeEndElement(); // node
  }

  writer->writeEndElement(); // nodes

  writer->writeStartElement(QStringLiteral("positions"));

  foreach (Node* context, nodes()) {
    const Node::PositionMap &map = context->GetContextPositions();

    if (!map.isEmpty()) {
      writer->writeStartElement(QStringLiteral("context"));

      writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(context)));

      for (auto jt=map.cbegin(); jt!=map.cend(); jt++) {
        writer->writeStartElement(QStringLiteral("node"));
        SavePosition(writer, jt.key(), jt.value());
        writer->writeEndElement(); // node
      }

      writer->writeEndElement(); // context
    }
  }

  writer->writeEndElement(); // positions

  // Save main window project layout
  MainWindowLayoutInfo main_window_info = Core::instance()->main_window()->SaveLayout();
  main_window_info.toXml(writer);
}

Folder *Project::root()
{
  return root_;
}

QString Project::name() const
{
  if (filename_.isEmpty()) {
    return tr("(untitled)");
  } else {
    return QFileInfo(filename_).completeBaseName();
  }
}

const QString &Project::filename() const
{
  return filename_;
}

QString Project::pretty_filename() const
{
  QString fn = filename();

  if (fn.isEmpty()) {
    return tr("(untitled)");
  } else {
    return fn;
  }
}

void Project::set_filename(const QString &s)
{
  filename_ = s;

#ifdef Q_OS_WINDOWS
  // Prevents filenames
  filename_.replace('/', '\\');
#endif

  emit NameChanged();
}

void Project::set_modified(bool e)
{
  is_modified_ = e;
  set_autorecovery_saved(!e);

  emit ModifiedChanged(is_modified_);
}

bool Project::has_autorecovery_been_saved() const
{
  return autorecovery_saved_;
}

void Project::set_autorecovery_saved(bool e)
{
  autorecovery_saved_ = e;
}

bool Project::is_new() const
{
  return !is_modified_ && filename_.isEmpty();
}

QString Project::cache_path() const
{
  ProjectSettingsNode::CacheSetting setting = settings_->GetCacheSetting();

  switch (setting) {
  case ProjectSettingsNode::kCacheUseDefaultLocation:
    break;
  case ProjectSettingsNode::kCacheCustomPath:
  {
    QString cache_path = settings_->GetCustomCachePath();

    if (cache_path.isEmpty()) {
      return cache_path;
    }
    break;
  }
  case ProjectSettingsNode::kCacheStoreAlongsideProject:
  {
    if (!filename_.isEmpty()) {
      return QFileInfo(filename_).path();
    }
    break;
  }
  }

  return DiskManager::instance()->GetDefaultCachePath();
}

void Project::RegenerateUuid()
{
  uuid_ = QUuid::createUuid();
}

bool Project::LoadPosition(QXmlStreamReader *reader, quintptr *node_ptr, Node::Position *pos)
{
  bool got_node_ptr = false;
  bool got_pos_x = false;
  bool got_pos_y = false;

  XMLAttributeLoop(reader, attr) {
    if (attr.name() == QStringLiteral("ptr")) {
      *node_ptr = attr.value().toULongLong();
      got_node_ptr = true;
      break;
    }
  }

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("x")) {
      pos->position.setX(reader->readElementText().toDouble());
      got_pos_x = true;
    } else if (reader->name() == QStringLiteral("y")) {
      pos->position.setY(reader->readElementText().toDouble());
      got_pos_y = true;
    } else if (reader->name() == QStringLiteral("expanded")) {
      pos->expanded = reader->readElementText().toInt();
    } else {
      reader->skipCurrentElement();
    }
  }

  return got_node_ptr && got_pos_x && got_pos_y;
}

void Project::SavePosition(QXmlStreamWriter *writer, Node *node, const Node::Position &pos)
{
  writer->writeAttribute(QStringLiteral("ptr"), QString::number(reinterpret_cast<quintptr>(node)));

  writer->writeTextElement(QStringLiteral("x"), QString::number(pos.position.x()));
  writer->writeTextElement(QStringLiteral("y"), QString::number(pos.position.y()));
  writer->writeTextElement(QStringLiteral("expanded"), QString::number(pos.expanded));
}

void Project::ColorManagerValueChanged(const NodeInput &input, const TimeRange &range)
{
  Q_UNUSED(input)
  Q_UNUSED(range)

  QVector<Footage*> footage = root()->ListChildrenOfType<Footage>();

  foreach (Footage* item, footage) {
    item->InvalidateAll(QString());
  }
}

}
