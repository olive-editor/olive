#include "probeserver.h"

#include <QApplication>
#include <QDebug>

#include "decoder/ffmpeg/ffmpegdecoder.h"

bool olive::ProbeMedia(Footage *f)
{
  // Check for a valid filename
  if (f->filename().isEmpty()) {
    qWarning() << QCoreApplication::translate("ProbeMedia", "Tried to probe media with an empty filename");
    return false;
  }

  FFmpegDecoder ff_dec;

  // Pass footage through FFmpeg's probe function
  if (ff_dec.Probe(f)) {
    return true;
  }

  return false;
}
