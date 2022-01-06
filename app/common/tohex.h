#ifndef TOHEX_H
#define TOHEX_H

#include <QtGlobal>

#include "common/define.h"

namespace olive {

inline QString ToHex(quint64 t) {
  return QStringLiteral("%1").arg(t, 0, 16);
}

}

#endif // TOHEX_H
