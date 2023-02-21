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

#ifndef PROJECT_H
#define PROJECT_H

#include <memory>
#include <QObject>
#include <QUuid>

#include "node/color/colormanager/colormanager.h"
#include "node/output/viewer/viewer.h"
#include "node/project/footage/footage.h"
#include "node/project/projectsettings/projectsettings.h"
#include "window/mainwindow/mainwindowlayoutinfo.h"

namespace olive {

/**
 * @brief A project instance containing all the data pertaining to the user's project
 *
 * A project instance uses a parent-child hierarchy of Item objects. Projects will usually contain the following:
 *
 * * Footage
 * * Sequences
 * * Folders
 * * Project Settings
 * * Window Layout
 */
class Project : public QObject
{
  Q_OBJECT
public:
  Project();

  virtual ~Project() override;

  /**
   * @brief Destructively destroys all nodes in the graph
   */
  void Clear();

  /**
   * @brief Retrieve a complete list of the nodes belonging to this graph
   */
  const QVector<Node*>& nodes() const
  {
    return node_children_;
  }

  const QVector<Node*>& default_nodes() const
  {
    return default_nodes_;
  }

  void Load(QXmlStreamReader *reader);
  void Save(QXmlStreamWriter *writer) const;

  int GetNumberOfContextsNodeIsIn(Node *node, bool except_itself = false) const;

  QString name() const;

  const QString& filename() const;
  QString pretty_filename() const;
  void set_filename(const QString& s);

  Folder* root() const { return root_; }
  ColorManager* color_manager() const { return color_manager_; }
  ProjectSettingsNode* settings() const { return settings_; }

  bool is_modified() const { return is_modified_; }
  void set_modified(bool e);

  bool has_autorecovery_been_saved() const;
  void set_autorecovery_saved(bool e);

  bool is_new() const;

  QString get_cache_alongside_project_path() const;
  QString cache_path() const;

  const QUuid& GetUuid() const
  {
    return uuid_;
  }

  void SetUuid(const QUuid &uuid)
  {
    uuid_ = uuid;
  }

  void RegenerateUuid();

  /**
   * @brief Returns the filename the project was saved as, but not necessarily where it is now
   *
   * May help for resolving relative paths.
   */
  const QString &GetSavedURL() const
  {
    return saved_url_;
  }

  void SetSavedURL(const QString &url)
  {
    saved_url_ = url;
  }

  /**
   * @brief Find project parent from object
   *
   * If an object is expected to be a child of a project, this function will traverse its parent
   * tree until it finds it.
   */
  static Project *GetProjectFromObject(const QObject *o);

  static const QString kItemMimeType;

signals:
  void NameChanged();

  void ModifiedChanged(bool e);

  /**
   * @brief Signal emitted when a Node is added to the graph
   */
  void NodeAdded(Node* node);

  /**
   * @brief Signal emitted when a Node is removed from the graph
   */
  void NodeRemoved(Node* node);

  void InputConnected(Node *output, const NodeInput& input);

  void InputDisconnected(Node *output, const NodeInput& input);

  void ValueChanged(const NodeInput& input);

  void InputValueHintChanged(const NodeInput& input);

  void GroupAddedInputPassthrough(NodeGroup *group, const NodeInput &input);

  void GroupRemovedInputPassthrough(NodeGroup *group, const NodeInput &input);

  void GroupChangedOutputPassthrough(NodeGroup *group, Node *output);

protected:
  void AddDefaultNode(Node* n)
  {
    default_nodes_.append(n);
  }

  virtual void childEvent(QChildEvent* event) override;

private:
  QUuid uuid_;

  Folder* root_;

  QString filename_;

  QString saved_url_;

  ColorManager* color_manager_;

  ProjectSettingsNode* settings_;

  bool is_modified_;

  bool autorecovery_saved_;

  QVector<Node*> node_children_;

  QVector<Node*> default_nodes_;

private slots:
  void ColorManagerValueChanged(const NodeInput& input, const TimeRange& range);

};

}

#endif // PROJECT_H
