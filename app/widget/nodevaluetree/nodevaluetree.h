/*
 * Oak Video Editor - Non-Linear Video Editor
 * Copyright (C) 2025 Olive CE Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NODEVALUETREE_H
#define NODEVALUETREE_H

#include <QRadioButton>
#include <QTreeWidget>

#include "node/node.h"

namespace olive
{

class NodeValueTree : public QTreeWidget {
	Q_OBJECT
public:
	NodeValueTree(QWidget *parent = nullptr);

	void SetNode(const NodeInput &input, const rational &time);

protected:
	virtual void changeEvent(QEvent *event) override;

private:
	void Retranslate();

private slots:
	void RadioButtonChecked(bool e);
};

}

#endif // NODEVALUETREE_H
