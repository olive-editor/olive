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

#ifndef OCIOBASENODE_H
#define OCIOBASENODE_H

#include "node/node.h"
#include "render/job/colortransformjob.h"

namespace olive {

class OCIOBaseNode : public Node
{
  Q_OBJECT
public:
  OCIOBaseNode();

  virtual void AddedToGraphEvent(Project *p)  override;
  virtual void RemovedFromGraphEvent(Project *p) override;

  virtual void Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const override;

  static const QString kTextureInput;

protected slots:
  virtual void ConfigChanged() = 0;

protected:
  ColorManager *manager() const { return manager_; }

  ColorProcessorPtr processor() const { return processor_; }
  void set_processor(ColorProcessorPtr p) { processor_ = p; }

private:
  ColorManager *manager_;

  ColorProcessorPtr processor_;

};

}

#endif // OCIOBASENODE_H
