#ifndef TOHEX_H
#define TOHEX_H

#include <QtGlobal>

#include "common/define.h"

OLIVE_NAMESPACE_ENTER

inline QString ToHex(quint64 t) {
  return QStringLiteral("%1").arg(t, 0, 16);
}

OLIVE_NAMESPACE_EXIT

#endif // TOHEX_H
