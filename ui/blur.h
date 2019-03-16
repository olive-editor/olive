#ifndef BLUR_H
#define BLUR_H

#include <QImage>
#include <QRect>

namespace olive {
  namespace ui {
    /**
     * @brief Convenience function for blurring a QImage
     *
     * @param result
     *
     * QImage to blur
     *
     * @param rect
     *
     * The rectangle of the QImage to blur. Use QImage::rect() to blur the entire image.
     *
     * @param radius
     *
     * The blur radius - how much to blur the image.
     *
     * @param alphaOnly
     *
     * True if only the alpha channel should be blurred rather than the entire RGBA space.
     */
    void blur(QImage& result, const QRect& rect, int radius, bool alphaOnly);
  }
}

#endif // BLUR_H
