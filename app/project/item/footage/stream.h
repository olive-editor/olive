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

#ifndef STREAM_H
#define STREAM_H

#include <memory>
#include <QCoreApplication>
#include <QMutex>
#include <QString>
#include <QXmlStreamWriter>

#include "common/rational.h"
#include "ui/icons/icons.h"

OLIVE_NAMESPACE_ENTER

class Footage;
class Stream;
using StreamPtr = std::shared_ptr<Stream>;
struct XMLNodeData;

/**
 * @brief A base class for keeping metadata about a media stream.
 *
 * A Stream can contain video data, audio data, subtitle data,
 * etc. and a Stream object stores metadata about it.
 *
 * The Stream class is fairly simple and is intended to be subclassed for data that pertains specifically to one
 * Stream::Type. \see VideoStream and \see AudioStream.
 */
class Stream : public QObject
{
  Q_OBJECT
public:
  enum Type {
    kUnknown,
    kVideo,
    kAudio,
    kData,
    kSubtitle,
    kAttachment
  };

  /**
   * @brief Stream constructor
   */
  Stream();

  /**
   * @brief Required virtual destructor, serves no purpose
   */
  virtual ~Stream() override;

  static StreamPtr Load(QXmlStreamReader* reader, XMLNodeData& xml_node_data, const QAtomicInt* cancelled);

  void Save(QXmlStreamWriter *writer) const;

  virtual QString description() const;

  const Type& type() const;
  void set_type(const Type& type);

  Footage* footage() const;
  void set_footage(Footage* f);

  const rational& timebase() const;
  void set_timebase(const rational& timebase);

  const int& index() const;
  void set_index(const int& index);

  const int64_t& duration() const;
  void set_duration(const int64_t& duration);

  bool enabled() const;
  void set_enabled(bool e);

  virtual QIcon icon() const;

  QMutex* proxy_access_lock();

protected:
  virtual void LoadCustomParameters(QXmlStreamReader *reader);

  virtual void SaveCustomParameters(QXmlStreamWriter* writer) const;

signals:
  void ParametersChanged();

private:
  Footage* footage_;

  rational timebase_;

  int64_t duration_;

  int index_;

  Type type_;

  bool enabled_;

  QMutex proxy_access_lock_;

};

OLIVE_NAMESPACE_EXIT

#include <QMetaType>
Q_DECLARE_METATYPE(OLIVE_NAMESPACE::StreamPtr)

#endif // STREAM_H
