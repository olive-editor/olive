#include <gtest/gtest.h>

#include <cmath>

#include <QStringList>

#include "node/block/clip/clip.h"
#include "node/block/subtitle/subtitle.h"
#include "node/block/transition/crossdissolve/crossdissolvetransition.h"
#include "node/block/transition/diptocolor/diptocolortransition.h"
#include "node/color/colormanager/colormanager.h"
#include "node/globals.h"
#include "node/math/merge/merge.h"
#include "node/math/trigonometry/trigonometry.h"
#include "node/project.h"
#include "node/traverser.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/samplebuffer.h"
#include "olive/core/util/color.h"
#include "olive/core/util/rational.h"
#include "olive/core/util/timerange.h"
#include "render/job/shaderjob.h"
#include "render/loopmode.h"
#include "render/texture.h"

namespace
{

constexpr double kPi = 3.14159265358979323846;

// A "dummy" texture has no renderer backend and is therefore safe to pass
// around in a headless, CPU-only test.
olive::TexturePtr
MakeDummyTexture(int channels = olive::VideoParams::kRGBAChannelCount)
{
	return std::make_shared<olive::Texture>(
		olive::VideoParams(64, 64, olive::core::PixelFormat::F32, channels));
}

olive::core::AudioParams TestAudioParams()
{
	return olive::core::AudioParams(48000, olive::core::kChannelLayoutStereo,
									olive::core::SampleFormat::F32P);
}

// Stereo buffer with every sample in both channels set to the same value
olive::core::SampleBuffer MakeConstantBuffer(float value, size_t sample_count)
{
	olive::core::SampleBuffer buffer(TestAudioParams(), sample_count);
	for (size_t i = 0; i < sample_count; i++) {
		buffer.data(0)[i] = value;
		buffer.data(1)[i] = value;
	}
	return buffer;
}

olive::NodeValue SampleValue(const olive::core::SampleBuffer &buffer)
{
	return olive::NodeValue(olive::NodeValue::kSamples,
							QVariant::fromValue(buffer));
}

// Globals for the audio path of TransitionBlock::Value, mixing over [in, out)
olive::NodeGlobals AudioGlobals(const olive::core::rational &in,
								const olive::core::rational &out)
{
	return olive::NodeGlobals(olive::VideoParams(), TestAudioParams(),
							  olive::TimeRange(in, out),
							  olive::LoopMode::kLoopModeOff);
}

// A fresh traverser per call: NodeTraverser caches tables per node/range, so
// reusing one would return stale results after changing standard values.
double GenerateTrigResult(olive::TrigonometryNode *node)
{
	olive::NodeTraverser traverser;
	olive::NodeValueTable table = traverser.GenerateTable(
		node, olive::TimeRange(olive::core::rational(0),
							   olive::core::rational(1, 30)));
	return table.Get(olive::NodeValue::kFloat).toDouble();
}

template <typename T> T *AddNode(olive::Project *project)
{
	T *node = new T();
	node->setParent(project);
	return node;
}

// Records the video/audio params seen in NodeGlobals so the traverser's
// NodeGlobals construction can be observed.
class GlobalsProbeNode : public olive::Node {
public:
	GlobalsProbeNode() = default;

	NODE_DEFAULT_FUNCTIONS(GlobalsProbeNode)

	virtual QString Name() const override
	{
		return QStringLiteral("Globals Probe");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.globalsprobe");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return {};
	}

	virtual void Value(const olive::NodeValueRow &value,
					   const olive::NodeGlobals &globals,
					   olive::NodeValueTable *table) const override
	{
		Q_UNUSED(value)

		last_vparams_ = globals.vparams();
		last_aparams_ = globals.aparams();
		table->Push(olive::NodeValue::kFloat, 0.0, this);
	}

	const olive::VideoParams &last_vparams() const
	{
		return last_vparams_;
	}

	const olive::core::AudioParams &last_aparams() const
	{
		return last_aparams_;
	}

private:
	mutable olive::VideoParams last_vparams_;
	mutable olive::core::AudioParams last_aparams_;
};

} // namespace

// -----------------------------------------------------------------------------
// TrigonometryNode
// -----------------------------------------------------------------------------

TEST(TrigonometryNode, MetadataIsCorrect)
{
	olive::TrigonometryNode node;

	EXPECT_EQ(node.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.trigonometry"));
	EXPECT_EQ(node.Name(), QStringLiteral("Trigonometry"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryMath));

	EXPECT_EQ(int(node.GetInputDataType(olive::TrigonometryNode::kMethodIn)),
			  int(olive::NodeValue::kCombo));
	EXPECT_EQ(int(node.GetInputDataType(olive::TrigonometryNode::kXIn)),
			  int(olive::NodeValue::kFloat));

	// Method defaults to sine, value defaults to zero
	EXPECT_EQ(node.GetStandardValue(olive::TrigonometryNode::kMethodIn).toInt(),
			  0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::TrigonometryNode::kXIn).toDouble(), 0.0);

	// The method is a static UI choice: neither connectable nor keyframable
	EXPECT_FALSE(
		node.IsInputConnectable(olive::TrigonometryNode::kMethodIn));
	EXPECT_FALSE(
		node.IsInputKeyframable(olive::TrigonometryNode::kMethodIn));
}

TEST(TrigonometryNode, RetranslateSetsInputNamesAndComboStrings)
{
	olive::TrigonometryNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::TrigonometryNode::kMethodIn),
			  QStringLiteral("Method"));
	EXPECT_EQ(node.GetInputName(olive::TrigonometryNode::kXIn),
			  QStringLiteral("Value"));

	const QStringList methods =
		node.GetInputProperty(olive::TrigonometryNode::kMethodIn,
							  QStringLiteral("combo_str"))
			.toStringList();
	ASSERT_EQ(methods.size(), 9);
	EXPECT_EQ(methods.at(0), QStringLiteral("Sine"));
	EXPECT_EQ(methods.at(1), QStringLiteral("Cosine"));
	EXPECT_EQ(methods.at(2), QStringLiteral("Tangent"));
	EXPECT_EQ(methods.at(3), QStringLiteral("Inverse Sine"));
	EXPECT_EQ(methods.at(4), QStringLiteral("Inverse Cosine"));
	EXPECT_EQ(methods.at(5), QStringLiteral("Inverse Tangent"));
	EXPECT_EQ(methods.at(6), QStringLiteral("Hyperbolic Sine"));
	EXPECT_EQ(methods.at(7), QStringLiteral("Hyperbolic Cosine"));
	EXPECT_EQ(methods.at(8), QStringLiteral("Hyperbolic Tangent"));

	// The combo list contains no separator entries, so combo indexes match
	// the Operation enum used by Value() exactly
}

TEST(TrigonometryNode, SineCosineTangent)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TrigonometryNode>(&project);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 0);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, kPi / 2);
	EXPECT_NEAR(GenerateTrigResult(node), 1.0, 1e-12);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 1);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, kPi);
	EXPECT_NEAR(GenerateTrigResult(node), -1.0, 1e-12);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 2);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, kPi / 4);
	EXPECT_NEAR(GenerateTrigResult(node), 1.0, 1e-12);
}

TEST(TrigonometryNode, InverseOperations)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TrigonometryNode>(&project);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 3);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, 1.0);
	EXPECT_NEAR(GenerateTrigResult(node), kPi / 2, 1e-12);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 4);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, -1.0);
	EXPECT_NEAR(GenerateTrigResult(node), kPi, 1e-12);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 5);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, 1.0);
	EXPECT_NEAR(GenerateTrigResult(node), kPi / 4, 1e-12);
}

TEST(TrigonometryNode, HyperbolicOperations)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TrigonometryNode>(&project);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, 1.0);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 6);
	EXPECT_NEAR(GenerateTrigResult(node), std::sinh(1.0), 1e-12);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 7);
	EXPECT_NEAR(GenerateTrigResult(node), std::cosh(1.0), 1e-12);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 8);
	EXPECT_NEAR(GenerateTrigResult(node), std::tanh(1.0), 1e-12);
}

TEST(TrigonometryNode, ComboIndexMatchesOperationEnum)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TrigonometryNode>(&project);

	// The combo list has no separator entries, so a combo index selects the
	// Operation enum value with the same index
	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 3);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, 0.5);
	EXPECT_NEAR(GenerateTrigResult(node), std::asin(0.5), 1e-12);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 4);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, 0.5);
	EXPECT_NEAR(GenerateTrigResult(node), std::acos(0.5), 1e-12);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 6);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, 1.0);
	EXPECT_NEAR(GenerateTrigResult(node), std::sinh(1.0), 1e-12);

	node->SetStandardValue(olive::TrigonometryNode::kMethodIn, 8);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, 1.0);
	EXPECT_NEAR(GenerateTrigResult(node), std::tanh(1.0), 1e-12);
}

// -----------------------------------------------------------------------------
// MergeNode
// -----------------------------------------------------------------------------

TEST(MergeNode, MetadataIsCorrect)
{
	olive::MergeNode node;

	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.merge"));
	EXPECT_EQ(node.Name(), QStringLiteral("Merge"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryMath));

	EXPECT_EQ(int(node.GetInputDataType(olive::MergeNode::kBaseIn)),
			  int(olive::NodeValue::kTexture));
	EXPECT_EQ(int(node.GetInputDataType(olive::MergeNode::kBlendIn)),
			  int(olive::NodeValue::kTexture));

	// Textures cannot be keyframed, and the merge node is an internal
	// compositing building block hidden from the param view
	EXPECT_FALSE(node.IsInputKeyframable(olive::MergeNode::kBaseIn));
	EXPECT_FALSE(node.IsInputKeyframable(olive::MergeNode::kBlendIn));
	EXPECT_TRUE(node.GetFlags() & olive::Node::kDontShowInParamView);
}

TEST(MergeNode, RetranslateSetsInputNames)
{
	olive::MergeNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::MergeNode::kBaseIn),
			  QStringLiteral("Base"));
	EXPECT_EQ(node.GetInputName(olive::MergeNode::kBlendIn),
			  QStringLiteral("Blend"));
}

TEST(MergeNode, ShaderCodeLoadsAlphaOver)
{
	olive::MergeNode node;

	const olive::ShaderCode code =
		node.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("x")));
	EXPECT_FALSE(code.frag_code().isEmpty());
}

TEST(MergeNode, ValueWithNoTexturesPushesNothing)
{
	olive::MergeNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

TEST(MergeNode, ValueWithOnlyBasePassesBaseThrough)
{
	olive::MergeNode node;

	olive::TexturePtr base = MakeDummyTexture();
	olive::NodeValueRow row;
	row.insert(olive::MergeNode::kBaseIn,
			   olive::NodeValue(olive::NodeValue::kTexture, base));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Get(olive::NodeValue::kTexture).toTexture(), base);
}

TEST(MergeNode, ValueWithOnlyBlendPassesBlendThrough)
{
	olive::MergeNode node;

	olive::TexturePtr blend = MakeDummyTexture();
	olive::NodeValueRow row;
	row.insert(olive::MergeNode::kBlendIn,
			   olive::NodeValue(olive::NodeValue::kTexture, blend));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Get(olive::NodeValue::kTexture).toTexture(), blend);
}

TEST(MergeNode, ValueWithRgbBlendPassesBlendThrough)
{
	olive::MergeNode node;

	// An RGB blend texture has no alpha channel, so alpha-over is skipped
	// and the blend passes through even when a base is present
	olive::TexturePtr base = MakeDummyTexture();
	olive::TexturePtr blend = MakeDummyTexture(3);
	olive::NodeValueRow row;
	row.insert(olive::MergeNode::kBaseIn,
			   olive::NodeValue(olive::NodeValue::kTexture, base));
	row.insert(olive::MergeNode::kBlendIn,
			   olive::NodeValue(olive::NodeValue::kTexture, blend));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Get(olive::NodeValue::kTexture).toTexture(), blend);
}

TEST(MergeNode, ValueWithBaseAndRgbaBlendPushesAlphaOverJob)
{
	olive::MergeNode node;

	olive::TexturePtr base = MakeDummyTexture();
	olive::TexturePtr blend = MakeDummyTexture();
	olive::NodeValueRow row;
	row.insert(olive::MergeNode::kBaseIn,
			   olive::NodeValue(olive::NodeValue::kTexture, base));
	row.insert(olive::MergeNode::kBlendIn,
			   olive::NodeValue(olive::NodeValue::kTexture, blend));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	olive::TexturePtr out = table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(out);
	ASSERT_TRUE(out->IsJob());

	auto *job = dynamic_cast<olive::ShaderJob *>(out->job());
	ASSERT_TRUE(job);
	EXPECT_EQ(job->Get(olive::MergeNode::kBaseIn).toTexture(), base);
	EXPECT_EQ(job->Get(olive::MergeNode::kBlendIn).toTexture(), blend);

	// The job texture inherits the base texture's parameters
	EXPECT_EQ(out->params().width(), base->params().width());
	EXPECT_EQ(out->params().height(), base->params().height());
}

// -----------------------------------------------------------------------------
// TransitionBlock (through the concrete CrossDissolveTransition and
// DipToColorTransition subclasses)
// -----------------------------------------------------------------------------

TEST(TransitionBlock, MetadataIsCorrect)
{
	olive::CrossDissolveTransition cross;
	EXPECT_EQ(cross.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.crossdissolve"));
	EXPECT_EQ(cross.Name(), QStringLiteral("Cross Dissolve"));
	EXPECT_FALSE(cross.Description().isEmpty());
	EXPECT_TRUE(cross.Category().contains(olive::Node::kCategoryTransition));

	olive::DipToColorTransition dip;
	EXPECT_EQ(dip.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.diptocolor"));
	EXPECT_EQ(dip.Name(), QStringLiteral("Dip To Color"));
	EXPECT_FALSE(dip.Description().isEmpty());
	EXPECT_TRUE(dip.Category().contains(olive::Node::kCategoryTransition));

	EXPECT_EQ(int(cross.GetInputDataType(
				  olive::TransitionBlock::kOutBlockInput)),
			  int(olive::NodeValue::kNone));
	EXPECT_EQ(int(cross.GetInputDataType(
				  olive::TransitionBlock::kInBlockInput)),
			  int(olive::NodeValue::kNone));
	EXPECT_EQ(
		int(cross.GetInputDataType(olive::TransitionBlock::kCurveInput)),
		int(olive::NodeValue::kCombo));
	EXPECT_EQ(
		int(cross.GetInputDataType(olive::TransitionBlock::kCenterInput)),
		int(olive::NodeValue::kRational));

	// The curve is a static UI choice defaulting to linear
	EXPECT_FALSE(
		cross.IsInputConnectable(olive::TransitionBlock::kCurveInput));
	EXPECT_FALSE(
		cross.IsInputKeyframable(olive::TransitionBlock::kCurveInput));
	EXPECT_EQ(cross.GetStandardValue(olive::TransitionBlock::kCurveInput)
				  .toInt(),
			  0);
	EXPECT_EQ(cross.offset_center(), olive::core::rational(0));

	// Blocks hide from the param view by default, transitions re-enable it
	EXPECT_FALSE(cross.GetFlags() & olive::Node::kDontShowInParamView);

	// Dip To Color adds a color parameter defaulting to black
	const olive::core::Color color =
		dip.GetStandardValue(olive::DipToColorTransition::kColorInput)
			.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(color.red(), 0.0f);
	EXPECT_FLOAT_EQ(color.green(), 0.0f);
	EXPECT_FLOAT_EQ(color.blue(), 0.0f);
}

TEST(TransitionBlock, RetranslateSetsInputNamesAndCurveStrings)
{
	olive::CrossDissolveTransition cross;
	cross.Retranslate();

	EXPECT_EQ(cross.GetInputName(olive::TransitionBlock::kOutBlockInput),
			  QStringLiteral("From"));
	EXPECT_EQ(cross.GetInputName(olive::TransitionBlock::kInBlockInput),
			  QStringLiteral("To"));
	EXPECT_EQ(cross.GetInputName(olive::TransitionBlock::kCurveInput),
			  QStringLiteral("Curve"));
	EXPECT_EQ(cross.GetInputName(olive::TransitionBlock::kCenterInput),
			  QStringLiteral("Center Offset"));

	const QStringList curves =
		cross.GetInputProperty(olive::TransitionBlock::kCurveInput,
							   QStringLiteral("combo_str"))
			.toStringList();
	ASSERT_EQ(curves.size(), 3);
	EXPECT_EQ(curves.at(0), QStringLiteral("Linear"));
	EXPECT_EQ(curves.at(1), QStringLiteral("Exponential"));
	EXPECT_EQ(curves.at(2), QStringLiteral("Logarithmic"));

	olive::DipToColorTransition dip;
	dip.Retranslate();
	EXPECT_EQ(dip.GetInputName(olive::DipToColorTransition::kColorInput),
			  QStringLiteral("Color"));
}

TEST(TransitionBlock, ShaderCodeLoadsTransitionShaders)
{
	olive::CrossDissolveTransition cross;
	EXPECT_FALSE(
		cross.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("x")))
			.frag_code()
			.isEmpty());

	olive::DipToColorTransition dip;
	EXPECT_FALSE(
		dip.GetShaderCode(olive::Node::ShaderRequest(QStringLiteral("x")))
			.frag_code()
			.isEmpty());
}

TEST(TransitionBlock, OffsetsWithoutConnectedBlocksAreZero)
{
	olive::CrossDissolveTransition trans;
	trans.set_length_and_media_out(olive::core::rational(2));

	EXPECT_FALSE(trans.is_dual_transition());
	EXPECT_EQ(trans.connected_out_block(), nullptr);
	EXPECT_EQ(trans.connected_in_block(), nullptr);
	EXPECT_EQ(trans.in_offset(), olive::core::rational(0));
	EXPECT_EQ(trans.out_offset(), olive::core::rational(0));

	// With zero offsets only the total progress is meaningful
	EXPECT_DOUBLE_EQ(trans.GetTotalProgress(0.5), 0.25);
	EXPECT_DOUBLE_EQ(trans.GetOutProgress(0.5), 0.0);
	EXPECT_DOUBLE_EQ(trans.GetInProgress(0.5), 0.0);
}

TEST(TransitionBlock, DualTransitionOffsetsAndProgress)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *trans = AddNode<olive::CrossDissolveTransition>(&project);
	auto *out_clip = AddNode<olive::ClipBlock>(&project);
	auto *in_clip = AddNode<olive::ClipBlock>(&project);

	trans->set_length_and_media_out(olive::core::rational(2));

	olive::Node::ConnectEdge(
		out_clip,
		olive::NodeInput(trans, olive::TransitionBlock::kOutBlockInput));
	olive::Node::ConnectEdge(
		in_clip, olive::NodeInput(trans, olive::TransitionBlock::kInBlockInput));

	ASSERT_TRUE(trans->is_dual_transition());
	EXPECT_EQ(trans->connected_out_block(), out_clip);
	EXPECT_EQ(trans->connected_in_block(), in_clip);

	// A centered dual transition splits its length evenly on both sides
	EXPECT_EQ(trans->in_offset(), olive::core::rational(1));
	EXPECT_EQ(trans->out_offset(), olive::core::rational(1));

	EXPECT_DOUBLE_EQ(trans->GetTotalProgress(0.0), 0.0);
	EXPECT_DOUBLE_EQ(trans->GetTotalProgress(0.5), 0.25);
	EXPECT_DOUBLE_EQ(trans->GetTotalProgress(1.5), 0.75);

	// Out progress runs from 1 to 0 over the out offset
	EXPECT_DOUBLE_EQ(trans->GetOutProgress(0.0), 1.0);
	EXPECT_DOUBLE_EQ(trans->GetOutProgress(0.5), 0.5);
	EXPECT_DOUBLE_EQ(trans->GetOutProgress(1.5), 0.0);

	// In progress runs from 0 to 1 over the in offset and is clamped
	EXPECT_DOUBLE_EQ(trans->GetInProgress(0.5), 0.0);
	EXPECT_DOUBLE_EQ(trans->GetInProgress(1.5), 0.5);
	EXPECT_DOUBLE_EQ(trans->GetInProgress(2.5), 1.0);
}

TEST(TransitionBlock, OffsetCenterShiftsInOutOffsets)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *trans = AddNode<olive::CrossDissolveTransition>(&project);
	auto *out_clip = AddNode<olive::ClipBlock>(&project);
	auto *in_clip = AddNode<olive::ClipBlock>(&project);

	trans->set_length_and_media_out(olive::core::rational(2));
	olive::Node::ConnectEdge(
		out_clip,
		olive::NodeInput(trans, olive::TransitionBlock::kOutBlockInput));
	olive::Node::ConnectEdge(
		in_clip, olive::NodeInput(trans, olive::TransitionBlock::kInBlockInput));

	trans->set_offset_center(olive::core::rational(1, 2));
	EXPECT_EQ(trans->offset_center(), olive::core::rational(1, 2));

	// A positive center offset moves the midpoint towards the out clip
	EXPECT_EQ(trans->in_offset(), olive::core::rational(3, 2));
	EXPECT_EQ(trans->out_offset(), olive::core::rational(1, 2));

	EXPECT_DOUBLE_EQ(trans->GetOutProgress(0.25), 0.5);
	EXPECT_DOUBLE_EQ(trans->GetInProgress(1.25), 0.5);
}

TEST(TransitionBlock, SingleSidedTransitionOffsets)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	// Only an outgoing clip: the whole length is the out offset (fade out)
	auto *out_trans = AddNode<olive::CrossDissolveTransition>(&project);
	auto *out_clip = AddNode<olive::ClipBlock>(&project);
	out_trans->set_length_and_media_out(olive::core::rational(2));
	olive::Node::ConnectEdge(
		out_clip,
		olive::NodeInput(out_trans, olive::TransitionBlock::kOutBlockInput));

	EXPECT_FALSE(out_trans->is_dual_transition());
	EXPECT_EQ(out_trans->out_offset(), olive::core::rational(2));
	EXPECT_EQ(out_trans->in_offset(), olive::core::rational(0));
	EXPECT_DOUBLE_EQ(out_trans->GetOutProgress(1.0), 0.5);
	EXPECT_DOUBLE_EQ(out_trans->GetInProgress(1.0), 0.0);

	// Only an incoming clip: the whole length is the in offset (fade in)
	auto *in_trans = AddNode<olive::CrossDissolveTransition>(&project);
	auto *in_clip = AddNode<olive::ClipBlock>(&project);
	in_trans->set_length_and_media_out(olive::core::rational(2));
	olive::Node::ConnectEdge(
		in_clip,
		olive::NodeInput(in_trans, olive::TransitionBlock::kInBlockInput));

	EXPECT_FALSE(in_trans->is_dual_transition());
	EXPECT_EQ(in_trans->in_offset(), olive::core::rational(2));
	EXPECT_EQ(in_trans->out_offset(), olive::core::rational(0));
	EXPECT_DOUBLE_EQ(in_trans->GetInProgress(1.0), 0.5);
	EXPECT_DOUBLE_EQ(in_trans->GetOutProgress(1.0), 0.0);
}

TEST(TransitionBlock, SetOffsetsAndLengthSetsLengthAndCenter)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *trans = AddNode<olive::CrossDissolveTransition>(&project);
	auto *out_clip = AddNode<olive::ClipBlock>(&project);
	auto *in_clip = AddNode<olive::ClipBlock>(&project);
	olive::Node::ConnectEdge(
		out_clip,
		olive::NodeInput(trans, olive::TransitionBlock::kOutBlockInput));
	olive::Node::ConnectEdge(
		in_clip, olive::NodeInput(trans, olive::TransitionBlock::kInBlockInput));

	trans->set_offsets_and_length(olive::core::rational(1, 4),
								  olive::core::rational(3, 4));

	// The length is always the sum of both offsets
	EXPECT_EQ(trans->length(), olive::core::rational(1));
	EXPECT_EQ(trans->offset_center(), olive::core::rational(1, 4));

	// The offset arguments are named from the adjoining clips' perspective
	// (OTIO convention): the in_offset argument describes the overlap with
	// the previous clip, which is the transition's out side
	EXPECT_EQ(trans->out_offset(), olive::core::rational(1, 4));
	EXPECT_EQ(trans->in_offset(), olive::core::rational(3, 4));
}

TEST(TransitionBlock, ConnectAndDisconnectUpdateLinkedClips)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *trans = AddNode<olive::CrossDissolveTransition>(&project);
	auto *clip = AddNode<olive::ClipBlock>(&project);

	olive::Node::ConnectEdge(
		clip, olive::NodeInput(trans, olive::TransitionBlock::kOutBlockInput));
	EXPECT_EQ(trans->connected_out_block(), clip);
	EXPECT_EQ(clip->out_transition(), trans);

	olive::Node::DisconnectEdge(
		clip, olive::NodeInput(trans, olive::TransitionBlock::kOutBlockInput));
	EXPECT_EQ(trans->connected_out_block(), nullptr);
	EXPECT_EQ(clip->out_transition(), nullptr);

	// Connecting a node that is not a clip leaves the linked block null
	auto *not_a_clip = AddNode<GlobalsProbeNode>(&project);
	olive::Node::ConnectEdge(
		not_a_clip,
		olive::NodeInput(trans, olive::TransitionBlock::kOutBlockInput));
	EXPECT_EQ(trans->connected_out_block(), nullptr);
	olive::Node::DisconnectEdge(
		not_a_clip,
		olive::NodeInput(trans, olive::TransitionBlock::kOutBlockInput));
}

TEST(TransitionBlock, TextureValuePushesJobWithTransitionProgress)
{
	olive::CrossDissolveTransition trans;
	trans.set_length_and_media_out(olive::core::rational(2));

	olive::TexturePtr out_tex = MakeDummyTexture();
	olive::TexturePtr in_tex = MakeDummyTexture();

	olive::NodeValueRow row;
	row.insert(olive::TransitionBlock::kOutBlockInput,
			   olive::NodeValue(olive::NodeValue::kTexture, out_tex));
	row.insert(olive::TransitionBlock::kInBlockInput,
			   olive::NodeValue(olive::NodeValue::kTexture, in_tex));
	row.insert(olive::TransitionBlock::kCurveInput,
			   olive::NodeValue(olive::NodeValue::kCombo, 0));

	// Half-way through a two second transition
	const olive::NodeGlobals globals(
		olive::VideoParams(64, 64, olive::core::PixelFormat::F32,
						   olive::VideoParams::kRGBAChannelCount),
		olive::core::AudioParams(),
		olive::TimeRange(olive::core::rational(1), olive::core::rational(2)),
		olive::LoopMode::kLoopModeOff);

	olive::NodeValueTable table;
	trans.Value(row, globals, &table);

	olive::TexturePtr job_tex =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(job_tex);
	ASSERT_TRUE(job_tex->IsJob());
	EXPECT_EQ(job_tex->params().width(), 64);

	auto *job = dynamic_cast<olive::ShaderJob *>(job_tex->job());
	ASSERT_TRUE(job);
	EXPECT_EQ(job->Get(olive::TransitionBlock::kOutBlockInput).toTexture(),
			  out_tex);
	EXPECT_EQ(job->Get(olive::TransitionBlock::kInBlockInput).toTexture(),
			  in_tex);
	EXPECT_EQ(job->Get(olive::TransitionBlock::kCurveInput).toInt(), 0);

	// Without connected clips the in/out offsets are zero, so only the total
	// transition progress is meaningful
	EXPECT_DOUBLE_EQ(job->Get(QStringLiteral("ove_tprog_all")).toDouble(),
					 0.5);
	EXPECT_DOUBLE_EQ(job->Get(QStringLiteral("ove_tprog_out")).toDouble(),
					 0.0);
	EXPECT_DOUBLE_EQ(job->Get(QStringLiteral("ove_tprog_in")).toDouble(),
					 0.0);
}

TEST(TransitionBlock, TextureValueInsertsNullTextureForMissingSide)
{
	olive::CrossDissolveTransition trans;
	trans.set_length_and_media_out(olive::core::rational(2));

	olive::TexturePtr in_tex = MakeDummyTexture();

	olive::NodeValueRow row;
	row.insert(olive::TransitionBlock::kInBlockInput,
			   olive::NodeValue(olive::NodeValue::kTexture, in_tex));
	row.insert(olive::TransitionBlock::kCurveInput,
			   olive::NodeValue(olive::NodeValue::kCombo, 0));

	olive::NodeValueTable table;
	trans.Value(row, olive::NodeGlobals(), &table);

	olive::TexturePtr job_tex =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(job_tex);
	ASSERT_TRUE(job_tex->IsJob());

	auto *job = dynamic_cast<olive::ShaderJob *>(job_tex->job());
	ASSERT_TRUE(job);
	EXPECT_EQ(job->Get(olive::TransitionBlock::kInBlockInput).toTexture(),
			  in_tex);

	// The missing "from" side is still inserted, as a null texture
	const olive::NodeValue out_side =
		job->Get(olive::TransitionBlock::kOutBlockInput);
	EXPECT_EQ(out_side.type(), olive::NodeValue::kTexture);
	EXPECT_TRUE(out_side.toTexture() == nullptr);
}

TEST(TransitionBlock, DipToColorValueInsertsColorIntoJob)
{
	olive::DipToColorTransition trans;
	trans.set_length_and_media_out(olive::core::rational(2));

	olive::TexturePtr out_tex = MakeDummyTexture();
	olive::TexturePtr in_tex = MakeDummyTexture();

	olive::NodeValueRow row;
	row.insert(olive::TransitionBlock::kOutBlockInput,
			   olive::NodeValue(olive::NodeValue::kTexture, out_tex));
	row.insert(olive::TransitionBlock::kInBlockInput,
			   olive::NodeValue(olive::NodeValue::kTexture, in_tex));
	row.insert(olive::TransitionBlock::kCurveInput,
			   olive::NodeValue(olive::NodeValue::kCombo, 0));
	row.insert(olive::DipToColorTransition::kColorInput,
			   olive::NodeValue(olive::NodeValue::kColor,
								QVariant::fromValue(olive::core::Color(
									0.25f, 0.5f, 0.75f, 1.0f))));

	olive::NodeValueTable table;
	trans.Value(row, olive::NodeGlobals(), &table);

	olive::TexturePtr job_tex =
		table.Get(olive::NodeValue::kTexture).toTexture();
	ASSERT_TRUE(job_tex);
	ASSERT_TRUE(job_tex->IsJob());

	auto *job = dynamic_cast<olive::ShaderJob *>(job_tex->job());
	ASSERT_TRUE(job);
	const olive::core::Color color =
		job->Get(olive::DipToColorTransition::kColorInput).toColor();
	EXPECT_FLOAT_EQ(color.red(), 0.25f);
	EXPECT_FLOAT_EQ(color.green(), 0.5f);
	EXPECT_FLOAT_EQ(color.blue(), 0.75f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);
}

TEST(TransitionBlock, AudioValueMixesLinearCrossfade)
{
	olive::CrossDissolveTransition trans;
	trans.set_length_and_media_out(olive::core::rational(1));

	// Half a second at 48 kHz
	const size_t sample_count = 24000;
	const olive::core::SampleBuffer from =
		MakeConstantBuffer(1.0f, sample_count);
	const olive::core::SampleBuffer to = MakeConstantBuffer(0.5f, sample_count);

	olive::NodeValueRow row;
	row.insert(olive::TransitionBlock::kOutBlockInput, SampleValue(from));
	row.insert(olive::TransitionBlock::kInBlockInput, SampleValue(to));

	olive::NodeValueTable table;
	trans.Value(row,
				AudioGlobals(olive::core::rational(0),
							 olive::core::rational(1, 2)),
				&table);

	const olive::NodeValue out_val = table.Get(olive::NodeValue::kSamples);
	ASSERT_EQ(out_val.type(), olive::NodeValue::kSamples);
	const olive::core::SampleBuffer out = out_val.toSamples();
	ASSERT_TRUE(out.is_allocated());
	EXPECT_EQ(out.sample_count(), sample_count);

	// Sample 0 is fully "from", sample 12000 is 25% through the transition
	for (int channel = 0; channel < 2; channel++) {
		EXPECT_FLOAT_EQ(out.data(channel)[0], 1.0f);
		EXPECT_FLOAT_EQ(out.data(channel)[12000], 0.875f);
	}
}

TEST(TransitionBlock, AudioValueMixesExponentialCrossfade)
{
	olive::CrossDissolveTransition trans;
	trans.set_length_and_media_out(olive::core::rational(1));
	trans.SetStandardValue(olive::TransitionBlock::kCurveInput, 1);

	const size_t sample_count = 24000;
	const olive::core::SampleBuffer from =
		MakeConstantBuffer(1.0f, sample_count);
	const olive::core::SampleBuffer to = MakeConstantBuffer(0.5f, sample_count);

	olive::NodeValueRow row;
	row.insert(olive::TransitionBlock::kOutBlockInput, SampleValue(from));
	row.insert(olive::TransitionBlock::kInBlockInput, SampleValue(to));

	olive::NodeValueTable table;
	trans.Value(row,
				AudioGlobals(olive::core::rational(0),
							 olive::core::rational(1, 2)),
				&table);

	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());

	// The exponential curve squares the linear progress: at 25% the mix is
	// 1.0 * 0.75^2 + 0.5 * 0.25^2
	EXPECT_FLOAT_EQ(out.data(0)[0], 1.0f);
	EXPECT_FLOAT_EQ(out.data(0)[12000], 0.59375f);
}

TEST(TransitionBlock, AudioValueMixesLogarithmicCrossfade)
{
	olive::CrossDissolveTransition trans;
	trans.set_length_and_media_out(olive::core::rational(1));
	trans.SetStandardValue(olive::TransitionBlock::kCurveInput, 2);

	const size_t sample_count = 24000;
	const olive::core::SampleBuffer from =
		MakeConstantBuffer(1.0f, sample_count);
	const olive::core::SampleBuffer to = MakeConstantBuffer(0.5f, sample_count);

	olive::NodeValueRow row;
	row.insert(olive::TransitionBlock::kOutBlockInput, SampleValue(from));
	row.insert(olive::TransitionBlock::kInBlockInput, SampleValue(to));

	olive::NodeValueTable table;
	trans.Value(row,
				AudioGlobals(olive::core::rational(0),
							 olive::core::rational(1, 2)),
				&table);

	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());

	// The logarithmic curve square-roots the linear progress: at 25% the mix
	// is 1.0 * sqrt(0.75) + 0.5 * sqrt(0.25)
	EXPECT_FLOAT_EQ(out.data(0)[0], 1.0f);
	EXPECT_NEAR(out.data(0)[12000], std::sqrt(0.75) + 0.25, 1e-6);
}

TEST(TransitionBlock, AudioValueWithSingleSideFades)
{
	// Only a "from" buffer: fade out
	olive::CrossDissolveTransition fade_out;
	fade_out.set_length_and_media_out(olive::core::rational(1));

	const size_t sample_count = 24000;
	olive::NodeValueRow out_row;
	out_row.insert(olive::TransitionBlock::kOutBlockInput,
				   SampleValue(MakeConstantBuffer(1.0f, sample_count)));

	olive::NodeValueTable out_table;
	fade_out.Value(out_row,
				   AudioGlobals(olive::core::rational(0),
								olive::core::rational(1, 2)),
				   &out_table);

	const olive::core::SampleBuffer faded_out =
		out_table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(faded_out.is_allocated());
	EXPECT_FLOAT_EQ(faded_out.data(0)[0], 1.0f);
	EXPECT_FLOAT_EQ(faded_out.data(0)[12000], 0.75f);

	// Only a "to" buffer: fade in
	olive::CrossDissolveTransition fade_in;
	fade_in.set_length_and_media_out(olive::core::rational(1));

	olive::NodeValueRow in_row;
	in_row.insert(olive::TransitionBlock::kInBlockInput,
				  SampleValue(MakeConstantBuffer(0.5f, sample_count)));

	olive::NodeValueTable in_table;
	fade_in.Value(in_row,
				  AudioGlobals(olive::core::rational(0),
							   olive::core::rational(1, 2)),
				  &in_table);

	const olive::core::SampleBuffer faded_in =
		in_table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(faded_in.is_allocated());
	EXPECT_FLOAT_EQ(faded_in.data(0)[0], 0.0f);
	EXPECT_FLOAT_EQ(faded_in.data(0)[12000], 0.125f);
}

TEST(TransitionBlock, ValueWithNoInputsPushesNothing)
{
	olive::CrossDissolveTransition trans;
	trans.set_length_and_media_out(olive::core::rational(1));

	olive::NodeValueTable table;
	trans.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_EQ(table.Count(), 0);
}

// -----------------------------------------------------------------------------
// SubtitleBlock
// -----------------------------------------------------------------------------

TEST(SubtitleBlock, MetadataIsCorrect)
{
	olive::SubtitleBlock node;

	EXPECT_EQ(node.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.subtitle"));
	EXPECT_EQ(node.Name(), QStringLiteral("Subtitle"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryTimeline));
}

TEST(SubtitleBlock, NameFollowsText)
{
	olive::SubtitleBlock node;

	EXPECT_TRUE(node.GetText().isEmpty());
	EXPECT_EQ(node.Name(), QStringLiteral("Subtitle"));

	node.SetText(QStringLiteral("Hello World"));
	EXPECT_EQ(node.GetText(), QStringLiteral("Hello World"));
	EXPECT_EQ(node.Name(), QStringLiteral("Hello World"));

	node.SetText(QString());
	EXPECT_EQ(node.Name(), QStringLiteral("Subtitle"));
}

TEST(SubtitleBlock, TextInputFlagsAndHiddenClipInputs)
{
	olive::SubtitleBlock node;

	EXPECT_EQ(int(node.GetInputDataType(olive::SubtitleBlock::kTextIn)),
			  int(olive::NodeValue::kText));

	// The text is edited inline: neither connectable nor keyframable
	EXPECT_FALSE(node.IsInputConnectable(olive::SubtitleBlock::kTextIn));
	EXPECT_FALSE(node.IsInputKeyframable(olive::SubtitleBlock::kTextIn));

	// The inherited clip inputs are meaningless for a subtitle and hidden
	EXPECT_TRUE(node.IsInputHidden(olive::ClipBlock::kBufferIn));
	EXPECT_TRUE(node.IsInputHidden(olive::Block::kLengthInput));
	EXPECT_TRUE(node.IsInputHidden(olive::ClipBlock::kMediaInInput));
	EXPECT_TRUE(node.IsInputHidden(olive::ClipBlock::kSpeedInput));
	EXPECT_TRUE(node.IsInputHidden(olive::ClipBlock::kReverseInput));
	EXPECT_TRUE(node.IsInputHidden(olive::ClipBlock::kMaintainAudioPitchInput));

	// Blocks hide from the param view by default, subtitles re-enable it
	EXPECT_FALSE(node.GetFlags() & olive::Node::kDontShowInParamView);
}

TEST(SubtitleBlock, RetranslateSetsInputName)
{
	olive::SubtitleBlock node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::SubtitleBlock::kTextIn),
			  QStringLiteral("Text"));
}

// -----------------------------------------------------------------------------
// NodeTraverser: NodeGlobals construction and value caching
// -----------------------------------------------------------------------------

TEST(NodeTraverser, GenerateTablePassesCacheParamsToNodeGlobals)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *probe = AddNode<GlobalsProbeNode>(&project);

	olive::NodeTraverser traverser;
	traverser.SetCacheVideoParams(
		olive::VideoParams(1920, 1080, olive::core::PixelFormat::F32,
						   olive::VideoParams::kRGBAChannelCount));
	traverser.SetCacheAudioParams(
		olive::core::AudioParams(44100, olive::core::kChannelLayoutStereo,
								 olive::core::SampleFormat::F32P));

	traverser.GenerateTable(probe,
							olive::TimeRange(olive::core::rational(0),
											 olive::core::rational(1, 30)));

	EXPECT_EQ(probe->last_vparams().width(), 1920);
	EXPECT_EQ(probe->last_vparams().height(), 1080);
	EXPECT_EQ(probe->last_aparams().sample_rate(), 44100);
}

TEST(NodeTraverser, GenerateTableCachesResultsPerNodeAndRange)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TrigonometryNode>(&project);
	node->SetStandardValue(olive::TrigonometryNode::kXIn, 1.0);

	const olive::TimeRange range(olive::core::rational(0),
								 olive::core::rational(1, 30));

	olive::NodeTraverser traverser;
	const double first =
		traverser.GenerateTable(node, range)
			.Get(olive::NodeValue::kFloat)
			.toDouble();
	EXPECT_NEAR(first, std::sin(1.0), 1e-12);

	// The same traverser returns the cached table for the same node and
	// range, even after the node's inputs change
	node->SetStandardValue(olive::TrigonometryNode::kXIn, 0.0);
	const double cached =
		traverser.GenerateTable(node, range)
			.Get(olive::NodeValue::kFloat)
			.toDouble();
	EXPECT_DOUBLE_EQ(cached, first);

	// A fresh traverser recomputes
	olive::NodeTraverser fresh;
	const double updated =
		fresh.GenerateTable(node, range)
			.Get(olive::NodeValue::kFloat)
			.toDouble();
	EXPECT_DOUBLE_EQ(updated, 0.0);
}
