/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#ifndef VOIDEFFECT_H
#define VOIDEFFECT_H

/* VoidEffect is a placeholder used when Olive is unable to find an effect
 * requested by a loaded project. It displays a missing effect so the user knows
 * an effect is missing, and stores the XML project data verbatim so that it
 * isn't lost if the user saves over the project.
 */

#include "project/effect.h"

class VoidEffect : public Effect {
    Q_OBJECT
public:
	VoidEffect(Clip* c, const QString& n);

	virtual Effect* copy(Clip* c) override;
	virtual void load(QXmlStreamReader &stream) override;
	virtual void save(QXmlStreamWriter &stream) override;
private:
	QByteArray bytes;
	EffectMeta void_meta;
};

#endif // VOIDEFFECT_H
