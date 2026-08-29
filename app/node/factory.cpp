/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team
  Modifications Copyright (C) 2025 mikesolar

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

#include "factory.h"

#include <QCoreApplication>
#include <QHash>

#include "audio/pan/pan.h"
#include "audio/volume/volume.h"
#include "block/clip/clip.h"
#include "block/gap/gap.h"
#include "block/subtitle/subtitle.h"
#include "block/transition/crossdissolve/crossdissolvetransition.h"
#include "block/transition/diptocolor/diptocolortransition.h"
#include "color/displaytransform/displaytransform.h"
#include "color/ociogradingtransformlinear/ociogradingtransformlinear.h"
#include "color/ociolut/ociolut.h"
#include "color/threewaycolor/threewaycolor.h"
#include "common/Current.h"
#include "distort/cornerpin/cornerpindistortnode.h"
#include "distort/crop/cropdistortnode.h"
#include "distort/flip/flipdistortnode.h"
#include "distort/mask/mask.h"
#include "distort/ripple/rippledistortnode.h"
#include "distort/swirl/swirldistortnode.h"
#include "distort/tile/tiledistortnode.h"
#include "distort/transform/transformdistortnode.h"
#include "distort/wave/wavedistortnode.h"
#include "effect/opacity/opacityeffect.h"
#include "filter/blur/blur.h"
#include "filter/dropshadow/dropshadowfilter.h"
#include "filter/mosaic/mosaicfilternode.h"
#include "filter/stroke/stroke.h"
#include "generator/matrix/matrix.h"
#include "generator/noise/noise.h"
#include "generator/polygon/polygon.h"
#include "generator/shape/shapenode.h"
#include "generator/solid/solid.h"
#include "generator/text/textv1.h"
#include "generator/text/textv2.h"
#include "generator/text/textv3.h"
#include "input/multicam/multicamnode.h"
#include "input/time/timeinput.h"
#include "input/value/valuenode.h"
#include "keying/chromakey/chromakey.h"
#include "keying/colordifferencekey/colordifferencekey.h"
#include "keying/despill/despill.h"
#include "math/math/math.h"
#include "math/merge/merge.h"
#include "math/trigonometry/trigonometry.h"
#include "output/track/track.h"
#include "output/viewer/viewer.h"
#include "pluginSupport/OliveHost.h"
#include "plugins/Plugin.h"
#include "project/folder/folder.h"
#include "project/footage/footage.h"
#include "project/sequence/sequence.h"
#include "time/timeformat/timeformat.h"
#include "time/timeoffset/timeoffsetnode.h"
#include "time/timeremap/timeremap.h"

namespace olive
{

QList<Node *> NodeFactory::library_;

void NodeFactory::Initialize()
{
	Destroy();

	// Add internal types
	for (int i = 0; i < kInternalNodeCount; i++) {
		Node *created_node = CreateFromFactoryIndex(static_cast<InternalID>(i));

		library_.append(created_node);
	}

	RegisterPluginNodes();
}

void NodeFactory::Destroy()
{
	qDeleteAll(library_);
	library_.clear();
}

Menu *NodeFactory::CreateMenu(QWidget *parent, bool create_none_item,
							  Node::CategoryID restrict_to,
							  uint64_t restrict_flags)
{
	Menu *menu = new Menu(parent);
	menu->setToolTipsVisible(true);

	for (int i = 0; i < library_.size(); i++) {
		Node *n = library_.at(i);

		if (restrict_to != Node::kCategoryUnknown &&
			!n->Category().contains(restrict_to)) {
			// Skip this node
			continue;
		}

		if (restrict_flags && !(n->GetFlags() & restrict_flags)) {
			continue;
		}

		if (n->GetFlags() & Node::kDontShowInCreateMenu) {
			continue;
		}

		// Make sure nodes are up-to-date with the current translation
		n->Retranslate();

		QString category_name = Node::GetCategoryName(
			n->Category().isEmpty() ? Node::kCategoryUnknown :
									  n->Category().first());

		// Find or create top-level category menu
		Menu *top_menu = nullptr;
		QList<QAction *> menu_actions = menu->actions();
		foreach (QAction *action, menu_actions) {
			if (action->menu() && action->menu()->title() == category_name) {
				top_menu = static_cast<Menu *>(action->menu());
				break;
			}
		}
		if (!top_menu) {
			top_menu = new Menu(category_name, menu);
			menu->InsertAlphabetically(top_menu);
		}

		// Determine final destination (support secondary grouping)
		Menu *destination = top_menu;
		QString sub = n->SubCategory();
		if (!sub.isEmpty() && n->Category().contains(Node::kCategoryOpenFX)) {
			QList<QAction *> sub_actions = top_menu->actions();
			foreach (QAction *action, sub_actions) {
				if (action->menu() && action->menu()->title() == sub) {
					destination = static_cast<Menu *>(action->menu());
					break;
				}
			}
			if (destination == top_menu) {
				destination = new Menu(sub, top_menu);
				top_menu->InsertAlphabetically(destination);
			}
		}

		// Add entry to menu
		QAction *a = destination->InsertAlphabetically(n->Name());
		a->setData(i);
		a->setToolTip(n->Description());
	}

	if (create_none_item) {
		QAction *none_item = new QAction(
			QCoreApplication::translate("NodeFactory", "None"), menu);

		none_item->setData(-1);

		if (menu->actions().isEmpty()) {
			menu->addAction(none_item);
		} else {
			QAction *separator = menu->insertSeparator(menu->actions().first());
			menu->insertAction(separator, none_item);
		}
	}

	return menu;
}

Node *NodeFactory::CreateFromMenuAction(QAction *action)
{
	int index = action->data().toInt();

	if (index == -1) {
		return nullptr;
	}

	return library_.at(index)->copy();
}

QString NodeFactory::GetIDFromMenuAction(QAction *action)
{
	int index = action->data().toInt();

	if (index == -1) {
		return QString();
	}

	return library_.at(action->data().toInt())->id();
}

QString NodeFactory::GetNameFromID(const QString &id)
{
	if (!id.isEmpty()) {
		foreach (Node *n, library_) {
			if (n->id() == id) {
				return n->Name();
			}
		}
	}

	return QString();
}

Node *NodeFactory::CreateFromID(const QString &id)
{
	QString resolved_id = id;

	// Node IDs renamed after older project files were written
	static const QHash<QString, QString> kLegacyIDs = {
		{ QStringLiteral("org.oliveeditor.Olive.flip"),
		  QStringLiteral("org.olivevideoeditor.Olive.flip") },
		{ QStringLiteral("org.oliveeditor.Olive.ripple"),
		  QStringLiteral("org.olivevideoeditor.Olive.ripple") },
		{ QStringLiteral("org.oliveeditor.Olive.swirl"),
		  QStringLiteral("org.olivevideoeditor.Olive.swirl") },
		{ QStringLiteral("org.oliveeditor.Olive.tile"),
		  QStringLiteral("org.olivevideoeditor.Olive.tile") },
		{ QStringLiteral("org.oliveeditor.Olive.wave"),
		  QStringLiteral("org.olivevideoeditor.Olive.wave") },
	};
	resolved_id = kLegacyIDs.value(id, id);

	foreach (Node *n, library_) {
		if (n->id() == resolved_id) {
			return n->copy();
		}
	}

	return nullptr;
}

void NodeFactory::RegisterPluginNodes()
{
	QSet<QString> existing_ids;
	for (Node *node : library_) {
		existing_ids.insert(node->id());
	}

	for (auto plugin : OFX::Host::PluginCache::getPluginCache()->getPlugins()) {
		auto *image_effect =
			dynamic_cast<OFX::Host::ImageEffect::ImageEffectPlugin *>(plugin);
		if (!image_effect) {
			continue;
		}

		const QString plugin_id =
			QString::fromStdString(image_effect->getIdentifier());
		if (existing_ids.contains(plugin_id)) {
			continue;
		}

		const auto &contexts = image_effect->getContexts();
		if (contexts.empty()) {
			qWarning() << "Skipping OFX plugin with no contexts:" << plugin_id;
			continue;
		}
		std::string context = kOfxImageEffectContextFilter;
		if (contexts.find(kOfxImageEffectContextFilter) == contexts.end()) {
			context = *contexts.begin();
		}

		auto *instance = image_effect->createInstance(context, nullptr);
		if (!instance) {
			continue;
		}

		plugin::PluginNode *plugin_node = new plugin::PluginNode(instance);
		library_.append(plugin_node);
		existing_ids.insert(plugin_id);
	}
}

Node *NodeFactory::CreateFromFactoryIndex(const NodeFactory::InternalID &id)
{
	switch (id) {
	case kClipBlock:
		return new ClipBlock();
	case kGapBlock:
		return new GapBlock();
	case kPolygonGenerator:
		return new PolygonGenerator();
	case kMatrixGenerator:
		return new MatrixGenerator();
	case kTransformDistort:
		return new TransformDistortNode();
	case kTrackOutput:
		return new Track();
	case kViewerOutput:
		return new ViewerOutput();
	case kAudioVolume:
		return new VolumeNode();
	case kAudioPanning:
		return new PanNode();
	case kMath:
		return new MathNode();
	case kTrigonometry:
		return new TrigonometryNode();
	case kTime:
		return new TimeInput();
	case kBlurFilter:
		return new BlurFilterNode();
	case kSolidGenerator:
		return new SolidGenerator();
	case kMerge:
		return new MergeNode();
	case kStrokeFilter:
		return new StrokeFilterNode();
	case kTextGeneratorV1:
		return new TextGeneratorV1();
	case kTextGeneratorV2:
		return new TextGeneratorV2();
	case kTextGeneratorV3:
		return new TextGeneratorV3();
	case kCrossDissolveTransition:
		return new CrossDissolveTransition();
	case kDipToColorTransition:
		return new DipToColorTransition();
	case kMosaicFilter:
		return new MosaicFilterNode();
	case kCropDistort:
		return new CropDistortNode();
	case kProjectFootage:
		return new Footage();
	case kProjectFolder:
		return new Folder();
	case kProjectSequence:
		return new Sequence();
	case kValueNode:
		return new ValueNode();
	case kTimeRemapNode:
		return new TimeRemapNode();
	case kSubtitleBlock:
		return new SubtitleBlock();
	case kShapeGenerator:
		return new ShapeNode();
	case kColorDifferenceKeyKeying:
		return new ColorDifferenceKeyNode();
	case kDespillKeying:
		return new DespillNode();
	case kGroupNode:
		return new NodeGroup();
	case kOpacityEffect:
		return new OpacityEffect();
	case kFlipDistort:
		return new FlipDistortNode();
	case kNoiseGenerator:
		return new NoiseGeneratorNode();
	case kTimeOffsetNode:
		return new TimeOffsetNode();
	case kCornerPinDistort:
		return new CornerPinDistortNode();
	case kDisplayTransform:
		return new DisplayTransformNode();
	case kOCIOGradingTransformLinear:
		return new OCIOGradingTransformLinearNode();
	case kOCIOLut:
		return new OCIOLutNode();
	case kThreeWayColor:
		return new ThreeWayColorNode();
	case kChromaKey:
		return new ChromaKeyNode();
	case kMaskDistort:
		return new MaskDistortNode();
	case kDropShadowFilter:
		return new DropShadowFilter();
	case kTimeFormat:
		return new TimeFormatNode();
	case kWaveDistort:
		return new WaveDistortNode();
	case kTileDistort:
		return new TileDistortNode();
	case kSwirlDistort:
		return new SwirlDistortNode();
	case kRippleDistort:
		return new RippleDistortNode();
	case kMulticamNode:
		return new MultiCamNode();

	case kInternalNodeCount:
		break;
	}

	return nullptr;
}

}
