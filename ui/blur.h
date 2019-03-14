#ifndef BLUR_H
#define BLUR_H

#include <QImage>
#include <QRect>

namespace olive {
  namespace ui {
    void blur(QImage& result, const QRect& rect, int radius, bool alphaOnly);
  }
}

#endif // BLUR_H
