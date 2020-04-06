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

#ifndef INPUTARRAY_H
#define INPUTARRAY_H

#include "input.h"

OLIVE_NAMESPACE_ENTER

class NodeInputArray : public NodeInput
{
  Q_OBJECT
public:
  NodeInputArray(const QString &id, const DataType& type, const QVariant& default_value = 0);

  virtual bool IsArray() override;

  int GetSize() const;

  void Prepend();
  void Append();
  void InsertAt(int index);
  void RemoveLast();
  void RemoveAt(int index);
  void SetSize(int size);

  bool ContainsSubParameter(NodeInput* input) const;
  int IndexOfSubParameter(NodeInput* input) const;

  NodeInput* First() const;
  NodeInput* Last() const;
  NodeInput* At(int index) const;

  const QVector<NodeInput*>& sub_params();

signals:
  void SizeChanged(int size);

protected:
  virtual void LoadInternal(QXmlStreamReader* reader, XMLNodeData& xml_node_data, const QAtomicInt *cancelled) override;

  virtual void SaveInternal(QXmlStreamWriter* writer) const override;

private:
  QVector<NodeInput*> sub_params_;

  QVariant default_value_;

};

OLIVE_NAMESPACE_EXIT

#endif // INPUTARRAY_H
