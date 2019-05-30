#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>

#include "global/rational.h"
#include "project/footage.h"
#include "frame.h"

/**
 * @brief The Decoder class
 *
 * A decoder's is the main class for bringing external media into Olive. Its responsibilities are to serve as
 * abstraction from codecs/decoders and provide complete frames. These frames can be video or audio data and are
 * provided as Frame objects in shared pointers to alleviate the responsibility of memory handling.
 *
 * A decoder does NOT perform any pixel format conversion
 */
class Decoder : public QObject
{
  Q_OBJECT
public:
  Decoder();

  Decoder(FootageStream* fs);

  virtual ~Decoder();

  FootageStream* stream();
  void set_stream(FootageStream* fs);

  virtual bool Open() = 0;
  virtual FramePtr Retrieve(const rational& timecode, const rational& length = 0) = 0;
  virtual void Close() = 0;

protected:
  bool open_;

private:
  FootageStream* stream_;
};

using DecoderPtr = std::shared_ptr<Decoder>;

#endif // DECODER_H
