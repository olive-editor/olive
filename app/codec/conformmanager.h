#ifndef CONFORMMANAGER_H
#define CONFORMMANAGER_H

#include <QMutex>
#include <QObject>

#include "decoder.h"
#include "task/conform/conform.h"

namespace olive {

class ConformManager : public QObject
{
  Q_OBJECT
public:
  static void CreateInstance()
  {
    if (!instance_) {
      instance_ = new ConformManager();
    }
  }

  static void DestroyInstance()
  {
    delete instance_;
    instance_ = nullptr;
  }

  static ConformManager *instance()
  {
    return instance_;
  }

  enum ConformState {
    kConformExists,
    kConformGenerating
  };

  struct Conform {
    ConformState state;
    QString filename;
    ConformTask *task;
  };

  /**
   * @brief Get conform state, and start conforming if no conform exists
   *
   * Thread-safe.
   */
  Conform GetConformState(const QString &decoder_id, const QString &cache_path, const Decoder::CodecStream &stream, const AudioParams &params, bool wait);

signals:
  void ConformReady();

private:
  ConformManager() = default;

  static ConformManager *instance_;

  QMutex mutex_;

  QWaitCondition conform_done_condition_;

  struct ConformData {
    Decoder::CodecStream stream;
    AudioParams params;
    ConformTask *task;
    QString working_filename;
    QString finished_filename;
  };

  QVector<ConformData> conforming_;

  /**
   * @brief Get the destination filename of an audio stream conformed to a set of parameters
   */
  static QString GetConformedFilename(const QString &cache_path, const Decoder::CodecStream &stream, const AudioParams &params);

private slots:
  void ConformTaskFinished(Task *task, bool succeeded);

};

}

#endif // CONFORMMANAGER_H
