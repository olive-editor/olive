#include "conformmanager.h"

#include <QDir>

#include "task/taskmanager.h"

namespace olive {

ConformManager *ConformManager::instance_ = nullptr;

ConformManager::Conform ConformManager::GetConformState(const QString &decoder_id, const QString &cache_path, const Decoder::CodecStream &stream, const AudioParams &params, bool wait)
{
  // Mutex because we'll need to check the status of a conform task
  QMutexLocker locker(&mutex_);

  // Return existing conform if exists
  QString filename = GetConformedFilename(cache_path, stream, params);
  if (QFileInfo::exists(filename)) {
    return {kConformExists, filename, nullptr};
  }

  ConformTask *conforming_task = nullptr;

  foreach (const ConformData &data, conforming_) {
    if (data.stream == stream && data.params == params) {
      // Already creating conform in a task
      conforming_task = data.task;
      break;
    }
  }

  if (!conforming_task) {
    // Not conforming yet, create a task to do so

    // We conform to a different filename until it's done to make it clear even across sessions
    // whether this conform is ready or not
    QString working_fn = filename;
    working_fn.append(QStringLiteral(".working"));

    conforming_task = new ConformTask(decoder_id, stream, params, working_fn);
    connect(conforming_task, &ConformTask::Finished, this, &ConformManager::ConformTaskFinished);
    conforming_task->moveToThread(TaskManager::instance()->thread());
    QMetaObject::invokeMethod(TaskManager::instance(), "AddTask", Qt::QueuedConnection, Q_ARG(Task *, conforming_task));

    conforming_.append({stream, params, conforming_task, working_fn, filename});
  }

  if (wait) {
    do {
      conform_done_condition_.wait(&mutex_);
    } while (!QFileInfo::exists(filename));
    return {kConformExists, filename, nullptr};
  }

  return {kConformGenerating, QString(), conforming_task};
}

QString ConformManager::GetConformedFilename(const QString &cache_path, const Decoder::CodecStream &stream, const AudioParams &params)
{
  QString index_fn = QStringLiteral("%1.%2:%3").arg(FileFunctions::GetUniqueFileIdentifier(stream.filename()),
                                                    QString::number(stream.stream()));

  index_fn = QDir(cache_path).filePath(index_fn);

  index_fn.append('.');
  index_fn.append(QString::number(params.sample_rate()));
  index_fn.append('.');
  index_fn.append(QString::number(params.format()));
  index_fn.append('.');
  index_fn.append(QString::number(params.channel_layout()));

  return index_fn;
}

void ConformManager::ConformTaskFinished(Task *task, bool succeeded)
{
  QMutexLocker locker(&mutex_);

  ConformData data;

  // Remove conform data from list
  for (int i=0; i<conforming_.size(); i++) {
    const ConformData &c = conforming_.at(i);
    if (c.task == task) {
      data = c;
      conforming_.removeAt(i);
      break;
    }
  }

  if (succeeded) {
    // Move file to standard conform name, making it clear this conform is ready for use
    QFile::remove(data.finished_filename);
    QFile::rename(data.working_filename, data.finished_filename);

    conform_done_condition_.wakeAll();
    locker.unlock();
    emit ConformReady();
  } else {
    // Failed, just delete the working filename if exists
    QFile::remove(data.working_filename);
  }
}

}
