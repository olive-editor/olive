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

#include "nodeviewtoolbar.h"

#include <QEvent>
#include <QHBoxLayout>

#include "ui/icons/icons.h"

namespace olive
{

#define super QWidget

NodeViewToolBar::NodeViewToolBar(QWidget *parent)
	: QWidget(parent)
{
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	add_node_btn_ = new QPushButton();
	connect(add_node_btn_, &QPushButton::clicked, this,
			&NodeViewToolBar::AddNodeClicked);
	layout->addWidget(add_node_btn_);

	minimap_btn_ = new QPushButton();
	minimap_btn_->setCheckable(true);
	connect(minimap_btn_, &QPushButton::clicked, this,
			&NodeViewToolBar::MiniMapEnabledToggled);
	layout->addWidget(minimap_btn_);

	layout->addStretch();

	Retranslate();
	UpdateIcons();
}

void NodeViewToolBar::changeEvent(QEvent *e)
{
	if (e->type() == QEvent::LanguageChange) {
		Retranslate();
	} else if (e->type() == QEvent::StyleChange) {
		UpdateIcons();
	}
	super::changeEvent(e);
}

void NodeViewToolBar::Retranslate()
{
	add_node_btn_->setToolTip(tr("Add Node"));
	minimap_btn_->setToolTip(tr("Toggle Mini-Map"));
}

void NodeViewToolBar::UpdateIcons()
{
	add_node_btn_->setIcon(icon::Add);
	minimap_btn_->setIcon(icon::MiniMap);
}

}
