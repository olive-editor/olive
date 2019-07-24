#include "qtversionabstraction.h"

int QFontMetricsWidth(const QFontMetrics* fm, const QString& s) {
#if QT_VERSION < QT_VERSION_CHECK(5, 11, 0)
  return fm->width(s);
#else
  return fm->horizontalAdvance(s);
#endif
}
