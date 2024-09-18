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

#include "transformwrap.h"
#include "widget/slider/floatslider.h"

namespace olive {
	
	const QString TransformWrap::kTextureInput = QStringLiteral("tex_in");

	const QString TransformWrap::kTranslationX = QStringLiteral("u_translation_x");
	const QString TransformWrap::kTranslationY = QStringLiteral("u_translation_y");
	const QString TransformWrap::kRotation = QStringLiteral("u_rotation");
	const QString TransformWrap::kScale = QStringLiteral("u_scale");
	const QString TransformWrap::kTextureWrapMode = QStringLiteral("texture_wrap_mode");

	#define super Node

	TransformWrap::TransformWrap() {
		AddInput(kTextureInput, NodeValue::kTexture, InputFlags(kInputFlagNotKeyframable));
		
		AddInput(kTranslationX, NodeValue::kFloat, 0.0);
		AddInput(kTranslationY, NodeValue::kFloat, 0.0);
		AddInput(kRotation, NodeValue::kFloat, 0.0);
		AddInput(kScale, NodeValue::kFloat, 100.0);
		AddInput(kTextureWrapMode, NodeValue::kCombo, 2, InputFlags(kInputFlagNotConnectable | kInputFlagNotKeyframable));

		SetFlag(kVideoEffect);
		SetEffectInput(kTextureInput);
	}

	QString TransformWrap::Name() const {
		return tr("Transform Wrap");
	}

	QString TransformWrap::id() const {
		return QStringLiteral("org.olivevideoeditor.Olive.TransformWrap");
	}

	QVector<Node::CategoryID> TransformWrap::Category() const {
		return {kCategoryDistort};
	}

	QString TransformWrap::Description() const {
		return tr("Transformations with wrap mode selection");
	}

	void TransformWrap::Retranslate() {
		super::Retranslate();
		SetInputName(kTextureInput, tr("Input Texture"));

		SetInputName(kTranslationX, tr("Translation X"));
		SetInputName(kTranslationY, tr("Translation Y"));
		SetInputName(kRotation, tr("Rotation"));
		SetInputName(kScale, tr("Scale"));
		
		SetInputName(kTextureWrapMode, tr("Wrap Mode"));
		SetComboBoxStrings(kTextureWrapMode, {tr("Clamp"), tr("Repeat"), tr("Mirrored Repeat")});
	}

	void TransformWrap::Value(const NodeValueRow &value, const NodeGlobals &globals, NodeValueTable *table) const {
		
		NodeValue texture_meta = value[kTextureInput];

		if (TexturePtr tex = texture_meta.toTexture()) {
			
			ShaderJob job(value);		
			
			job.Insert(QStringLiteral("resolution_in"), NodeValue(NodeValue::kVec2, tex->virtual_resolution(), this));
			
			int wrapMode = static_cast<Texture::WrapMode>(value[kTextureWrapMode].toInt());
	
			switch (wrapMode) {
				case 0:
					job.SetWrapMode(QStringLiteral("tex_in"), Texture::kClamp);
				break;
				case 1:
					job.SetWrapMode(QStringLiteral("tex_in"), Texture::kRepeat);
				break;
				case 2:
					job.SetWrapMode(QStringLiteral("tex_in"), Texture::kMirroredRepeat);
				break;
			}
			

			table->Push(NodeValue::kTexture, tex->toJob(job), this);
		}
	}

	ShaderCode TransformWrap::GetShaderCode(const ShaderRequest &request) const {
		Q_UNUSED(request)
		return ShaderCode(FileFunctions::ReadFileAsString(":/shaders/transformation.frag"));
	}
}
