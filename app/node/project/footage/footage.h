/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2021 Olive Team

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
#include "footagedescription.h"
#include "node/output/viewer/viewer.h"
#include "render/audioparams.h"
#include "render/videoparams.h"
#include "timeline/timelinepoints.h"

namespace olive {

/**
 * @brief A reference to an external media file with metadata in a project structure
 *
 * Footage objects serve two purposes: storing metadata about external media and storing it as a project item.
 * Footage objects store a list of Stream objects which store the majority of video/audio metadata. These streams
 * are identical to the stream data in the files.
 */
class Footage : public ViewerOutput
{
  Q_OBJECT
public:
  enum LoopMode {
    kLoopModeOff,
    kLoopModeLoop,
    kLoopModeClamp
  };

  /**
   * @brief Footage Constructor
   */
  Footage(const QString& filename = QString());

  NODE_DEFAULT_DESTRUCTOR(Footage)

  virtual Node* copy() const override
  {
    return new Footage();
  }

  virtual QString Name() const override
  {
    return tr("Media");
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
   * @brief Get currently set loop mode
   */
  LoopMode loop_mode() const;

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

  int GetStreamIndex(Track::Type type, int index) const;
  int GetStreamIndex(const Track::Reference& ref) const
  {
    return GetStreamIndex(ref.type(), ref.index());
  }

  Track::Reference GetReferenceFromRealIndex(int real_index) const;

  /**
   * @brief Get the Decoder ID set when this Footage was probed
   *
   * @return
   *
   * A decoder ID
   */
  const QString& decoder() const;

  virtual QIcon icon() const override;

  virtual bool IsItem() const override
  {
    return true;
  }

  static QString DescribeVideoStream(const VideoParams& params);
  static QString DescribeAudioStream(const AudioParams& params);

  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  static QString GetStreamTypeName(Track::Type type);

  virtual Node *GetConnectedTextureOutput() override;

  virtual Node *GetConnectedSampleOutput() override;

  static rational AdjustTimeByLoopMode(rational time, LoopMode loop_mode, const rational& length, VideoParams::Type type, const rational &timebase);

  virtual qint64 creation_time() const override;
  virtual qint64 mod_time() const override;

  static const QString kFilenameInput;
  static const QString kLoopModeInput;

protected:
  virtual void InputValueChangedEvent(const QString &input, int element) override;

  virtual rational VerifyLengthInternal(Track::Type type) const override;

  virtual void Hash(QCryptographicHash &hash, const NodeGlobals &globals, const VideoParams& video_params) const override;

private:
  QString GetColorspaceToUse(const VideoParams& params) const;

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
   * @brief Internal timestamp object
   */
  qint64 timestamp_;

  /**
   * @brief Internal attached decoder ID
   */
  QString decoder_;

  bool valid_;

  const QAtomicInt* cancelled_;

private slots:
  void CheckFootage();

};

}

#endif // FOOTAGE_H
