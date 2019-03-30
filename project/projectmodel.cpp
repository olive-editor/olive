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

#include "projectmodel.h"

#include "panels/panels.h"
#include "panels/viewer.h"
#include "ui/viewerwidget.h"
#include "project/media.h"
#include "global/debug.h"

ProjectModel olive::project_model;

ProjectModel::ProjectModel(QObject *parent) : QAbstractItemModel(parent), root_item_(nullptr) {
  make_root();
}

ProjectModel::~ProjectModel() {
  destroy_root();
}

int ProjectModel::PrepareToSave()
{
  int element_count = 0;

  PrepareToSaveInternal(element_count, get_root());

  return element_count;
}

void ProjectModel::PrepareToSaveInternal(int& element_count, Media *root)
{
  for (int i=0;i<root->childCount();i++) {
    Media* child = root->child(i);

    switch (child->get_type()) {
    case MEDIA_TYPE_FOLDER:
      child->temp_id = element_count;
      break;
    case MEDIA_TYPE_FOOTAGE:
      child->to_footage()->save_id = element_count;
      break;
    case MEDIA_TYPE_SEQUENCE:
      child->to_sequence()->save_id = element_count;
      break;
    }

    element_count++;

    if (child->childCount() > 0) {
      PrepareToSaveInternal(element_count, child);
    }
  }
}

void ProjectModel::Save(QXmlStreamWriter &stream, Media* root)
{
  if (root == nullptr) {
    root = get_root();
  }

  for (int i=0;i<root->childCount();i++) {
    Media* child = root->child(i);

    child->Save(stream);

    if (child->childCount() > 0) {
      Save(stream, child);
    }
  }
}

void ProjectModel::make_root() {
  root_item_ = std::make_shared<Media>();
  root_item_->temp_id = 0;
  root_item_->root = true;
}

void ProjectModel::destroy_root() {
  if (panel_sequence_viewer != nullptr) {
    panel_sequence_viewer->set_media(nullptr);
  }
  if (panel_footage_viewer != nullptr) {
    panel_footage_viewer->set_media(nullptr);
  }

  root_item_ = nullptr;
}

void ProjectModel::clear() {
  beginResetModel();
  destroy_root();
  make_root();
  endResetModel();
}

Media *ProjectModel::get_root() const {
  return root_item_.get();
}

QVariant ProjectModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  Media* media = static_cast<Media*>(index.internalPointer());

  return media->data(index.column(), role);
}

Qt::ItemFlags ProjectModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  return QAbstractItemModel::flags(index) | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEditable;
}

QVariant ProjectModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return root_item_->data(section, role);

  return QVariant();
}

QModelIndex ProjectModel::index(int row, int column, const QModelIndex &parent) const {
  if (!hasIndex(row, column, parent))
    return QModelIndex();

  Media *parentItem;

  if (!parent.isValid()) {
    parentItem = get_root();
  } else {
    parentItem = static_cast<Media*>(parent.internalPointer());
  }

  Media *childItem = parentItem->child(row);
  if (childItem) {
    return createIndex(row, column, childItem);
  } else {
    return QModelIndex();
  }
}

QModelIndex ProjectModel::create_index(int arow, int acolumn, void* adata) {
    return createIndex(arow, acolumn, adata);
}

QModelIndex ProjectModel::parent(const QModelIndex &index) const {
  if (!index.isValid())
    return QModelIndex();

  Media *childItem = static_cast<Media*>(index.internalPointer());
  Media *parentItem = childItem->parentItem();

  if (parentItem == get_root())
    return QModelIndex();

  return createIndex(parentItem->row(), 0, parentItem);
}

bool ProjectModel::canFetchMore(const QModelIndex &parent) const
{

  // Mostly implementing this because QSortFilterProxyModel will actually *ignore* a "true" result from hasChildren()
  // and then query this value for a "true" result. So we need to override both this and hasChildren() to show a
  // persistent childIndicator for folder items.

  if (parent.isValid()
      && static_cast<Media*>(parent.internalPointer())->get_type() == MEDIA_TYPE_FOLDER) {
    return true;
  }
  return QAbstractItemModel::canFetchMore(parent);
}

bool ProjectModel::hasChildren(const QModelIndex &parent) const
{
  if (parent.isValid()
      && static_cast<Media*>(parent.internalPointer())->get_type() == MEDIA_TYPE_FOLDER) {
    return true;
  }
  return QAbstractItemModel::hasChildren(parent);
}

bool ProjectModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (role != Qt::EditRole)
    return false;

  Media *item = static_cast<Media*>(index.internalPointer());
  bool result = item->setData(index.column(), value);

  if (result)
    emit dataChanged(index, index);

  return result;
}

int ProjectModel::rowCount(const QModelIndex &parent) const {
  Media *parentItem;
  if (parent.column() > 0)
    return 0;

  if (!parent.isValid()) {
    parentItem = get_root();
  } else {
    parentItem = static_cast<Media*>(parent.internalPointer());
  }

  return parentItem->childCount();
}

int ProjectModel::columnCount(const QModelIndex &parent) const {
  if (parent.isValid())
    return static_cast<Media*>(parent.internalPointer())->columnCount();
  else
    return root_item_->columnCount();
}

Media *ProjectModel::getItem(const QModelIndex &index) const {
  if (index.isValid()) {
    Media *item = static_cast<Media*>(index.internalPointer());
    if (item)
      return item;
  }
  return get_root();
}

void ProjectModel::set_icon(Media* m, const QIcon &ico) {
  QModelIndex index = createIndex(m->row(), 0, m);
  m->set_icon(ico);
  emit dataChanged(index, index);
}

QVector<Media *> ProjectModel::GetAllSequences()
{
  return GetAllMediaOfType(MEDIA_TYPE_SEQUENCE);
}

QVector<Media *> ProjectModel::GetAllFootage()
{
  return GetAllMediaOfType(MEDIA_TYPE_FOOTAGE);
}

QVector<Media *> ProjectModel::GetAllFolders()
{
  return GetAllMediaOfType(MEDIA_TYPE_FOLDER);
}

QString ProjectModel::GetNextSequenceName(QString prepend)
{
  if (prepend.isEmpty()) {
    prepend = tr("Sequence %1");
  }

  int sequence_number = 1;
  QString test;
  QVector<Media*> all_sequences = GetAllSequences();
  bool found;

  do {
    found = false;
    test = prepend.arg(sequence_number);
    for (int i=0;i<all_sequences.size();i++) {
      if (all_sequences.at(i)->get_name() == test) {
        found = true;
        sequence_number++;
        break;
      }
    }
  } while (found);

  return test;
}

MediaPtr ProjectModel::CreateSequence(ComboAction *ca, SequencePtr s, bool open, Media *parent)
{
  MediaPtr item = std::make_shared<Media>();
  item->set_sequence(s);

  if (ca != nullptr) {

    ca->append(new AddMediaCommand(item, parent));

    if (open) {
      ca->append(new ChangeSequenceAction(s));
    }

  } else {

    appendChild(parent, item);

    if (open) {
      olive::Global->set_sequence(s);
    }

  }

  return item;
}

QVector<Media *> ProjectModel::GetAllMediaOfType(int search_type)
{
  QVector<Media*> media_list;
  RecurseTree(root_item_.get(), media_list, search_type);
  return media_list;
}

void ProjectModel::RecurseTree(Media* parent, QVector<Media*>& list, int search_type)
{
  for (int i=0;i<parent->childCount();i++) {

    Media* child = parent->child(i);

    if (child->get_type() == search_type) {
      list.append(child);
    }

    if (child->childCount() > 0) {
      RecurseTree(child, list, search_type);
    }
  }
}

void ProjectModel::appendChild(Media* parent, MediaPtr child) {
  QModelIndex row_start;

  if (parent == nullptr) {

    parent = get_root();
    row_start = QModelIndex();

  } else {

    row_start = createIndex(parent->row(), 0, parent);

  }

  beginInsertRows(row_start, parent->childCount(), parent->childCount());
  parent->appendChild(child);
  endInsertRows();
}

void ProjectModel::moveChild(MediaPtr child, Media *to) {
  if (to == nullptr) {
    to = get_root();
  }

  Media* from = child->parentItem();

  beginMoveRows(
        from == get_root() ? QModelIndex() : createIndex(from->row(), 0, from),
        child->row(),
        child->row(),
        to == get_root() ? QModelIndex() : createIndex(to->row(), 0, to),
        to->childCount()
      );

  from->removeChild(child->row());
  to->appendChild(child);
  endMoveRows();
}

void ProjectModel::removeChild(Media* parent, Media* m) {
  if (parent == nullptr) {
    parent = get_root();
  }
  beginRemoveRows(parent == get_root() ?
                    QModelIndex() : createIndex(parent->row(), 0, parent), m->row(), m->row());
  parent->removeChild(m->row());
  endRemoveRows();
}

Media* ProjectModel::child(int i, Media* parent) {
  if (parent == nullptr) {
    parent = get_root();
  }
  return parent->child(i);
}

int ProjectModel::childCount(Media *parent) {
  if (parent == nullptr) {
    parent = get_root();
  }
  return parent->childCount();
}

void ProjectModel::process_file_list(QStringList& files, bool recursive, MediaPtr replace, Media* parent) {
  bool imported = false;

  // retrieve the array of image formats from the user's configuration
  QStringList image_sequence_formats = olive::CurrentConfig.img_seq_formats.split("|");

  // a cache of image sequence formatted URLS to assist the user in importing image sequences
  QVector<QString> image_sequence_urls;
  QVector<bool> image_sequence_importassequence;

  if (!recursive) last_imported_media.clear();

  bool create_undo_action = (!recursive && replace == nullptr);
  ComboAction* ca = nullptr;
  if (create_undo_action) ca = new ComboAction();

  // Loop through received files
  for (int i=0;i<files.size();i++) {

    // If this file is a directory, we'll recursively call this function again to process the directory's contents
    if (QFileInfo(files.at(i)).isDir()) {

      QString folder_name = get_file_name_from_path(files.at(i));
      MediaPtr folder = CreateFolder(folder_name);

      QDir directory(files.at(i));
      directory.setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);

      QFileInfoList subdir_files = directory.entryInfoList();
      QStringList subdir_filenames;

      for (int j=0;j<subdir_files.size();j++) {
        subdir_filenames.append(subdir_files.at(j).filePath());
      }

      if (create_undo_action) {
        ca->append(new AddMediaCommand(folder, parent));
      } else {
        appendChild(parent, folder);
      }

      process_file_list(subdir_filenames, true, nullptr, folder.get());

      imported = true;

    } else if (!files.at(i).isEmpty()) {
      QString file = files.at(i);

      // Check if the user is importing an Olive project file
      if (file.endsWith(".ove", Qt::CaseInsensitive)) {

        // This file is an Olive project file. Ask the user if they really want to import it.
        if (QMessageBox::question(this,
                                  tr("Import a Project"),
                                  tr("\"%1\" is an Olive project file. It will merge with this project. "
                                     "Do you wish to continue?").arg(file),
                                  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

          // load the project without clearing the current one
          olive::Global->ImportProject(file);

        }

      } else {

        // This file is NOT an Olive project file

        // Used later if this file is part of an already processed image sequence
        bool skip = false;

        /* Heuristic to determine whether file is part of an image sequence */

        // Firstly, we run a heuristic on whether this file is an image by checking its file extension

        bool file_is_an_image = false;

        // Get the string position of the extension in the filename
        int lastcharindex = file.lastIndexOf(".");

        if (lastcharindex != -1 && lastcharindex > file.lastIndexOf('/')) {

          QString ext = file.mid(lastcharindex+1);

          // If the file extension is part of a predetermined list (from Config::img_seq_formats), we'll treat it
          // as an image
          if (image_sequence_formats.contains(ext, Qt::CaseInsensitive)) {
            file_is_an_image = true;
          }

        } else {

          // If we're here, the file has no extension, but we'll still check if its an image sequence just in case
          lastcharindex = file.length();
          file_is_an_image = true;

        }

        // Some image sequence's don't start at "0", if it is indeed an image sequence, we'll use this variable
        // later to determine where it does start
        int start_number = 0;

        // Check if we passed the earlier heuristic to check whether this is a file, and whether the last number in
        // the filename (before the extension) is a number
        if (file_is_an_image && file[lastcharindex-1].isDigit()) {

          // Check how many digits are at the end of this filename
          int digit_count = 0;
          int digit_test = lastcharindex-1;
          while (file[digit_test].isDigit()) {
            digit_count++;
            digit_test--;
          }

          // Retrieve the integer represented at the end of this filename
          digit_test++;
          int file_number = file.mid(digit_test, digit_count).toInt();

          // Check whether a file exists with the same format but one number higher or one number lower
          if (QFileInfo::exists(QString(file.left(digit_test) + QString("%1").arg(file_number-1, digit_count, 10, QChar('0')) + file.mid(lastcharindex)))
              || QFileInfo::exists(QString(file.left(digit_test) + QString("%1").arg(file_number+1, digit_count, 10, QChar('0')) + file.mid(lastcharindex)))) {

            //
            // If so, it certainly looks like it *could* be an image sequence, but we'll ask the user just in case
            //

            // Firstly we should check if this file is part of a sequence the user has already confirmed as either a
            // sequence or not a sequence (e.g. if the user happened to select a bunch of images that happen to increase
            // consecutively). We format the filename with FFmpeg's '%Nd' (N = digits) formatting for reading image
            // sequences
            QString new_filename = file.left(digit_test) + "%" + QString::number(digit_count) + "d" + file.mid(lastcharindex);

            int does_url_cache_already_contain_this = image_sequence_urls.indexOf(new_filename);

            if (does_url_cache_already_contain_this > -1) {

              // We've already processed an image with the same formatting

              // Check if the last time we saw this formatting, the user chose to import as a sequence
              if (image_sequence_importassequence.at(does_url_cache_already_contain_this)) {

                // If so, no need to import this file too, so we signal to the rest of the function to skip this file
                skip = true;

              }

              // If not, we can fall-through to the next step which is importing normally

            } else {

              // If we're here, we've never seen a file with this formatting before, so we'll ask whether to import
              // as a sequence or not

              // Add this file formatting file to the URL cache
              image_sequence_urls.append(new_filename);

              // This does look like an image sequence, let's ask the user if it'll indeed be an image sequence
              if (QMessageBox::question(this,
                                        tr("Image sequence detected"),
                                        tr("The file '%1' appears to be part of an image sequence. "
                                           "Would you like to import it as such?").arg(file),
                                        QMessageBox::Yes | QMessageBox::No,
                                        QMessageBox::Yes) == QMessageBox::Yes) {

                // Proceed to the next step of this with the formatted filename
                file = new_filename;

                // Cache the user's answer alongside the image_sequence_urls value - in this case, YES, this will be an
                // image sequence
                image_sequence_importassequence.append(true);


                // FFmpeg needs to know what file number to start at in the sequence. In case the image sequence doesn't
                // start at a zero, we'll loop decreasing the number until it doesn't exist anymore
                QString test_filename_format = QString("%1%2%3").arg(file.left(digit_test), "%1", file.mid(lastcharindex));
                int test_file_number = file_number;
                do {
                  test_file_number--;
                } while (QFileInfo::exists(test_filename_format.arg(QString("%1").arg(test_file_number, digit_count, 10, QChar('0')))));

                // set the image sequence's start number to the last that existed
                start_number = test_file_number + 1;

              } else {

                // Cache the user's response to the image sequence question - i.e. none of the files imported with this
                // formatting should be imported as an image sequence
                image_sequence_importassequence.append(false);

              }
            }

          }

        }

        // If we're not skipping this file, let's import it
        if (!skip) {
          MediaPtr item;
          FootagePtr m;

          if (replace != nullptr) {
            item = replace;
          } else {
            item = std::make_shared<Media>();
          }

          m = std::make_shared<Footage>();

          // Edge case for PNGs that standardized unassociated alpha
          if (file.endsWith("png", Qt::CaseInsensitive)) {
            m->alpha_is_associated = false;
          }

          m->using_inout = false;
          m->url = file;
          m->name = QFileInfo(files.at(i)).fileName();
          m->start_number = start_number;

          item->set_footage(m);

          last_imported_media.append(item.get());

          if (replace == nullptr) {
            if (create_undo_action) {
              ca->append(new AddMediaCommand(item, parent));
            } else {
              appendChild(parent, item);
            }
          }

          imported = true;
        }

      }


    }
  }
  if (create_undo_action) {
    if (imported) {
      olive::UndoStack.push(ca);

      for (int i=0;i<last_imported_media.size();i++) {
        // generate waveform/thumbnail in another thread
        PreviewGenerator::AnalyzeMedia(last_imported_media.at(i));
      }
    } else {
      delete ca;
    }
  }
}
