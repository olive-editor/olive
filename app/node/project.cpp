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

#include "project.h"

#include <QDir>
#include <QFileInfo>

#include "common/qtutils.h"
#include "common/xmlutils.h"
#include "core.h"
#include "dialog/progress/progress.h"
#include "node/color/ociobase/ociobase.h"
#include "node/factory.h"
#include "node/serializeddata.h"
#include "render/diskmanager.h"
#include "window/mainwindow/mainwindow.h"

namespace olive {

#define super QObject

const QString Project::kCacheLocationSettingKey = QStringLiteral("cachesetting");
const QString Project::kCachePathKey = QStringLiteral("customcachepath");
const QString Project::kColorConfigFilename = QStringLiteral("colorconfigfilename");
const QString Project::kDefaultInputColorSpaceKey = QStringLiteral("defaultinputcolorspace");
const QString Project::kColorReferenceSpace = QStringLiteral("colorreferencespace");

const QString Project::kItemMimeType = QStringLiteral("application/x-oliveprojectitemdata");

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
  AddDefaultNode(root_);

  color_manager_ = new ColorManager(this);
  color_manager_->Init();
}

Project::~Project()
{
  Clear();
}

void Project::Clear()
{
  // By deleting the last nodes first, we assume that nodes that are most important are deleted last
  // (e.g. Project's ColorManager or ProjectSettingsNode.
  for (auto it=node_children_.cbegin(); it!=node_children_.cend(); it++) {
    (*it)->SetCachesEnabled(false);
  }

  while (!node_children_.isEmpty()) {
    delete node_children_.last();
  }
}

SerializedData Project::Load(QXmlStreamReader *reader)
{
  SerializedData data;

  while (XMLReadNextStartElement(reader)) {
    if (reader->name() == QStringLiteral("uuid")) {

      this->SetUuid(QUuid::fromString(reader->readElementText()));

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
            reader->skipCurrentElement();
          } else {
            Node* node;

            if (is_root) {
              node = this->root();
            } else {
              node = NodeFactory::CreateFromID(id);
            }

            if (!node) {
              qWarning() << "Failed to find node with ID" << id;
              reader->skipCurrentElement();
            } else {
              // Disable cache while node is being loaded (we'll re-enable it later)
              node->SetCachesEnabled(false);

              node->Load(reader, &data);

              node->setParent(this);
            }
          }
        } else {
          reader->skipCurrentElement();
        }
      }

    } else if (reader->name() == QStringLiteral("settings")) {
      while (XMLReadNextStartElement(reader)) {
        QString key = reader->name().toString();
        QString val = reader->readElementText();
        SetSetting(key, val);
      }
    } else {

      // Skip this
      reader->skipCurrentElement();

    }
  }

  return data;
}

void Project::Save(QXmlStreamWriter *writer) const
{
  writer->writeAttribute(QStringLiteral("version"), QString::number(1));

  writer->writeTextElement(QStringLiteral("uuid"), this->GetUuid().toString());

  if (!this->nodes().isEmpty()) {
    writer->writeStartElement(QStringLiteral("nodes"));

    foreach (Node* node, this->nodes()) {
      writer->writeStartElement(QStringLiteral("node"));

      if (node == this->root()) {
        writer->writeAttribute(QStringLiteral("root"), QStringLiteral("1"));
      }

      node->Save(writer);

      writer->writeEndElement(); // node
    }

    writer->writeEndElement(); // nodes
  }

  if (!this->settings_.isEmpty()) {
    writer->writeStartElement(QStringLiteral("settings"));

    for (auto it = this->settings_.cbegin(); it != this->settings_.cend(); it++) {
      writer->writeTextElement(it.key(), it.value());
    }

    writer->writeEndElement(); // settings
  }
}

int Project::GetNumberOfContextsNodeIsIn(Node *node, bool except_itself) const
{
  int count = 0;

  foreach (Node *ctx, node_children_) {
    if (ctx->ContextContainsNode(node) && (!except_itself || ctx != node)) {
      count++;
    }
  }

  return count;
}

void Project::childEvent(QChildEvent *event)
{
  super::childEvent(event);

  Node* node = dynamic_cast<Node*>(event->child());

  if (node) {
    if (event->type() == QEvent::ChildAdded) {

      node_children_.append(node);

      // Connect signals
      connect(node, &Node::InputConnected, this, &Project::InputConnected, Qt::DirectConnection);
      connect(node, &Node::InputDisconnected, this, &Project::InputDisconnected, Qt::DirectConnection);
      connect(node, &Node::ValueChanged, this, &Project::ValueChanged, Qt::DirectConnection);
      connect(node, &Node::InputValueHintChanged, this, &Project::InputValueHintChanged, Qt::DirectConnection);

      if (NodeGroup *group = dynamic_cast<NodeGroup*>(node)) {
        connect(group, &NodeGroup::InputPassthroughAdded, this, &Project::GroupAddedInputPassthrough, Qt::DirectConnection);
        connect(group, &NodeGroup::InputPassthroughRemoved, this, &Project::GroupRemovedInputPassthrough, Qt::DirectConnection);
        connect(group, &NodeGroup::OutputPassthroughChanged, this, &Project::GroupChangedOutputPassthrough, Qt::DirectConnection);
      }

      emit NodeAdded(node);
      emit node->AddedToGraph(this);
      node->AddedToGraphEvent(this);

      // Emit input connections
      for (auto it=node->input_connections().cbegin(); it!=node->input_connections().cend(); it++) {
        if (nodes().contains(it->second)) {
          emit InputConnected(it->second, it->first);
        }
      }

      // Emit output connections
      for (auto it=node->output_connections().cbegin(); it!=node->output_connections().cend(); it++) {
        if (nodes().contains(it->second.node())) {
          emit InputConnected(it->first, it->second);
        }
      }

    } else if (event->type() == QEvent::ChildRemoved) {

      node_children_.removeOne(node);

      // Disconnect signals
      disconnect(node, &Node::InputConnected, this, &Project::InputConnected);
      disconnect(node, &Node::InputDisconnected, this, &Project::InputDisconnected);
      disconnect(node, &Node::ValueChanged, this, &Project::ValueChanged);
      disconnect(node, &Node::InputValueHintChanged, this, &Project::InputValueHintChanged);

      if (NodeGroup *group = dynamic_cast<NodeGroup*>(node)) {
        disconnect(group, &NodeGroup::InputPassthroughAdded, this, &Project::GroupAddedInputPassthrough);
        disconnect(group, &NodeGroup::InputPassthroughRemoved, this, &Project::GroupRemovedInputPassthrough);
        disconnect(group, &NodeGroup::OutputPassthroughChanged, this, &Project::GroupChangedOutputPassthrough);
      }

      emit NodeRemoved(node);
      emit node->RemovedFromGraph(this);
      node->RemovedFromGraphEvent(this);

      // Remove from any contexts
      foreach (Node *context, node_children_) {
        context->RemoveNodeFromContext(node);
      }
    }
  }
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

QString Project::get_cache_alongside_project_path() const
{
  if (!filename_.isEmpty()) {
    // Non-translated string so the path doesn't change if the language does
    return QFileInfo(filename_).dir().filePath(QStringLiteral("cache"));
  }
  return QString();
}

QString Project::cache_path() const
{
  CacheSetting setting = GetCacheLocationSetting();

  switch (setting) {
  case kCacheUseDefaultLocation:
    break;
  case kCacheCustomPath:
  {
    QString cache_path = GetCustomCachePath();
    if (cache_path.isEmpty()) {
      return cache_path;
    }
    break;
  }
  case kCacheStoreAlongsideProject:
  {
    QString alongside = get_cache_alongside_project_path();
    if (!alongside.isEmpty()) {
      return alongside;
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

Project *Project::GetProjectFromObject(const QObject *o)
{
  return QtUtils::GetParentOfType<Project>(o);
}

void Project::SetSetting(const QString &key, const QString &value)
{
  settings_.insert(key, value);
  emit SettingChanged(key, value);

  if (key == kColorReferenceSpace) {
    emit color_manager_->ReferenceSpaceChanged(value);
  } else if (key == kColorConfigFilename) {
    color_manager_->UpdateConfigFromFilename();
  } else if (key == kDefaultInputColorSpaceKey) {
    emit color_manager_->DefaultInputChanged(value);
  }
}

}
