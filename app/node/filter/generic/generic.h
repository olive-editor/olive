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

#ifndef GENERICFILTERNODE_H
#define GENERICFILTERNODE_H

#include "node/node.h"

namespace olive {

class GenericFilterNode : public Node
{
  Q_OBJECT
public:
  // constructor accepts the full path of GLSL frag file
  GenericFilterNode( const QString & srcFilePath);

  NODE_DEFAULT_DESTRUCTOR(GenericFilterNode)

  virtual Node* copy() const override;

  virtual QString Name() const override;
  virtual QString id() const override;
  virtual QVector<CategoryID> Category() const override;
  virtual QString Description() const override;

  virtual void Retranslate() override;

  virtual ShaderCode GetShaderCode(const QString &shader_id) const override;
  virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;

  static const QString kTextureInput;

private:
  void searchParametersInSourceFile(const QString &src_file_path);
  NodeValue::Type typeFromName( const QString & paramName);

  QString src_file_path_;
  QString filter_name_;
};

}

#endif // GENERICFILTERNODE_H
