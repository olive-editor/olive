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
#include <QMessageBox>

namespace olive {

ExternalEditorProxy::ExternalEditorProxy(QObject *parent) :
  QObject(parent),
  process_(nullptr)
{

}

ExternalEditorProxy::~ExternalEditorProxy()
{
  // stop process
  if (process_ && process_->isOpen()) {
    process_->kill();
  }

  // delete temporary file
  if (QFileInfo::exists(file_path_)) {
    QFile::remove( file_path_);
  }
}

void ExternalEditorProxy::launch(const QString &start_text)
{
  if (file_path_ == QString())
  {
    // create a file whose name is random but unique for each instance.
    // We can use 'this' pointer as file name.
    // 'QStandardPaths::TempLocation' is guaranteed not to be empty
    file_path_ = QStandardPaths::standardLocations( QStandardPaths::TempLocation).first();
    file_path_ += QString("%1%2.frag").arg(QDir::separator()).arg((long long)this);

    // create file before watching it
    QFile out(file_path_);
    out.open( QIODevice::WriteOnly);
    bool ok = false;

    if (out.isOpen())
    {
      ok = watcher_.addPath( file_path_);
      out.write( start_text.toLatin1());
      out.close();
    }

    if (ok == false)
    {
      QMessageBox::warning( nullptr, "Olive",
                            tr("Can't send data to external editor"));
    }

    connect( & watcher_, & QFileSystemWatcher::fileChanged, this, & ExternalEditorProxy::onFileChanged);
  }

  if (process_ == nullptr)
  {
    process_ = new QProcess(this);
    connect( process_, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(& QProcess::finished),
             this, &ExternalEditorProxy::onProcessFinished);

    QString cmd = Config::Current()["EditorExternalCommand"].toString();
    cmd.replace("%FILE", file_path_);
    cmd.replace("%LINE", "1");  // not yet supported

    QStringList params = cmd.split(' ', Qt::SkipEmptyParts);
    QString opcode = params.takeAt(0);
    process_->start( opcode, params);
  }
}

void ExternalEditorProxy::onFileChanged(const QString &path)
{
  // this should always be true
  Q_ASSERT( path == file_path_);

  QFile f(file_path_);
  f.open( QIODevice::ReadOnly);

  if (f.isOpen())
  {
    QString content = QString::fromLatin1( f.readAll());
    emit textChanged( content);
    f.close();
  }
  else
  {
    QMessageBox::warning( nullptr, "Olive", tr("Can't update data from external editor"));
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


}  // namespace olive
