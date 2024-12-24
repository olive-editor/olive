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

#ifndef EXTERNALEDITORPROXY_H
#define EXTERNALEDITORPROXY_H


#include <QProcess>
#include <QFileSystemWatcher>
#include <QTimer>


namespace olive {


/** @brief Interface to edit a shader code in external editor.
 *
 * This class generates a temporary file initialized with the contents
 * passed in constructor. Then an external QProcess is launched and this
 * class listens if generated file is changed externally.
 * The class destructor deletes the temporary file.
 */
class ExternalEditorProxy : public QObject
{
  Q_OBJECT
public:
  explicit ExternalEditorProxy(QObject *parent = nullptr);

  ~ExternalEditorProxy() override;

  // launch an external process, if one is not already running.
  // If a process is running, nothing is done.
  void Launch( const QString & start_text);

  // associate a file path to edit text with an external viewer.
  // Olive will listen for changes in that file.
  void SetFilePath(const QString & path);

  // stop listening to changes of external files.
  // Used when user switches from external to internal editor
  void Detach();

  // to be called when application quits in order to remove
  // temporary files.
  static void CleanGeneratedFiles();

signals:
  // emnitted when temporary file is saved
  void textChanged( const QString & new_text);

private:
  void onFileChanged(const QString& path);
  void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void triggerSynchFromFile();

private slots:
  void synchFromFile();

private:
  // QFileSystemWatcher watcher_;
  QString file_path_;
  QProcess *process_;
  QFileSystemWatcher watcher_;
  QTimer synch_timer_;
};

}  // namespace olive

#endif // EXTERNALEDITORPROXY_H
