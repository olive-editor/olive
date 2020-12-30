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

#ifndef SEQUENCE_H
#define SEQUENCE_H

#include "common/rational.h"
#include "node/graph.h"
#include "node/output/viewer/viewer.h"
#include "render/videoparams.h"
#include "project/item/footage/stream.h"
#include "project/item/item.h"
#include "timeline/timelinepoints.h"

namespace olive {

/**
 * @brief The main timeline object, an graph of edited clips that forms a complete edit
 */
class Sequence : public NodeGraph, public TimelinePoints
{
  Q_OBJECT
public:
  Sequence();

  /**
   * @brief Load function
   */
  virtual void Load(QXmlStreamReader* reader, XMLNodeData &xml_node_data, uint version, const QAtomicInt* cancelled) override;

  /**
   * @brief Save function
   */
  virtual void Save(QXmlStreamWriter *writer) const override;

  void add_default_nodes();

  /**
   * @brief Item::Type() override
   */
  virtual Type type() const override;

  virtual QIcon icon() override;

  virtual QString duration() override;
  virtual QString rate() override;

  const VideoParams &video_params() const;
  void set_video_params(const VideoParams &vparam);

  const AudioParams& audio_params() const;
  void set_audio_params(const AudioParams& params);

  void set_default_parameters();

  void set_parameters_from_footage(const QList<Footage*> footage);

  ViewerOutput* viewer_output() const;

protected:
  virtual void NameChangedEvent(const QString& name) override;

private:
  ViewerOutput* viewer_output_;

};

}

#endif // SEQUENCE_H
