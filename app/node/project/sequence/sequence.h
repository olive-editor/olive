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

#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "node/output/track/tracklist.h"
#include "node/output/viewer/viewer.h"

namespace olive {

/**
 * @brief The main timeline object, an graph of edited clips that forms a complete edit
 */
class Sequence : public ViewerOutput
{
  Q_OBJECT
public:
  Sequence();

  NODE_DEFAULT_FUNCTIONS(Sequence)

  virtual QString Name() const override
  {
    return tr("Sequence");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.sequence");
  }

  virtual QVector<CategoryID> Category() const override
  {
    return {kCategoryProject};
  }

  virtual QString Description() const override
  {
    return tr("A series of cuts that result in an edited video. Also called a timeline.");
  }

  void add_default_nodes(MultiUndoCommand *command = nullptr);

  virtual QVariant data(const DataType &d) const override;

  const QVector<Track *> &GetTracks() const
  {
    return track_cache_;
  }

  Track* GetTrackFromReference(const Track::Reference& track_ref) const
  {
    return track_lists_.at(track_ref.type())->GetTrackAt(track_ref.index());
  }

  /**
   * @brief Same as GetTracks() but omits tracks that are locked.
   */
  QVector<Track *> GetUnlockedTracks() const;

  TrackList* track_list(Track::Type type) const
  {
    return track_lists_.at(type);
  }

  virtual void Retranslate() override;

  virtual void InvalidateCache(const TimeRange& range, const QString& from, int element, InvalidateCacheOptions options) override;

  static const QString kTrackInputFormat;

protected:
  virtual void InputConnectedEvent(const QString &input, int element, const NodeOutput &output) override;

  virtual void InputDisconnectedEvent(const QString &input, int element, const NodeOutput &output) override;

  virtual rational VerifyLengthInternal(Track::Type type) const override;

signals:
  void TrackAdded(Track* track);
  void TrackRemoved(Track* track);

  void SubtitlesChanged(const TimeRange &range);

private:
  QVector<TrackList*> track_lists_;

  QVector<Track*> track_cache_;

private slots:
  void UpdateTrackCache();

};

}

#endif // SEQUENCE_H
