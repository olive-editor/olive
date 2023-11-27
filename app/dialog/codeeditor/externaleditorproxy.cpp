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

#include "externaleditorproxy.h"
#include "config/config.h"

#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QThread>
#include <QMessageBox>
#include <QSet>

namespace {
// files created in this session.
// They can be deleted when application quits.
QSet<QString> CreatedFiles;
}

namespace olive {

ExternalEditorProxy::ExternalEditorProxy(QObject *parent) :
  QObject(parent),
  process_(nullptr),
  watcher_( this)
{
  connect( & watcher_, & QFileSystemWatcher::fileChanged, this, & ExternalEditorProxy::onFileChanged);
}

ExternalEditorProxy::~ExternalEditorProxy()
{
  // Do not delete "file_path_" now, bacause it might be read from
  // another instance of this class. This happens when the clip associated with
  // this instance loses focus: when it gets focus back, the file file name is used.
  //
  // All files will be deleted when application quits.
}

void ExternalEditorProxy::Launch(const QString &start_text)
{

  // create file before watching it
  QFile out(file_path_);
  out.open( QIODevice::WriteOnly);

  if (out.isOpen()) {
    out.write( start_text.toLatin1());
    out.close();

    CreatedFiles.insert( file_path_);

    // now the file exists. We can watch for changes
    watcher_.addPath( file_path_);

  } else {
    QMessageBox::warning( nullptr, "Olive",
                          tr("Can't send data to external editor"));
  }

  if (process_ == nullptr) {
    process_ = new QProcess(this);
    connect( process_, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(& QProcess::finished),
             this, &ExternalEditorProxy::onProcessFinished);

    QString cmd = Config::Current()["EditorExternalCommand"].toString();
    QString params = Config::Current()["EditorExternalParams"].toString();
    params.replace("%FILE", file_path_);
    params.replace("%LINE", "1");  // not yet supported

    process_->start( cmd, params.split(' ', Qt::SkipEmptyParts));
  }
}

void ExternalEditorProxy::SetFilePath(const QString & path)
{
  file_path_ = path;

  // at this point, "file_path_" may or may not exist.

  // If it exists, align the shader to the content of the file
  // as it is possible that user modified the file while the clip was
  // not selected and so the shader did not update with file.
  if (QFileInfo(file_path_).exists()) {
    triggerSynchFromFile();
  }

  // The following instruction is effective when the file exists.
  watcher_.addPath( file_path_);
}

void ExternalEditorProxy::Detach()
{
  watcher_.removePath( file_path_);
}

void ExternalEditorProxy::CleanGeneratedFiles()
{
  foreach (const QString & file, CreatedFiles) {
    QFile::remove( file);
  }
}

void ExternalEditorProxy::onFileChanged(const QString &path)
{
  // this should always be true
  Q_ASSERT( path == file_path_);

  QFile f(file_path_);
  f.open( QIODevice::ReadOnly);

  if (f.size() > 0)
  {
    if (f.isOpen()) {
      QString content = QString::fromLatin1( f.readAll());
      emit textChanged( content);
      f.close();
    } else {
      QMessageBox::warning( nullptr, "Olive", tr("Can't update data from external editor"));
    }
  } else {
    // on some platforms, when the file is saved, it is deleted before
    // being re-written. In this case, there is a signal of file changed
    // but it's size is zero. This must be discarded; a new message will arrive later.
    f.close();
  }
}

void ExternalEditorProxy::onProcessFinished(int /*exitCode*/, QProcess::ExitStatus /*exitStatus*/)
{
  // Process has saved file before exit or has unexpectedly finished.
  // Do not emit the text changed signal

  // Also, some editors fork their process and this slot is called when the editor is
  // still running. For this reason, do not stop listening for modified file

  process_->deleteLater();
  process_ = nullptr;
}

void ExternalEditorProxy::triggerSynchFromFile()
{
  // run a 0 ms timer so that "synch" operation can be done
  // immediately but after setting up param-view panel
  synch_timer_.singleShot(0, this, & ExternalEditorProxy::synchFromFile);
}

void ExternalEditorProxy::synchFromFile()
{
  onFileChanged( file_path_);
}


}  // namespace olive
