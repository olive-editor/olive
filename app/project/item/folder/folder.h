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

#ifndef FOLDER_H
#define FOLDER_H

#include "node/node.h"

namespace olive {

/**
 * @brief The Folder class representing a directory in a project structure
 *
 * The Item base class already has support for children, but this functionality is disabled by default
 * (see CanHaveChildren() override). The Folder is a specific type that enables this functionality.
 */
class Folder : public Node
{
  Q_OBJECT
public:
  Folder();

  virtual Node* copy() const override
  {
    return new Folder();
  }

  virtual QString Name() const override
  {
    return tr("Folder");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.folder");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryProject};
  }

  virtual QString Description() const override
  {
    return tr("Organize several items into a single collection.");
  }

  virtual QIcon icon() const override;

  virtual void Retranslate() override;

  bool ChildExistsWithName(const QString& s) const;

  int item_child_count() const
  {
    return item_children_.size();
  }

  Node* item_child(int i) const
  {
    return item_children_.at(i);
  }

  const QVector<Node*>& children() const
  {
    return item_children_;
  }

  virtual bool IsItem() const override
  {
    return true;
  }

  /**
   * @brief Returns a list of nodes that are of a certain type that this node outputs to
   */
  template <typename T>
  QVector<T*> ListOutputsOfType(bool recursive = true) const
  {
    QVector<T *> list;

    ListOutputsOfTypeInternal(this, list, recursive);

    return list;
  }

  int index_of_child(Node* item) const
  {
    return item_children_.indexOf(item);
  }

  int index_of_child_in_array(Node* item) const;

  static const QString kChildInput;

signals:
  void BeginInsertItem(Node* n, int index);

  void EndInsertItem();

  void BeginRemoveItem(Node* n, int index);

  void EndRemoveItem();

protected:
  virtual void InputConnectedEvent(const QString& input, int element, const NodeOutput& output) override;

  virtual void InputDisconnectedEvent(const QString& input, int element, const NodeOutput& output) override;

private:
  template<typename T>
  static void ListOutputsOfTypeInternal(const Folder* n, QVector<T*>& list, bool recursive)
  {
    foreach (const Node::OutputConnection& c, n->output_connections()) {
      Node* connected = c.second.node();

      T* cast_test = dynamic_cast<T*>(connected);

      if (cast_test) {
        // Avoid duplicates
        if (!list.contains(cast_test)) {
          list.append(cast_test);
        }
      }

      if (recursive) {
        Folder* subfolder = dynamic_cast<Folder*>(connected);

        if (subfolder) {
          ListOutputsOfTypeInternal(subfolder, list, recursive);
        }
      }
    }
  }

  QVector<Node*> item_children_;
  QVector<int> item_element_index_;

};

class FolderAddChild : public UndoCommand
{
public:
  FolderAddChild(Folder* folder, Node* child, bool autoposition = true);

  virtual ~FolderAddChild() override;

  virtual Project * GetRelevantProject() const override;

  virtual void redo() override;

  virtual void undo() override;

private:
  Folder* folder_;

  Node* child_;

  bool autoposition_;

  QPointF old_position_;

  NodeSetPositionAsChildCommand* position_command_;

};

}

#endif // FOLDER_H
