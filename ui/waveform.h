#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <QPainter>
#include <QRect>

#include "timeline/clip.h"
#include "project/footage.h"

namespace olive {
namespace ui {

void DrawWaveform(Clip* clip,
                   const FootageStream *ms,
                   long media_length,
                   QPainter* p,
                   const QRect& clip_rect,
                   int waveform_start,
                   int waveform_limit,
                   double zoom);

}
}

#endif // WAVEFORM_H
