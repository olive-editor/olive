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

#ifndef MERGENODE_H
#define MERGENODE_H

#include "node/node.h"

OLIVE_NAMESPACE_ENTER

class MergeNode : public Node
{
public:
  MergeNode();

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QList<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual Capabilities GetCapabilities(const NodeValueDatabase&) const override;
  virtual QString ShaderFragmentCode(const NodeValueDatabase&) const override;

  NodeInput* base_in() const;
  NodeInput* blend_in() const;

  virtual void Hash(QCryptographicHash &hash, const rational &time) const override;

private:
  NodeInput* base_in_;

  NodeInput* blend_in_;

};

OLIVE_NAMESPACE_EXIT

#endif // MERGENODE_H
