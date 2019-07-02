/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "project/item/item.h"
#include "project/item/footage/audiostream.h"
#include "project/item/footage/videostream.h"
#include "rational.h"

class Footage : public Item
{
public:
  enum Status {
    kUnprobed,
    kReady,
    kInvalid
  };

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
   * @brief Deleted copy constructor
   */
  Footage(const Footage& other) = delete;

  /**
   * @brief Deleted move constructor
   */
  Footage(Footage&& other) = delete;

  /**
   * @brief Deleted copy assignment
   */
  Footage& operator=(const Footage& other) = delete;

  /**
   * @brief Deleted move assignment
   */
  Footage& operator=(Footage&& other) = delete;

  /**
   * @brief Check the ready state of this Footage object
   *
   * @return
   *
   * If the Footage has been successfully probed, this will return TRUE.
   */
  const Status& status();

  /**
   * @brief Set ready state
   *
   * This should only be set by olive::ProbeMedia. Sets the Footage's current status to a member of enum
   * Footage::Status.
   *
   * This function also runs UpdateIcon() and UpdateTooltip(). If you need to override the tooltip (e.g. for an error
   * message), you must run set_tooltip() *after* running set_status();
   */
  void set_status(const Status& status);

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

  /**
   * @brief Return the current filename of this Footage object
   */
  const QString& filename();

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
  const QDateTime& timestamp();

  /**
   * @brief Set the last modified time/date
   *
   * This should probably only be done on import or replace.
   *
   * @param t
   *
   * New last modified time/date
   */
  void set_timestamp(const QDateTime& t);

  const rational& duration();

  void set_duration(const rational& duration);

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
  void add_stream(Stream* s);

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
  const Stream* stream(int index);

  /**
   * @brief Retrieve total number of streams in this Footage file
   */
  int stream_count();

  /**
   * @brief Item::Type() override
   *
   * @return kFootage
   */
  virtual Type type() const override;

private:
  /**
   * @brief Internal function to delete all Stream children and empty the array
   */
  void ClearStreams();

  /**
   * @brief Check if this footage has streams of a certain type
   *
   * @param type
   *
   * The stream type to check for
   */
  bool HasStreamsOfType(const Stream::Type type);

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
  QDateTime timestamp_;

  /**
   * @brief Internal streams array
   */
  QList<Stream*> streams_;

  /**
   * @brief Internal ready setting
   */
  Status status_;

  /**
   * @brief Media duration
   */
  rational duration_;
};

#endif // FOOTAGE_H
