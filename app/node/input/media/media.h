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

#ifndef MEDIAINPUT_H
#define MEDIAINPUT_H

#include "codec/decoder.h"
#include "node/node.h"
#include "project/item/footage/stream.h"

namespace olive {

/**
 * @brief A node that imports an image
 */
class MediaInput : public Node
{
  Q_OBJECT
public:
  MediaInput();

  virtual QString Name() const override
  {
    return tr("Media");
  }

  virtual QString id() const override
  {
    return QStringLiteral("org.olivevideoeditor.Olive.mediainput");
  }

  virtual QString Description() const override
  {
    return tr("Import footage into the node graph.");
  }

  virtual Node* copy() const override
  {
    return new MediaInput();
  }

  virtual QVector<CategoryID> Category() const override;

  Stream* stream() const;
  void SetStream(Stream *s);

  virtual bool IsMedia() const override;

  virtual void Retranslate() override;

  virtual NodeValueTable Value(NodeValueDatabase& value) const override;

protected:
  NodeInput* footage_input_;

  Stream* connected_footage_;

private slots:
  void FootageChanged();

  void FootageParametersChanged();

};

}

#endif // MEDIAINPUT_H
