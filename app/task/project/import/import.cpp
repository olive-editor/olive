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

#include "import.h"

#include <QDir>
#include <QFileInfo>

#include "config/config.h"
#include "core.h"
#include "node/project/footage/footage.h"
#include "widget/nodeview/nodeviewundo.h"

namespace olive {

ProjectImportTask::ProjectImportTask(ProjectViewModel *model, Folder *folder, const QStringList &filenames) :
  command_(nullptr),
  model_(model),
  folder_(folder)
{
  foreach (const QString& f, filenames) {
    filenames_.append(f);
  }

  file_count_ = Core::CountFilesInFileList(filenames_);

  SetTitle(tr("Importing %n file(s)", nullptr, file_count_));
}

const int &ProjectImportTask::GetFileCount() const
{
  return file_count_;
}

bool ProjectImportTask::Run()
{
  command_ = new MultiUndoCommand();

  int imported = 0;

  Import(folder_, filenames_, imported, command_);

  if (IsCancelled()) {
    delete command_;
    command_ = nullptr;
    return false;
  } else {
    return true;
  }
}

void ProjectImportTask::Import(Folder *folder, QFileInfoList import, int &counter, MultiUndoCommand* parent_command)
{
  for (int i=0; i<import.size(); i++) {
    if (IsCancelled()) {
      break;
    }

    const QFileInfo& file_info = import.at(i);

    // Check if this file is a directory
    if (file_info.isDir()) {

      // QDir::entryList only returns filenames, we can use entryInfoList() to get full paths
      QFileInfoList entry_list = QDir(file_info.absoluteFilePath()).entryInfoList();

      // Strip out "." and ".." (for some reason QDir::NoDotAndDotDot	doesn't work with entryInfoList, so we have to
      // check manually)
      for (int j=0;j<entry_list.size();j++) {
        if (entry_list.at(j).fileName() == QStringLiteral(".")
            || entry_list.at(j).fileName() == QStringLiteral("..")) {
          entry_list.removeAt(j);
          j--;
        }
      }

      // Only proceed if the empty actually has files in it
      if (!entry_list.isEmpty()) {
        // Create a folder corresponding to the directory
        Folder* f = new Folder();

        f->SetLabel(file_info.fileName());

        // Create undoable command that adds the items to the model
        AddItemToFolder(folder, f, parent_command);

        // Recursively follow this path
        Import(f, entry_list, counter, parent_command);
      }

    } else {

      Footage* footage = new Footage(file_info.absoluteFilePath());

      footage->SetLabel(file_info.fileName());

      if (footage->IsValid()) {
        // See if this footage is an image sequence
        ValidateImageSequence(footage, import, i);

        // Create undoable command that adds the items to the model
        AddItemToFolder(folder, footage, parent_command);
      } else {
        // Add to list so we can tell the user about it later
        invalid_files_.append(file_info.absoluteFilePath());

        delete footage;
      }

      counter++;

      emit ProgressChanged(static_cast<double>(counter) / static_cast<double>(file_count_));

    }
  }
}

void ProjectImportTask::ValidateImageSequence(Footage *footage, QFileInfoList& info_list, int index)
{
  // Heuristically determine whether this file is part of an image sequence or not
  //
  // By this point we've established that video contains a single still image stream. Now we'll
  // see if it ends with numbers.
  if (Decoder::GetImageSequenceDigitCount(footage->filename()) > 0
      && !image_sequence_ignore_files_.contains(footage->filename())
      && footage->InputArraySize(Footage::kVideoParamsInput)) {
    VideoParams video_stream = footage->GetVideoParams(0);
    QSize dim(video_stream.width(), video_stream.height());

    int64_t ind = Decoder::GetImageSequenceIndex(footage->filename());

    // Check if files around exist around it with that follow a sequence
    QString previous_img_fn = Decoder::TransformImageSequenceFileName(footage->filename(), ind - 1);
    QString next_img_fn = Decoder::TransformImageSequenceFileName(footage->filename(), ind + 1);

    // See if the same decoder can retrieve surrounding files
    DecoderPtr decoder = Decoder::CreateFromID(footage->decoder());

    Footage* previous_file = new Footage(previous_img_fn);
    Footage* next_file = new Footage(next_img_fn);

    // Finally see if these files have the same dimensions
    if ((previous_file->IsValid() && CompareStillImageSize(previous_file, dim))
        || (next_file->IsValid() && CompareStillImageSize(next_file, dim))) {
      // By this point, we've established this file is a still image with a number at the end of
      // the filename surrounded by adjacent numbers. It could be a still image! But let's ask the
      // user just in case...
      bool is_sequence;

      QMetaObject::invokeMethod(Core::instance(),
                                "ConfirmImageSequence",
                                Qt::BlockingQueuedConnection,
                                Q_RETURN_ARG(bool, is_sequence),
                                Q_ARG(QString, footage->filename()));

      int64_t seq_index = Decoder::GetImageSequenceIndex(footage->filename());

      // Heuristic to find the first and last images (users can always override this later in
      // FootagePropertiesDialog)
      int64_t start_index = GetImageSequenceLimit(footage->filename(), seq_index, false);
      int64_t end_index = GetImageSequenceLimit(footage->filename(), seq_index, true);

      // Depending on the user's choice, either remove them from the list or don't ask for the
      // remainders
      for (int64_t j=start_index; j<=end_index; j++) {
        QString entry_fn = Decoder::TransformImageSequenceFileName(footage->filename(), j);

        if (is_sequence) {
          // If this is part of the sequence we're importing here, remove it
          for (int i=index+1; i<info_list.size(); i++) {
            if (info_list.at(i).absoluteFilePath() == entry_fn) {
              if (is_sequence) {
                info_list.removeAt(i);
              }
              break;
            }
          }
        } else {
          image_sequence_ignore_files_.append(entry_fn);
        }
      }

      if (is_sequence) {
        // User has confirmed it is a still image, let's set it accordingly.
        video_stream.set_video_type(VideoParams::kVideoTypeImageSequence);

        rational default_timebase = Config::Current()[QStringLiteral("DefaultSequenceFrameRate")].value<rational>();
        video_stream.set_time_base(default_timebase);
        video_stream.set_frame_rate(default_timebase.flipped());

        video_stream.set_start_time(start_index);
        video_stream.set_duration(end_index - start_index + 1);

        footage->SetVideoParams(video_stream, 0);
      }
    }

    delete previous_file;
    delete next_file;
  }
}

void ProjectImportTask::AddItemToFolder(Folder *folder, Node *item, MultiUndoCommand *command)
{
  // Create undoable command that adds the items to the model
  Project* project = model_->project();

  NodeAddCommand* nac = new NodeAddCommand(project, item);
  nac->PushToThread(project->thread());
  command->add_child(nac);

  command->add_child(new FolderAddChild(folder, item));
}

bool ProjectImportTask::ItemIsStillImageFootageOnly(Footage* footage)
{
  if (footage->GetTotalStreamCount() != 1) {
    // Footage with more than one stream (usually video+audio) most likely isn't an image sequence
    return false;
  }

  VideoParams vp = footage->GetVideoParams(0);

  // Footage must be valid and video stream must be a still image to be an image sequence
  return vp.is_valid() && vp.video_type() == VideoParams::kVideoTypeStill;
}

bool ProjectImportTask::CompareStillImageSize(Footage* footage, const QSize &sz)
{
  if (!ItemIsStillImageFootageOnly(footage)) {
    return false;
  }

  VideoParams stream = footage->GetVideoParams(0);

  return stream.width() == sz.width() && stream.height() == sz.height();
}

int64_t ProjectImportTask::GetImageSequenceLimit(const QString& start_fn, int64_t start, bool up)
{
  QString test_filename;
  int test_index;

  forever {
    if (up) {
      test_index = start + 1;
    } else {
      test_index = start - 1;
    }

    test_filename = Decoder::TransformImageSequenceFileName(start_fn, test_index);

    if (!QFileInfo::exists(test_filename)) {
      // Reached end of index
      break;
    }

    start = test_index;
  }

  return start;
}

}
