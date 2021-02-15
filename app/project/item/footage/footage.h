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
#include "node/node.h"
#include "project/item/item.h"
#include "stream.h"
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
  Footage(const QString& filename = QString());

  virtual Node* copy() const override
  {
    return new Footage();
  }

  virtual QString Name() const override
  {
    return tr("Footage");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.footage");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryProject};
  }

  virtual QString Description() const override
  {
    return tr("Import video, audio, or still image files into the composition.");
  }

  virtual void Retranslate() override;

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
  QString filename() const;

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

  void SetCancelPointer(const QAtomicInt* c)
  {
    cancelled_ = c;
  }

  class StreamReference
  {
  public:
    StreamReference()
    {
      footage_ = nullptr;
      type_ = Stream::kUnknown;
      index_ = -1;
    }

    StreamReference(const Footage* footage, Stream::Type type, int index)
    {
      footage_ = footage;
      type_ = type;
      index_ = index;
    }

    bool operator==(const StreamReference& rhs) const
    {
      return footage_ == rhs.footage_ && type_ == rhs.type_ && index_ == rhs.index_;
    }

    bool IsValid() const
    {
      return footage_ && index_ >= 0;
    }

    void Reset()
    {
      *this = StreamReference();
    }

    const Footage* footage() const
    {
      return footage_;
    }

    Stream::Type type() const
    {
      return type_;
    }

    int index() const
    {
      return index_;
    }

    Stream GetStream() const
    {
      if (IsValid()) {
        return footage_->GetStreamAt(*this);
      } else {
        return Stream();
      }
    }

    int64_t GetTimeInTimebaseUnits(const rational& timecode) const
    {
      if (IsValid()) {
        return footage_->GetTimeInTimebaseUnits(type_, index_, timecode);
      } else {
        return -1;
      }
    }

    int GetRealStreamIndex() const
    {
      if (IsValid()) {
        return footage_->GetRealStreamIndex(*this);
      } else {
        return -1;
      }
    }

    QString filename() const
    {
      if (footage_) {
        return footage_->filename();
      } else {
        return QString();
      }
    }

    VideoParams video_params() const
    {
      return GetStream().video_params();
    }

    AudioParams audio_params() const
    {
      return GetStream().audio_params();
    }

    int64_t duration() const
    {
      return GetStream().duration();
    }

    QString video_colorspace(bool default_if_empty = true) const;

  private:
    const Footage* footage_;
    Stream::Type type_;
    int index_;

  };

  Stream GetStreamAt(int index) const
  {
    return GetStandardValue(GetInputIDOfIndex(index)).value<Stream>();
  }

  Stream GetStreamAt(Stream::Type type, int index_within_type) const
  {
    return GetStreamAt(GetRealStreamIndex(type, index_within_type));
  }

  Stream GetStreamAt(const StreamReference& ref) const
  {
    return GetStreamAt(ref.type(), ref.index());
  }

  void SetStreamAt(int index, const Stream& stream)
  {
    SetStandardValue(GetInputIDOfIndex(index), QVariant::fromValue(stream));
  }

  void SetStreamAt(Stream::Type type, int index_within_type, const Stream& stream)
  {
    SetStreamAt(GetRealStreamIndex(type, index_within_type), stream);
  }

  void SetStreamAt(const StreamReference& ref, const Stream& stream)
  {
    SetStreamAt(ref.type(), ref.index(), stream);
  }

  int64_t GetTimeInTimebaseUnits(int index, const rational& time) const;
  int64_t GetTimeInTimebaseUnits(Stream::Type type, int index_within_type, const rational& time) const
  {
    return GetTimeInTimebaseUnits(GetRealStreamIndex(type, index_within_type), time);
  }

  int GetRealStreamIndex(Stream::Type type, int index_within_type) const;
  int GetRealStreamIndex(const StreamReference& ref) const
  {
    return GetRealStreamIndex(ref.type(), ref.index());
  }

  static QString GetStringFromReference(Stream::Type type, int index);
  static QString GetStringFromReference(const StreamReference& ref)
  {
    return GetStringFromReference(ref.type(), ref.index());
  }

  StreamReference GetReferenceFromRealIndex(int real_index) const;

  Stream::Type GetTypeFromOutput(const QString& output) const;

  StreamReference GetReferenceFromOutput(const QString& s) const;

  int GetStreamCount() const
  {
    return stream_count_;
  }

  int GetStreamTypeCount(Stream::Type type) const;

  bool IsStreamEnabled(int index) const
  {
    return GetStreamAt(index).enabled();
  }

  Stream GetFirstEnabledStreamOfType(Stream::Type type) const;

  QVector<int> GetStreamIndexesOfType(Stream::Type type) const;

  Stream::Type GetStreamType(int index);

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

  virtual QIcon icon() const override;

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
  bool HasEnabledStreamsOfType(const Stream::Type& type) const;

  static bool CompareFootageToFile(Footage* footage, const QString& filename);
  static bool CompareFootageToItsFilename(Footage* footage);

  virtual void Hash(const QString& output, QCryptographicHash &hash, const rational &time) const override;

  virtual NodeValueTable Value(const QString &output, NodeValueDatabase& value) const override;

  static QString GetStreamTypeName(Stream::Type type);

  static const QString kFilenameInput;
  static const QString kStreamPropertiesFormat;

protected:
  /**
   * @brief Load function
   */
  virtual void LoadInternal(QXmlStreamReader* reader, XMLNodeData &xml_node_data, uint version, const QAtomicInt *cancelled) override;

  /**
   * @brief Save function
   */
  virtual void SaveInternal(QXmlStreamWriter *writer) const override;

  virtual void InputValueChangedEvent(const QString &input, int element) override;

private:
  QString DescribeStream(int index) const;

  struct MetadataCache {
    QString decoder;
    Streams streams;
  };

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

  MetadataCache LoadStreamCache(const QString& filename);

  bool SaveStreamCache(const QString& filename, const MetadataCache& data);

  static QString GetInputIDOfIndex(int index)
  {
    return kStreamPropertiesFormat.arg(index);
  }

  /**
   * @brief List of dynamic inputs added for stream properties
   */
  QMap<int, QString> inputs_for_stream_properties_;

  /**
   * @brief List of dynamic outputs added for streams
   */
  QMap<int, QString> outputs_for_streams_;

  /**
   * @brief Internal timestamp object
   */
  qint64 timestamp_;

  /**
   * @brief Internal attached decoder ID
   */
  QString decoder_;

  int stream_count_;

  bool valid_;

  const QAtomicInt* cancelled_;

private slots:
  void CheckFootage();

};

uint qHash(const Footage::StreamReference& ref, uint seed = 0);

}

Q_DECLARE_METATYPE(olive::Footage::StreamReference)

#endif // FOOTAGE_H
