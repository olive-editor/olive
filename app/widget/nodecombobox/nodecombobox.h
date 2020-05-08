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

#ifndef NODECOMBOBOX_H
#define NODECOMBOBOX_H

#include <QComboBox>

#include "node/node.h"

OLIVE_NAMESPACE_ENTER

class NodeComboBox : public QComboBox
{
  Q_OBJECT
public:
  NodeComboBox(QWidget* parent = nullptr);

  virtual void showPopup() override;

  const QString& GetSelectedNode() const;

public slots:
  void SetNode(const QString& id);

protected:
  virtual void changeEvent(QEvent *e) override;

signals:
  void NodeChanged(const QString& id);

private:
  void UpdateText();

  void SetNodeInternal(const QString& id, bool emit_signal);

  QString selected_id_;

};

OLIVE_NAMESPACE_EXIT

#endif // FOOTAGECOMBOBOX_H
