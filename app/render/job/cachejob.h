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

#ifndef CACHEJOB_H
#define CACHEJOB_H

#include <QString>
#include <QVariant>

#include "node/value.h"
#include "render/job/acceleratedjob.h"

namespace olive {

class CacheJob : public AcceleratedJob
{
public:
  CacheJob() = default;
  CacheJob(const QString &filename, const NodeValue &fallback = NodeValue())
  {
    filename_ = filename;
  }

  const QString &GetFilename() const { return filename_; }
  void SetFilename(const QString &s) { filename_ = s; }

  const NodeValue &GetFallback() const { return fallback_; }
  void SetFallback(const NodeValue &val) { fallback_ = val; }

private:
  QString filename_;

  NodeValue fallback_;

};

}

#endif // CACHEJOB_H
