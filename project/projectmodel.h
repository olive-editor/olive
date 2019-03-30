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

#ifndef PROJECTMODEL_H
#define PROJECTMODEL_H

#include <QAbstractItemModel>

#include "project/media.h"
#include "undo/comboaction.h"

class ProjectModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  ProjectModel(QObject* parent = nullptr);
  ~ProjectModel() override;

  /**
   * @brief Makes preparations for saving the project file.
   *
   * Some items require IDs to link between them (e.g. the Footage or Nested Sequences used by Clips, linked clips,
   * etc). This function sets up those IDs before saving.
   *
   * NOTE: This function should **always** be called before Save().
   *
   * @return
   *
   * A count of the elements in the project to save into the project file. The loading system can later use this value
   * to determine the load progress.
   */
  int PrepareToSave();

  /**
   * @brief Initiate a save of all the project data
   *
   * Recursively goes through the entire project tree saving everything to a specified QXmlStreamWriter object.
   *
   * It's not recommended to use this function directly as it expects an existing QXmlStreamWriter and doesn't write a
   * header and footer for the resulting XML document. Instead use olive::Save().
   *
   * NOTE: **Always** call PrepareToSave() just before calling this function to set up valid IDs for saving.
   *
   * @param stream
   *
   * A QXmlStreamWriter object.
   *
   * @param root
   *
   * Used for recursion, set to any child and called again whenever a child is found with children. If this is nullptr,
   * this function will loop over the root item.
   */
  void Save(QXmlStreamWriter& stream, Media *root = nullptr);

  void make_root();
  void destroy_root();
  void clear();
  Media* get_root() const;
  QVariant data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex create_index(int arow, int acolumn, void *adata);
  QModelIndex parent(const QModelIndex &index) const override;
  bool canFetchMore(const QModelIndex &parent) const override;
  bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  Media* getItem(const QModelIndex &index) const;

  void appendChild(Media *parent, MediaPtr child);
  void moveChild(MediaPtr child, Media* to);
  void removeChild(Media* parent, Media* m);
  Media* child(int i, Media* parent = nullptr);
  int childCount(Media* parent = nullptr);
  void set_icon(Media* m, const QIcon &ico);

  void process_file_list(QStringList& files, bool recursive = false, MediaPtr replace = nullptr, Media *parent = nullptr);

  /**
   * @brief Get a list of the last imported media
   *
   * @return
   *
   * Returns a list of all the Media processed by the last call to process_file_list().
   */
  QVector<Media*> GetLastImportedMedia();

  QVector<Media*> GetAllSequences();
  QVector<Media*> GetAllFootage();
  QVector<Media*> GetAllFolders();

  QString GetNextSequenceName(QString prepend = QString());

  MediaPtr CreateSequence(ComboAction *ca, SequencePtr s, bool open, Media* parent);

private:
  MediaPtr root_item_;

  QVector<Media*> last_imported_media;

  QVector<Media*> GetAllMediaOfType(int search_type);
  void RecurseTree(Media* parent, QVector<Media *> &list, int search_type);

  void PrepareToSaveInternal(int& element_count, Media* root);
};

namespace olive {
extern ProjectModel project_model;
}

#endif // PROJECTMODEL_H
