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
  // Adds a color manager "node" to this project so that it synchronizes
  color_manager_ = new ColorManager();
  color_manager_->setParent(this);

  // Folder root for project
  root_ = new Folder();
  root_->setParent(this);
  root_->SetLabel(tr("Root"));

  connect(color_manager(), &ColorManager::ValueChanged,
          this, &Project::ColorManagerValueChanged);
}

void Project::Load(QXmlStreamReader *reader, MainWindowLayoutInfo* layout, uint version, const QAtomicInt* cancelled)
{
  XMLNodeData xml_node_data;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("root")) {

      root_->Load(reader, xml_node_data, version, cancelled);

    } else if (reader->name() == QStringLiteral("cachepath")) {

      set_cache_path(reader->readElementText());

    } else if (reader->name() == QStringLiteral("layout")) {

      // Since the main window's functions have to occur in the GUI thread (and we're likely
      // loading in a secondary thread), we load all necessary data into a separate struct so we
      // can continue loading and queue it with the main window so it can handle the data
      // appropriately in its own thread.

      *layout = MainWindowLayoutInfo::fromXml(reader, xml_node_data);

    } else if (reader->name() == QStringLiteral("nodes")) {

      while (XMLReadNextStartElement(reader)) {
        if (reader->name() == QStringLiteral("node")) {
          bool is_root = false;
          QString id;

          {
            XMLAttributeLoop(reader, attr) {
              if (attr.name() == QStringLiteral("id")) {
                id = attr.value().toString();
              } else if (attr.name() == QStringLiteral("root") && attr.value() == QStringLiteral("1")) {
                is_root = true;
              }
            }
          }

          if (id.isEmpty()) {
            qWarning() << "Failed to load node with empty ID";
          } else {
            Node* node;

            if (is_root) {
              node = root_;
            } else {
              node = NodeFactory::CreateFromID(id);
            }

            if (!node) {
              qWarning() << "Failed to find node with ID" << id;
            } else {
              node->Load(reader, xml_node_data, version, cancelled);
              node->setParent(this);
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
  XMLConnectNodes(xml_node_data);

  // Link blocks
  XMLLinkBlocks(xml_node_data);
}

void Project::Save(QXmlStreamWriter *writer) const
{
  writer->writeTextElement(QStringLiteral("cachepath"), cache_path(false));

  writer->writeStartElement(QStringLiteral("nodes"));

  foreach (Node* node, nodes()) {
    writer->writeStartElement(QStringLiteral("node"));

    if (node == root_) {
      writer->writeAttribute(QStringLiteral("root"), QStringLiteral("1"));
    }

    writer->writeAttribute(QStringLiteral("id"), node->id());

    node->Save(writer);

    writer->writeEndElement(); // node
  }

  writer->writeEndElement(); // nodes

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

ColorManager *Project::color_manager()
{
  return color_manager_;
}

bool Project::is_modified() const
{
  return is_modified_;
}

void Project::set_modified(bool e)
{
  is_modified_ = e;
  autorecovery_saved_ = !e;

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

const QString &Project::cache_path(bool default_if_empty) const
{
  if (cache_path_.isEmpty() && default_if_empty) {
    return DiskManager::instance()->GetDefaultCachePath();
  }
  return cache_path_;
}

void Project::ColorManagerValueChanged(const NodeInput &input, const TimeRange &range)
{
  Q_UNUSED(range)

  QVector<Footage*> footage = root()->ListOutputsOfType<Footage>();

  foreach (Footage* item, footage) {
    item->InvalidateAll(QString());
    //static_cast<VideoStream*>(s)->ColorConfigChanged();
    //static_cast<VideoStream*>(s)->DefaultColorSpaceChanged();
  }
}

}
