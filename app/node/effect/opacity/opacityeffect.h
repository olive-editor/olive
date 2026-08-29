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

#ifndef OPACITYEFFECT_H
#define OPACITYEFFECT_H

#include "node/group/group.h"

namespace olive
{

class OpacityEffect : public Node {
public:
	OpacityEffect();

	NODE_DEFAULT_FUNCTIONS(OpacityEffect)

	virtual QString Name() const override
	{
		return tr("Opacity");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.olivevideoeditor.Olive.opacity");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return { kCategoryFilter };
	}

	virtual QString Description() const override
	{
		return tr(
			"Alter a video's opacity.\n\nThis is equivalent to multiplying a video by a number between 0.0 and 1.0.");
	}

	virtual void Retranslate() override;

	virtual ShaderCode
	GetShaderCode(const ShaderRequest &request) const override;
	virtual void Value(const NodeValueRow &value, const NodeGlobals &globals,
					   NodeValueTable *table) const override;

	static const QString kTextureInput;
	static const QString kValueInput;
};

}

#endif // OPACITYEFFECT_H
