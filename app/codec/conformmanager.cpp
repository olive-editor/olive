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
  QVector<QString> filenames = GetConformedFilename(cache_path, stream, params);
  if (AllConformsExist(filenames)) {
    return {kConformExists, filenames, nullptr};
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
    QVector<QString> working_filenames = filenames;
    for (int i=0; i<working_filenames.size(); i++) {
      working_filenames[i].append(QStringLiteral(".working"));
    }

    conforming_task = new ConformTask(decoder_id, stream, params, working_filenames);
    connect(conforming_task, &ConformTask::Finished, this, &ConformManager::ConformTaskFinished);
    conforming_task->moveToThread(TaskManager::instance()->thread());
    QMetaObject::invokeMethod(TaskManager::instance(), "AddTask", Qt::QueuedConnection, Q_ARG(Task *, conforming_task));

    conforming_.append({stream, params, conforming_task, working_filenames, filenames});
  }

  if (wait) {
    do {
      conform_done_condition_.wait(&mutex_);
    } while (!AllConformsExist(filenames));
    return {kConformExists, filenames, nullptr};
  }

  return {kConformGenerating, QVector<QString>(), conforming_task};
}

QVector<QString> ConformManager::GetConformedFilename(const QString &cache_path, const Decoder::CodecStream &stream, const AudioParams &params)
{
  QVector<QString> filenames(params.channel_count());

  for (int i=0; i<filenames.size(); i++) {
    QString index_fn = QStringLiteral("%1-%2.%3.%4.%5.%6.pcm").arg(FileFunctions::GetUniqueFileIdentifier(stream.filename()),
                                                                   QString::number(stream.stream()),
                                                                   QString::number(params.sample_rate()),
                                                                   QString::number(params.format()),
                                                                   QString::number(params.channel_layout()),
                                                                   QString::number(i));

    filenames[i] = QDir(cache_path).filePath(index_fn);
  }

  return filenames;
}

bool ConformManager::AllConformsExist(const QVector<QString> &filenames)
{
  foreach (const QString &fn, filenames) {
    if (!QFileInfo::exists(fn)) {
      return false;
    }
  }

  return true;
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
    for (int i=0; i<data.finished_filename.size(); i++) {
      const QString &finished = data.finished_filename.at(i);
      const QString &working = data.working_filename.at(i);

      QFile::remove(finished);
      QFile::rename(working, finished);
    }

    conform_done_condition_.wakeAll();
    locker.unlock();
    emit ConformReady();
  } else {
    // Failed, just delete the working filename if exists
    for (int i=0; i<data.working_filename.size(); i++) {
      QFile::remove(data.working_filename.at(i));
    }
  }
}

}
