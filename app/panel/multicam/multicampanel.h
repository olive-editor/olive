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

#ifndef MULTICAMPANEL_H
#define MULTICAMPANEL_H

#include "panel/viewer/viewerbase.h"
#include "widget/multicam/multicamwidget.h"

namespace olive
{

class MulticamPanel : public TimeBasedPanel {
	Q_OBJECT
public:
	MulticamPanel();

	MulticamWidget *GetMulticamWidget() const
	{
		return static_cast<MulticamWidget *>(GetTimeBasedWidget());
	}

protected:
	virtual void Retranslate() override;
};

}

#endif // MULTICAMPANEL_H
