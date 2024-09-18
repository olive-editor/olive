/***
 
	Olive - Non-Linear Video Editor
	Copyright (C) 2024 Johann JEG (johannjeg1057@gmail.com)

	This program is free software: you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation, either version 3 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef TRANSFORM_WRAP_H
#define TRANSFORM_WRAP_H

#include "node/node.h"

namespace olive {

	class TransformWrap : public Node {
		
		Q_OBJECT
		
		public:
		
			TransformWrap();

			NODE_DEFAULT_FUNCTIONS(TransformWrap)

			virtual QString Name() const override;
			virtual QString id() const override;
			virtual QVector<CategoryID> Category() const override;
			virtual QString Description() const override;

			virtual void Retranslate() override;

			virtual void Value(const NodeValueRow& value, const NodeGlobals &globals, NodeValueTable *table) const override;
			virtual ShaderCode GetShaderCode(const ShaderRequest &request) const override;

			static const QString kTextureInput;

			static const QString kTranslationX;
			static const QString kTranslationY;
			static const QString kRotation;
			static const QString kScale;
			static const QString kTextureWrapMode;
	};

}

#endif // TRANSFORM_WRAP_H
