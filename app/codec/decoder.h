/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

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

#ifndef DECODER_H
#define DECODER_H

extern "C" {
#include <libswresample/swresample.h>
}

#include <QFileInfo>
#include <QMutex>
#include <QObject>
#include <QWaitCondition>
#include <stdint.h>

#include "node/block/block.h"
#include "node/project/footage/footagedescription.h"
#include "render/cancelatom.h"
#include "render/rendermodes.h"

namespace olive {

class Decoder;
using DecoderPtr = std::shared_ptr<Decoder>;

#define DECODER_DEFAULT_DESTRUCTOR(x) virtual ~x() override {CloseInternal();}

/**
 * @brief A decoder's is the main class for bringing external media into Olive
 *
 * Its responsibilities are to serve as
 * abstraction from codecs/decoders and provide complete frames. These frames can be video or audio data and are
 * provided as Frame objects in shared pointers to alleviate the responsibility of memory handling.
 *
 * The main function in a decoder is Retrieve() which should return complete image/audio data. A decoder should
 * alleviate all the complexities of codec compression from the rest of the application (i.e. a decoder should never
 * return a partial frame or require other parts of the system to interface directly with the codec). Often this will
 * necessitate pre-emptively caching, indexing, or even fully transcoding media before using it which can be implemented
 * through the Analyze() function.
 *
 * A decoder does NOT perform any pixel/sample format conversion. Frames should pass through the PixelService
 * to be utilized in the rest of the rendering pipeline.
 */
class Decoder : public QObject
{
  Q_OBJECT
public:
  enum RetrieveState {
    kReady,
    kFailedToOpen,
    kIndexUnavailable
  };

  Decoder();

  /**
   * @brief Unique decoder ID
   */
  virtual QString id() const = 0;

  virtual bool SupportsVideo(){return false;}
  virtual bool SupportsAudio(){return false;}

  void IncrementAccessTime(qint64 t);

  class CodecStream
  {
  public:
    CodecStream() :
      stream_(-1),
      block_(nullptr)
    {
    }

    CodecStream(const QString& filename, int stream, Block *block) :
      filename_(filename),
      stream_(stream),
      block_(block)
    {
    }

    bool IsValid() const
    {
      return !filename_.isEmpty() && stream_ >= 0;
    }

    bool Exists() const
    {
      return QFileInfo::exists(filename_);
    }

    void Reset()
    {
      *this = CodecStream();
    }

    bool operator==(const CodecStream& rhs) const
    {
      return filename_ == rhs.filename_ && stream_ == rhs.stream_;
    }

    const QString& filename() const
    {
      return filename_;
    }

    int stream() const
    {
      return stream_;
    }

    Block *block() const
    {
      return block_;
    }

  private:
    QString filename_;

    int stream_;

    Block *block_;

  };

  /**
   * @brief Open stream for decoding
   *
   * This function is thread safe.
   *
   * Returns TRUE if stream could be opened successfully. Also returns TRUE if the decoder is
   * already open and the stream == the stream provided. Returns FALSE if the stream couldn't
   * be opened OR if already open and the stream is NOT the same.
   */
  bool Open(const CodecStream& stream);

  static const rational kAnyTimecode;

  struct RetrieveVideoParams
  {
    Renderer *renderer = nullptr;
    rational time;
    int divider = 1;
    PixelFormat maximum_format = PixelFormat::INVALID;
    CancelAtom *cancelled = nullptr;
    VideoParams::ColorRange force_range = VideoParams::kColorRangeDefault;
    VideoParams::Interlacing src_interlacing = VideoParams::kInterlaceNone;
  };

  /**
   * @brief Retrieves a video frame from footage
   *
   * This function will always return a valid frame unless a fatal error occurs (in such case,
   * nullptr will return). If the timecode is before the start of the footage, this function should
   * return the first frame. Likewise, if it is after the timecode, this function should return the
   * last frame.
   *
   * This function is thread safe and can only run while the decoder is open. \see Open()
   */
  TexturePtr RetrieveVideo(const RetrieveVideoParams& p);

  enum RetrieveAudioStatus {
    kInvalid = -1,
    kOK,
    kWaitingForConform,
    kUnknownError
  };

  /**
   * @brief Retrieve audio data from footage
   *
   * This function will always return a sample buffer unless a fatal error occurs (in such case,
   * nullptr will return). The SampleBuffer should always have enough audio for the range provided.
   *
   * This function is thread safe and can only run while the decoder is open. \see Open()
   */
  RetrieveAudioStatus RetrieveAudio(SampleBuffer &dest, const TimeRange& range, const AudioParams& params, const QString &cache_path, LoopMode loop_mode, RenderMode::Mode mode);

  /**
   * @brief Determine the last time this decoder instance was used in any way
   */
  qint64 GetLastAccessedTime();

  /**
   * @brief Generate a Footage object from a file
   *
   * If this decoder is able to parse this file, it will return a valid FootagePtr. Otherwise, it
   * will return nullptr.
   *
   * For sub-classes, this function should be effectively static. We can't do virtual static
   * functions in C++, but it should hold and access no state during its run.
   *
   * This function is re-entrant.
   */
  virtual FootageDescription Probe(const QString& filename, CancelAtom *cancelled) const = 0;

  /**
   * @brief Closes media/deallocates memory
   *
   * This function is thread safe and can only run while the decoder is open. \see Open()
   */
  void Close();

  /**
   * @brief Conform audio stream
   */
  bool ConformAudio(const QVector<QString> &output_filenames, const AudioParams &params, CancelAtom *cancelled = nullptr);

  /**
   * @brief Create a Decoder instance using a Decoder ID
   *
   * @return
   *
   * A Decoder instance or nullptr if a Decoder with this ID does not exist
   */
  static DecoderPtr CreateFromID(const QString& id);

  static QString TransformImageSequenceFileName(const QString& filename, const int64_t& number);

  static int GetImageSequenceDigitCount(const QString& filename);

  static int64_t GetImageSequenceIndex(const QString& filename);

  static QVector<DecoderPtr> ReceiveListOfAllDecoders();

protected:
  /**
   * @brief Internal open function
   *
   * Sub-classes must override this function. Function will already be mutexed, so there is no need
   * to worry about thread safety. Also many other sanity checks will be done before this, so
   * sub-classes only need to worry about their own opening functions. It is guaranteed that the
   * decoder is not open yet and that the footage stream was from that sub-classes probe function.
   *
   * Return TRUE if everything opened successfully and the decoder is ready to work. Otherwise,
   * return FALSE. If this function returns false, Decoder will call CloseInternal to clean any
   * memory allocated during OpenInternal.
   */
  virtual bool OpenInternal() = 0;

  /**
   * @brief Internal close function
   *
   * Sub-classes must override this function. Function should be able to safely clear all allocated
   * memory. It may be called even if Open() didn't complete or RetrieveVideo() was never called.
   */
  virtual void CloseInternal() = 0;

  /**
   * @brief Internal frame retrieval function
   *
   * Sub-classes must override this function IF they support video. Function is already mutexed
   * so sub-classes don't need to worry about thread safety.
   */
  virtual TexturePtr RetrieveVideoInternal(const RetrieveVideoParams& p);

  virtual bool ConformAudioInternal(const QVector<QString>& filenames, const AudioParams &params, CancelAtom *cancelled);

  void SignalProcessingProgress(int64_t ts, int64_t duration);

  /**
   * @brief Return currently open stream
   *
   * This function is NOT thread safe and should therefore only be called by thread safe functions.
   */
  const CodecStream& stream() const
  {
    return stream_;
  }

  virtual rational GetAudioStartOffset() const { return 0; }

signals:
  /**
   * @brief While indexing, this signal will provide progress as a percentage (0-100 inclusive) if
   * available
   */
  void IndexProgress(double);

private:
  void UpdateLastAccessed();

  bool RetrieveAudioFromConform(SampleBuffer &sample_buffer, const QVector<QString> &conform_filenames, TimeRange range, LoopMode loop_mode, const AudioParams &params);

  CodecStream stream_;

  QMutex mutex_;

  std::atomic_int64_t last_accessed_;

  TexturePtr cached_texture_;
  rational cached_time_;
  int cached_divider_;

};

uint qHash(Decoder::CodecStream stream, uint seed = 0);

}

Q_DECLARE_METATYPE(olive::Decoder::RetrieveState)

#endif // DECODER_H
