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

#ifndef FOOTAGE_H
#define FOOTAGE_H

#include <QList>
#include <QDateTime>

#include "common/rational.h"
#include "project/item/item.h"
#include "project/item/footage/audiostream.h"
#include "project/item/footage/videostream.h"
#include "timeline/timelinepoints.h"

namespace olive {

/**
 * @brief A reference to an external media file with metadata in a project structure
 *
 * Footage objects serve two purposes: storing metadata about external media and storing it as a project item.
 * Footage objects store a list of Stream objects which store the majority of video/audio metadata. These streams
 * are identical to the stream data in the files.
 */
class Footage : public Item, public TimelinePoints
{
  Q_OBJECT
public:
  /**
   * @brief Footage Constructor
   */
  Footage();

  /**
   * @brief Footage Destructor
   *
   * Makes sure Stream objects are cleared properly
   */
  virtual ~Footage() override;

  /**
   * @brief Load function
   */
  virtual void Load(QXmlStreamReader* reader, XMLNodeData &xml_node_data, uint version, const QAtomicInt *cancelled) override;

  /**
   * @brief Save function
   */
  virtual void Save(QXmlStreamWriter *writer) const override;

  /**
   * @brief Reset Footage state ready for running through Probe() again
   *
   * If a Footage object needs to be re-probed (e.g. source file changes or Footage is linked to a new file), its
   * state needs to be reset so the Decoder::Probe() function can accurately mirror the source file. Clear() will
   * reset the Footage object's state to being freshly created (keeping the filename).
   *
   * In most cases, you'll be using olive::ProbeMedia() for re-probing which already runs Clear(), so you won't need
   * to worry about this.
   */
  void Clear();

  bool IsValid() const
  {
    return valid_;
  }

  /**
   * @brief Sets this footage to valid and ready to use
   */
  void SetValid();

  /**
   * @brief Return the current filename of this Footage object
   */
  const QString& filename() const;

  /**
   * @brief Set the filename
   *
   * NOTE: This does not automtaically clear the old streams and re-probe for new ones. If the file link has been
   * changed, this will need to be done manually.
   *
   * @param s
   *
   * New filename
   */
  void set_filename(const QString& s);

  /**
   * @brief Retrieve the last modified time/date
   *
   * The file's last modified timestamp is stored for potential organization in the ProjectExplorer. It can be
   * retrieved here.
   */
  const qint64 &timestamp() const;

  /**
   * @brief Set the last modified time/date
   *
   * This should probably only be done on import or replace.
   *
   * @param t
   *
   * New last modified time/date
   */
  void set_timestamp(const qint64 &t);

  /**
   * @brief Add a stream metadata object to this footage
   *
   * Usually done during a Decoder::Probe() function for retrieving metadata about the video/audio/other streams
   * inside a container. Streams can have non-video/audio types so that they can be equivalent to the file's actual
   * stream list, though the only streams officially supported are video and audio streams.
   *
   * @param s
   *
   * A pointer to a stream object. The Footage takes ownership of this object and will free it when it's deleted.
   */
  void add_stream(StreamPtr s);

  /**
   * @brief Retrieve a stream at the given index.
   *
   * @param index
   *
   * The index will be equivalent to the stream's index in the file (or in FFmpeg
   * terms AVStream->file_index). Must be < stream_count().
   *
   * @return
   *
   * The stream at the index provided
   */
  StreamPtr stream(int index) const;

  /**
   * @brief Returns a list of the streams in this Footage
   */
  const QList<StreamPtr>& streams() const;

  /**
   * @brief Retrieve total number of streams in this Footage file
   */
  int stream_count() const;

  /**
   * @brief Item::Type() override
   *
   * @return kFootage
   */
  virtual Type type() const override;

  /**
   * @brief Get the Decoder ID set when this Footage was probed
   *
   * @return
   *
   * A decoder ID
   */
  const QString& decoder() const;

  /**
   * @brief Used by decoders when they Probe to attach itself to this Footage
   */
  void set_decoder(const QString& id);

  virtual QIcon icon() override;

  virtual QString duration() override;

  virtual QString rate() override;

  quint64 get_enabled_stream_flags() const;

  /**
   * @brief Check if this footage has streams of a certain type
   *
   * @param type
   *
   * The stream type to check for
   */
  bool HasStreamsOfType(const Stream::Type& type) const;

  StreamPtr get_first_stream_of_type(const Stream::Type& type) const;

  static bool CompareFootageToFile(Footage* footage, const QString& filename);
  static bool CompareFootageToItsFilename(Footage* footage);

private:
  /**
   * @brief Internal function to delete all Stream children and empty the array
   */
  void ClearStreams();

  /**
   * @brief Update the icon based on the Footage status
   *
   * For kUnprobed and kError an appropriate icon will be shown. For kReady, this function will determine what the
   * dominant type of media in this Footage is (video/audio/image) and set the icon accordingly based on that.
   */
  void UpdateIcon();

  /**
   * @brief Update the tooltip based on the Footage status
   *
   * For kUnprobed and kError, this sets an appropriate generic message. For kReady, this function will set
   * basic information about the Footage in the tooltip (based on the results of a previous probe).
   */
  void UpdateTooltip();

  /**
   * @brief Internal filename string
   */
  QString filename_;

  /**
   * @brief Internal timestamp object
   */
  qint64 timestamp_;

  /**
   * @brief Internal streams array
   */
  QList<StreamPtr> streams_;

  /**
   * @brief Internal attached decoder ID
   */
  QString decoder_;

  bool valid_;

};

}

#endif // FOOTAGE_H
