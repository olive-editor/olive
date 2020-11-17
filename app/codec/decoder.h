/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2020 Olive Team

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

#include <QMutex>
#include <QObject>
#include <QWaitCondition>
#include <stdint.h>

#include "codec/frame.h"
#include "codec/samplebuffer.h"
#include "codec/waveoutput.h"
#include "common/rational.h"
#include "project/item/footage/footage.h"

namespace olive {

class Decoder;
using DecoderPtr = std::shared_ptr<Decoder>;

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
  virtual QString id() = 0;

  virtual bool SupportsVideo(){return false;}
  virtual bool SupportsAudio(){return false;}

  /**
   * @brief Open stream for decoding
   *
   * This function is thread safe.
   *
   * Returns TRUE if stream could be opened successfully. Also returns TRUE if the decoder is
   * already open and the stream == the stream provided. Returns FALSE if the stream couldn't
   * be opened OR if already open and the stream is NOT the same.
   */
  bool Open(StreamPtr fs);

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
  FramePtr RetrieveVideo(const rational& timecode, const int& divider);

  /**
   * @brief Retrieve audio data from footage
   *
   * This function will always return a sample buffer unless a fatal error occurs (in such case,
   * nullptr will return). The SampleBuffer should always have enough audio for the range provided.
   *
   * This function is thread safe and can only run while the decoder is open. \see Open()
   */
  SampleBufferPtr RetrieveAudio(const TimeRange& range, const AudioParams& params, const QAtomicInt *cancelled);

  /**
   * @brief Try to probe a Footage file by passing it through all available Decoders
   *
   * This is a helper function designed to abstract the process of communicating with several Decoders from the rest of
   * the application. This function will take a Footage file and manually pass it through the available Decoders' Probe()
   * functions until one indicates that it can decode this file. That Decoder will then dump information about the file
   * into the Footage object for use throughout the program.
   *
   * Probing may be a lengthy process and it's recommended to run this in a separate thread.
   *
   * @param f
   *
   * A Footage object with a valid filename. If the Footage does not have a valid filename (e.g. is empty or file doesn't
   * exist), this function will return FALSE.
   *
   * @return
   *
   * TRUE if a Decoder was successfully able to parse and probe this file. FALSE if not.
   */
  static FootagePtr Probe(Project *project, const QString& filename, const QAtomicInt *cancelled);

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
  virtual FootagePtr Probe(const QString& filename, const QAtomicInt* cancelled) const = 0;

  /**
   * @brief Closes media/deallocates memory
   *
   * This function is thread safe and can only run while the decoder is open. \see Open()
   */
  void Close();

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
  virtual FramePtr RetrieveVideoInternal(const rational& timecode, const int& divider);

  virtual bool ConformAudioInternal(const QString& filename, const AudioParams &params, const QAtomicInt* cancelled);

  void SignalProcessingProgress(const int64_t& ts);

  /**
   * @brief Get the destination filename of an audio stream conformed to a set of parameters
   */
  QString GetConformedFilename(const AudioParams &params);

  QString GetIndexFilename();

  struct CurrentlyConforming {
    StreamPtr stream;
    AudioParams params;

    bool operator==(const CurrentlyConforming& rhs) const
    {
      return this->stream == rhs.stream && this->params == rhs.params;
    }
  };

  /**
   * @brief Return currently open stream
   *
   * This function is NOT thread safe and should therefore only be called by thread safe functions.
   */
  StreamPtr stream() const
  {
    return stream_;
  }

  static QMutex currently_conforming_mutex_;
  static QWaitCondition currently_conforming_wait_cond_;
  static QVector<CurrentlyConforming> currently_conforming_;

signals:
  /**
   * @brief While indexing, this signal will provide progress as a percentage (0-100 inclusive) if
   * available
   */
  void IndexProgress(double);

private:
  SampleBufferPtr RetrieveAudioFromConform(const QString& conform_filename, const TimeRange &range);

  StreamPtr stream_;

  QMutex mutex_;

};

}

Q_DECLARE_METATYPE(olive::Decoder::RetrieveState)

#endif // DECODER_H
