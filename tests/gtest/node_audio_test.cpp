#include <gtest/gtest.h>

#include <vector>

#include <QStringList>
#include <QVector3D>

#include "node/audio/pan/pan.h"
#include "node/audio/volume/volume.h"
#include "node/color/colormanager/colormanager.h"
#include "node/globals.h"
#include "node/input/time/timeinput.h"
#include "node/input/value/valuenode.h"
#include "node/keyframe.h"
#include "node/project.h"
#include "node/traverser.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/samplebuffer.h"
#include "olive/core/util/rational.h"
#include "render/job/samplejob.h"
#include "widget/slider/floatslider.h"

namespace
{

// Minimal node that emits a single configurable value, used to feed the
// samples input of the audio nodes without any decoder or audio hardware.
class ConstantValueNode : public olive::Node {
public:
	ConstantValueNode() = default;

	NODE_DEFAULT_FUNCTIONS(ConstantValueNode)

	virtual QString Name() const override
	{
		return QStringLiteral("Test Constant");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.constant");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return { kCategoryGenerator };
	}

	void SetOutput(const olive::NodeValue &value)
	{
		output_ = value;
	}

	virtual void Value(const olive::NodeValueRow &value,
					   const olive::NodeGlobals &globals,
					   olive::NodeValueTable *table) const override
	{
		Q_UNUSED(value)
		Q_UNUSED(globals)

		table->Push(output_);
	}

private:
	olive::NodeValue output_;
};

// Traverser that resolves SampleJobs on the CPU (no audio hardware) so the
// non-static (job) paths of the audio nodes can be verified end to end.
class SampleResolvingTraverser : public olive::NodeTraverser {
public:
	void Resolve(olive::NodeValue &value)
	{
		ResolveJobs(value);
	}

protected:
	virtual olive::core::SampleBuffer
	CreateSampleBuffer(const olive::core::AudioParams &params,
					   int sample_count) override
	{
		return olive::core::SampleBuffer(params, size_t(sample_count));
	}

	virtual void ProcessSamples(olive::core::SampleBuffer &destination,
								const olive::Node *node,
								const olive::TimeRange &range,
								const olive::SampleJob &job) override
	{
		Q_UNUSED(range)

		for (size_t i = 0; i < destination.sample_count(); i++) {
			node->ProcessSamples(job.GetValues(), job.samples(), destination,
								 int(i));
		}
	}
};

template <typename T> T *AddNode(olive::Project *project)
{
	T *node = new T();
	node->setParent(project);
	return node;
}

ConstantValueNode *AddConstant(olive::Project *project,
							   const olive::NodeValue &value)
{
	auto *node = new ConstantValueNode();
	node->setParent(project);
	node->SetOutput(value);
	return node;
}

olive::NodeKeyframe *AddKey(olive::Node *node, const QString &input,
							const olive::core::rational &time,
							const QVariant &value)
{
	auto *key = new olive::NodeKeyframe(
		time, value, olive::NodeKeyframe::kLinear, 0, -1, input);
	key->setParent(node);
	return key;
}

// A fresh traverser per call: NodeTraverser caches tables per node+range, so
// reusing one would return stale results after the node's parameters change.
olive::NodeValueTable GenerateTableAt(const olive::Node *node,
									  const olive::core::rational &time)
{
	olive::NodeTraverser traverser;
	return traverser.GenerateTable(
		node, olive::TimeRange(time, time + olive::core::rational(1, 30)));
}

olive::core::AudioParams StereoParams()
{
	return olive::core::AudioParams(48000, olive::core::kChannelLayoutStereo,
									olive::core::SampleFormat::F32P);
}

olive::core::AudioParams MonoParams()
{
	return olive::core::AudioParams(48000, olive::core::kChannelLayoutMono,
									olive::core::SampleFormat::F32P);
}

// Creates a stereo buffer with the given per-channel samples. Both channels
// must have the same number of samples.
olive::core::SampleBuffer MakeStereoBuffer(const std::vector<float> &channel0,
										   const std::vector<float> &channel1)
{
	olive::core::SampleBuffer buffer(StereoParams(), channel0.size());
	for (size_t i = 0; i < channel0.size(); i++) {
		buffer.data(0)[i] = channel0[i];
	}
	for (size_t i = 0; i < channel1.size(); i++) {
		buffer.data(1)[i] = channel1[i];
	}
	return buffer;
}

olive::core::SampleBuffer MakeMonoBuffer(const std::vector<float> &samples)
{
	olive::core::SampleBuffer buffer(MonoParams(), samples.size());
	for (size_t i = 0; i < samples.size(); i++) {
		buffer.data(0)[i] = samples[i];
	}
	return buffer;
}

olive::NodeValue SampleValue(const olive::core::SampleBuffer &buffer)
{
	return olive::NodeValue(olive::NodeValue::kSamples,
							QVariant::fromValue(buffer));
}

// Connects a stereo buffer {1,2,3,4}/{5,6,7,8} to the node's samples input.
ConstantValueNode *ConnectTestSamples(olive::Project *project,
									  olive::Node *node,
									  const QString &samples_input)
{
	ConstantValueNode *samples = AddConstant(
		project, SampleValue(MakeStereoBuffer({ 1.0f, 2.0f, 3.0f, 4.0f },
											  { 5.0f, 6.0f, 7.0f, 8.0f })));
	olive::Node::ConnectEdge(samples, olive::NodeInput(node, samples_input));
	return samples;
}

} // namespace

TEST(PanNode, Metadata)
{
	olive::PanNode pan;

	EXPECT_EQ(pan.Name(), QStringLiteral("Pan"));
	EXPECT_EQ(pan.id(), QStringLiteral("org.olivevideoeditor.Olive.pan"));
	EXPECT_FALSE(pan.Description().isEmpty());
	EXPECT_TRUE(pan.Category().contains(olive::Node::kCategoryFilter));

	// Registered as an audio effect with the samples input as effect input
	EXPECT_TRUE(pan.GetFlags() & olive::Node::kAudioEffect);
	EXPECT_EQ(pan.GetEffectInputID(), olive::PanNode::kSamplesInput);
}

TEST(PanNode, InputDefaults)
{
	olive::PanNode pan;

	EXPECT_EQ(pan.GetInputDataType(olive::PanNode::kSamplesInput),
			  olive::NodeValue::kSamples);
	EXPECT_FALSE(pan.IsInputKeyframable(olive::PanNode::kSamplesInput));
	EXPECT_TRUE(pan.IsInputConnectable(olive::PanNode::kSamplesInput));

	EXPECT_EQ(pan.GetInputDataType(olive::PanNode::kPanningInput),
			  olive::NodeValue::kFloat);
	EXPECT_TRUE(pan.IsInputKeyframable(olive::PanNode::kPanningInput));
	EXPECT_DOUBLE_EQ(
		pan.GetStandardValue(olive::PanNode::kPanningInput).toDouble(), 0.0);
	EXPECT_DOUBLE_EQ(pan.GetInputProperty(olive::PanNode::kPanningInput,
										  QStringLiteral("min"))
						 .toDouble(),
					 -1.0);
	EXPECT_DOUBLE_EQ(pan.GetInputProperty(olive::PanNode::kPanningInput,
										  QStringLiteral("max"))
						 .toDouble(),
					 1.0);
	EXPECT_EQ(int(pan.GetInputProperty(olive::PanNode::kPanningInput,
									   QStringLiteral("view"))
					  .toInt()),
			  int(olive::FloatSlider::kPercentage));
}

TEST(PanNode, RetranslateSetsInputNames)
{
	olive::PanNode pan;

	pan.Retranslate();

	EXPECT_EQ(pan.GetInputName(olive::PanNode::kSamplesInput),
			  QStringLiteral("Samples"));
	EXPECT_EQ(pan.GetInputName(olive::PanNode::kPanningInput),
			  QStringLiteral("Pan"));
}

TEST(PanNode, StaticCenterPanLeavesSamplesUnchanged)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::PanNode *pan = AddNode<olive::PanNode>(&project);
	ConnectTestSamples(&project, pan, olive::PanNode::kSamplesInput);

	// Pan 0 is a no-op, but the (unmodified) buffer is still pushed
	const olive::NodeValueTable table = GenerateTableAt(pan, olive::core::rational(0));
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	ASSERT_EQ(out.sample_count(), 4u);
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], float(i + 1));
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5));
	}
}

TEST(PanNode, StaticRightPanAttenuatesLeftChannel)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::PanNode *pan = AddNode<olive::PanNode>(&project);
	pan->SetStandardValue(olive::PanNode::kPanningInput, 0.5);
	ConnectTestSamples(&project, pan, olive::PanNode::kSamplesInput);

	const olive::NodeValueTable table = GenerateTableAt(pan, olive::core::rational(0));
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], float(i + 1) * 0.5f);
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5));
	}
}

TEST(PanNode, StaticLeftPanAttenuatesRightChannel)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::PanNode *pan = AddNode<olive::PanNode>(&project);
	pan->SetStandardValue(olive::PanNode::kPanningInput, -0.5);
	ConnectTestSamples(&project, pan, olive::PanNode::kSamplesInput);

	const olive::NodeValueTable table = GenerateTableAt(pan, olive::core::rational(0));
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], float(i + 1));
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5) * 0.5f);
	}
}

TEST(PanNode, StaticFullRightPanSilencesLeftChannel)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::PanNode *pan = AddNode<olive::PanNode>(&project);
	pan->SetStandardValue(olive::PanNode::kPanningInput, 1.0);
	ConnectTestSamples(&project, pan, olive::PanNode::kSamplesInput);

	const olive::NodeValueTable table = GenerateTableAt(pan, olive::core::rational(0));
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], 0.0f);
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5));
	}
}

TEST(PanNode, NonStereoSamplesPassThroughUnchanged)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::PanNode *pan = AddNode<olive::PanNode>(&project);
	pan->SetStandardValue(olive::PanNode::kPanningInput, 0.5);

	ConstantValueNode *samples = AddConstant(
		&project, SampleValue(MakeMonoBuffer({ 1.0f, -2.0f, 3.0f })));
	olive::Node::ConnectEdge(
		samples, olive::NodeInput(pan, olive::PanNode::kSamplesInput));

	// Pan only supports stereo: a mono buffer is pushed through untouched
	const olive::NodeValueTable table = GenerateTableAt(pan, olive::core::rational(0));
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	ASSERT_EQ(out.audio_params().channel_count(), 1);
	ASSERT_EQ(out.sample_count(), 3u);
	EXPECT_FLOAT_EQ(out.data(0)[0], 1.0f);
	EXPECT_FLOAT_EQ(out.data(0)[1], -2.0f);
	EXPECT_FLOAT_EQ(out.data(0)[2], 3.0f);
}

TEST(PanNode, NoSamplesInputProducesNoOutput)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::PanNode *pan = AddNode<olive::PanNode>(&project);
	pan->SetStandardValue(olive::PanNode::kPanningInput, 0.5);

	// Without an allocated buffer on the samples input, Value() pushes nothing
	const olive::NodeValueTable table = GenerateTableAt(pan, olive::core::rational(0));
	EXPECT_EQ(table.Get(olive::NodeValue::kSamples).type(),
			  olive::NodeValue::kNone);
}

TEST(PanNode, KeyframedPanProducesSampleJobWithPanValue)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::PanNode *pan = AddNode<olive::PanNode>(&project);
	ConnectTestSamples(&project, pan, olive::PanNode::kSamplesInput);

	// A non-static (keyframed) pan makes Value() push a SampleJob instead of
	// processing the buffer immediately
	pan->SetInputIsKeyframing(olive::PanNode::kPanningInput, true);
	AddKey(pan, olive::PanNode::kPanningInput, olive::core::rational(0), 1.0);

	const olive::NodeValueTable table = GenerateTableAt(pan, olive::core::rational(0));
	olive::NodeValue result = table.Get(olive::NodeValue::kSamples);
	ASSERT_EQ(result.type(), olive::NodeValue::kSamples);
	ASSERT_TRUE(result.canConvert<olive::SampleJob>());

	// Like VolumeNode, PanNode::Value() inserts the panning value into the
	// SampleJob so ProcessSamples() sees the keyframed pan
	const olive::SampleJob job = result.value<olive::SampleJob>();
	ASSERT_TRUE(job.GetValues().contains(olive::PanNode::kPanningInput));
	EXPECT_DOUBLE_EQ(job.GetValues().value(olive::PanNode::kPanningInput).toDouble(),
					 1.0);

	SampleResolvingTraverser resolver;
	resolver.Resolve(result);

	// Pan 1.0 (full right) silences the left channel and leaves the right
	const olive::core::SampleBuffer out = result.toSamples();
	ASSERT_TRUE(out.is_allocated());
	ASSERT_EQ(out.sample_count(), 4u);
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], 0.0f);
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5));
	}
}

TEST(PanNode, ProcessSamplesAppliesPanPerSample)
{
	olive::PanNode pan;

	olive::NodeValueRow row;
	row.insert(olive::PanNode::kPanningInput,
			   olive::NodeValue(olive::NodeValue::kFloat, 0.5));

	olive::core::SampleBuffer input(StereoParams(), 2);
	olive::core::SampleBuffer output(StereoParams(), 2);
	input.data(0)[0] = 1.5f;
	input.data(0)[1] = -2.0f;
	input.data(1)[0] = 0.25f;
	input.data(1)[1] = 8.0f;

	pan.ProcessSamples(row, input, output, 0);
	pan.ProcessSamples(row, input, output, 1);

	// Panning right attenuates the left channel only
	EXPECT_FLOAT_EQ(output.data(0)[0], 0.75f);
	EXPECT_FLOAT_EQ(output.data(0)[1], -1.0f);
	EXPECT_FLOAT_EQ(output.data(1)[0], 0.25f);
	EXPECT_FLOAT_EQ(output.data(1)[1], 8.0f);

	// Panning left attenuates the right channel only
	row.insert(olive::PanNode::kPanningInput,
			   olive::NodeValue(olive::NodeValue::kFloat, -0.25));
	pan.ProcessSamples(row, input, output, 0);

	EXPECT_FLOAT_EQ(output.data(0)[0], 1.5f);
	EXPECT_FLOAT_EQ(output.data(1)[0], 0.1875f);
}

TEST(PanNode, ProcessSamplesWithoutPanValueCopiesInput)
{
	olive::PanNode pan;

	// No panning value in the row: samples are copied unchanged
	olive::NodeValueRow row;

	olive::core::SampleBuffer input(StereoParams(), 1);
	olive::core::SampleBuffer output(StereoParams(), 1);
	input.data(0)[0] = 3.0f;
	input.data(1)[0] = -4.0f;

	pan.ProcessSamples(row, input, output, 0);

	EXPECT_FLOAT_EQ(output.data(0)[0], 3.0f);
	EXPECT_FLOAT_EQ(output.data(1)[0], -4.0f);
}

TEST(VolumeNode, Metadata)
{
	olive::VolumeNode volume;

	EXPECT_EQ(volume.Name(), QStringLiteral("Volume"));
	EXPECT_EQ(volume.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.volume"));
	EXPECT_FALSE(volume.Description().isEmpty());
	EXPECT_TRUE(volume.Category().contains(olive::Node::kCategoryFilter));

	EXPECT_TRUE(volume.GetFlags() & olive::Node::kAudioEffect);
	EXPECT_EQ(volume.GetEffectInputID(), olive::VolumeNode::kSamplesInput);
}

TEST(VolumeNode, InputDefaults)
{
	olive::VolumeNode volume;

	EXPECT_EQ(volume.GetInputDataType(olive::VolumeNode::kSamplesInput),
			  olive::NodeValue::kSamples);
	EXPECT_FALSE(volume.IsInputKeyframable(olive::VolumeNode::kSamplesInput));

	EXPECT_EQ(volume.GetInputDataType(olive::VolumeNode::kVolumeInput),
			  olive::NodeValue::kFloat);
	EXPECT_TRUE(volume.IsInputKeyframable(olive::VolumeNode::kVolumeInput));
	EXPECT_DOUBLE_EQ(
		volume.GetStandardValue(olive::VolumeNode::kVolumeInput).toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(volume.GetInputProperty(olive::VolumeNode::kVolumeInput,
											 QStringLiteral("min"))
						 .toDouble(),
					 0.0);
	EXPECT_EQ(int(volume.GetInputProperty(olive::VolumeNode::kVolumeInput,
										  QStringLiteral("view"))
					  .toInt()),
			  int(olive::FloatSlider::kDecibel));
}

TEST(VolumeNode, RetranslateSetsInputNames)
{
	olive::VolumeNode volume;

	volume.Retranslate();

	EXPECT_EQ(volume.GetInputName(olive::VolumeNode::kSamplesInput),
			  QStringLiteral("Samples"));
	EXPECT_EQ(volume.GetInputName(olive::VolumeNode::kVolumeInput),
			  QStringLiteral("Volume"));
}

TEST(VolumeNode, StaticVolumeScalesSamples)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::VolumeNode *volume = AddNode<olive::VolumeNode>(&project);
	volume->SetStandardValue(olive::VolumeNode::kVolumeInput, 2.0);
	ConnectTestSamples(&project, volume, olive::VolumeNode::kSamplesInput);

	const olive::NodeValueTable table = GenerateTableAt(volume, olive::core::rational(0));
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	ASSERT_EQ(out.sample_count(), 4u);
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], float(i + 1) * 2.0f);
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5) * 2.0f);
	}
}

TEST(VolumeNode, StaticUnityVolumeLeavesSamplesUnchanged)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::VolumeNode *volume = AddNode<olive::VolumeNode>(&project);
	volume->SetStandardValue(olive::VolumeNode::kVolumeInput, 1.0);
	ConnectTestSamples(&project, volume, olive::VolumeNode::kSamplesInput);

	// Volume 1 is a no-op, but the (unmodified) buffer is still pushed
	const olive::NodeValueTable table = GenerateTableAt(volume, olive::core::rational(0));
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], float(i + 1));
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5));
	}
}

TEST(VolumeNode, StaticZeroVolumeSilencesSamples)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::VolumeNode *volume = AddNode<olive::VolumeNode>(&project);
	volume->SetStandardValue(olive::VolumeNode::kVolumeInput, 0.0);
	ConnectTestSamples(&project, volume, olive::VolumeNode::kSamplesInput);

	const olive::NodeValueTable table = GenerateTableAt(volume, olive::core::rational(0));
	const olive::core::SampleBuffer out =
		table.Get(olive::NodeValue::kSamples).toSamples();
	ASSERT_TRUE(out.is_allocated());
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], 0.0f);
		EXPECT_FLOAT_EQ(out.data(1)[i], 0.0f);
	}
}

TEST(VolumeNode, KeyframedVolumeProducesSampleJobWithInterpolatedValue)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::VolumeNode *volume = AddNode<olive::VolumeNode>(&project);
	ConnectTestSamples(&project, volume, olive::VolumeNode::kSamplesInput);

	// A non-static (keyframed) volume makes Value() push a SampleJob carrying
	// the volume value instead of processing the buffer immediately
	volume->SetInputIsKeyframing(olive::VolumeNode::kVolumeInput, true);
	AddKey(volume, olive::VolumeNode::kVolumeInput, olive::core::rational(0),
		   1.0);
	AddKey(volume, olive::VolumeNode::kVolumeInput, olive::core::rational(1),
		   3.0);

	// At t=0.5 the linear ramp 1.0 -> 3.0 interpolates to 2.0
	const olive::NodeValueTable table = GenerateTableAt(volume, olive::core::rational(1, 2));
	olive::NodeValue result = table.Get(olive::NodeValue::kSamples);
	ASSERT_EQ(result.type(), olive::NodeValue::kSamples);
	ASSERT_TRUE(result.canConvert<olive::SampleJob>());

	const olive::SampleJob job = result.value<olive::SampleJob>();
	ASSERT_TRUE(job.GetValues().contains(olive::VolumeNode::kVolumeInput));
	EXPECT_DOUBLE_EQ(job.Get(olive::VolumeNode::kVolumeInput).toDouble(), 2.0);

	// Resolve the job on the CPU and verify the scaled samples
	SampleResolvingTraverser resolver;
	resolver.Resolve(result);

	const olive::core::SampleBuffer out = result.toSamples();
	ASSERT_TRUE(out.is_allocated());
	ASSERT_EQ(out.sample_count(), 4u);
	for (int i = 0; i < 4; i++) {
		EXPECT_FLOAT_EQ(out.data(0)[i], float(i + 1) * 2.0f);
		EXPECT_FLOAT_EQ(out.data(1)[i], float(i + 5) * 2.0f);
	}
}

TEST(VolumeNode, ProcessSamplesMultipliesPerSample)
{
	olive::VolumeNode volume;

	olive::NodeValueRow row;
	row.insert(olive::VolumeNode::kVolumeInput,
			   olive::NodeValue(olive::NodeValue::kFloat, 2.0));

	olive::core::SampleBuffer input(StereoParams(), 2);
	olive::core::SampleBuffer output(StereoParams(), 2);
	input.data(0)[0] = 1.5f;
	input.data(0)[1] = -2.0f;
	input.data(1)[0] = 0.25f;
	input.data(1)[1] = 8.0f;

	volume.ProcessSamples(row, input, output, 0);
	volume.ProcessSamples(row, input, output, 1);

	EXPECT_FLOAT_EQ(output.data(0)[0], 3.0f);
	EXPECT_FLOAT_EQ(output.data(0)[1], -4.0f);
	EXPECT_FLOAT_EQ(output.data(1)[0], 0.5f);
	EXPECT_FLOAT_EQ(output.data(1)[1], 16.0f);
}

TEST(VolumeNode, ProcessSamplesWithoutVolumeLeavesOutputUntouched)
{
	olive::VolumeNode volume;

	// Neither the samples nor the volume input carries a number: the output
	// must not be written
	olive::NodeValueRow row;

	olive::core::SampleBuffer input(StereoParams(), 1);
	olive::core::SampleBuffer output(StereoParams(), 1);
	input.data(0)[0] = 10.0f;
	output.data(0)[0] = 123.0f;
	output.data(1)[0] = 45.0f;

	volume.ProcessSamples(row, input, output, 0);

	EXPECT_FLOAT_EQ(output.data(0)[0], 123.0f);
	EXPECT_FLOAT_EQ(output.data(1)[0], 45.0f);
}

TEST(TimeInput, Metadata)
{
	olive::TimeInput time;

	EXPECT_EQ(time.Name(), QStringLiteral("Time"));
	EXPECT_EQ(time.id(), QStringLiteral("org.olivevideoeditor.Olive.time"));
	EXPECT_FALSE(time.Description().isEmpty());
	EXPECT_TRUE(time.Category().contains(olive::Node::kCategoryTime));
}

TEST(TimeInput, ValuePushesCurrentTimeInSeconds)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::TimeInput *time = AddNode<olive::TimeInput>(&project);

	olive::NodeValueTable table = GenerateTableAt(time, olive::core::rational(0));
	olive::NodeValue result = table.Get(olive::NodeValue::kFloat);
	ASSERT_EQ(result.type(), olive::NodeValue::kFloat);
	EXPECT_DOUBLE_EQ(result.toDouble(), 0.0);

	table = GenerateTableAt(time, olive::core::rational(5, 2));
	result = table.Get(olive::NodeValue::kFloat);
	ASSERT_EQ(result.type(), olive::NodeValue::kFloat);
	EXPECT_DOUBLE_EQ(result.toDouble(), 2.5);
}

TEST(ValueNode, Metadata)
{
	olive::ValueNode value;

	EXPECT_EQ(value.Name(), QStringLiteral("Value"));
	EXPECT_EQ(value.id(), QStringLiteral("org.olivevideoeditor.Olive.value"));
	EXPECT_FALSE(value.Description().isEmpty());
	EXPECT_TRUE(value.Category().contains(olive::Node::kCategoryGenerator));
}

TEST(ValueNode, InputDefaults)
{
	olive::ValueNode value;

	EXPECT_EQ(value.GetInputDataType(olive::ValueNode::kTypeInput),
			  olive::NodeValue::kCombo);
	EXPECT_FALSE(value.IsInputConnectable(olive::ValueNode::kTypeInput));
	EXPECT_FALSE(value.IsInputKeyframable(olive::ValueNode::kTypeInput));
	EXPECT_EQ(value.GetStandardValue(olive::ValueNode::kTypeInput).toInt(), 0);

	// The value input starts out as a float (the first supported type)
	EXPECT_EQ(value.GetInputDataType(olive::ValueNode::kValueInput),
			  olive::NodeValue::kFloat);
	EXPECT_FALSE(value.IsInputConnectable(olive::ValueNode::kValueInput));
	EXPECT_TRUE(value.IsInputKeyframable(olive::ValueNode::kValueInput));
}

TEST(ValueNode, RetranslateSetsInputNamesAndTypeCombo)
{
	olive::ValueNode value;

	value.Retranslate();

	EXPECT_EQ(value.GetInputName(olive::ValueNode::kTypeInput),
			  QStringLiteral("Type"));
	EXPECT_EQ(value.GetInputName(olive::ValueNode::kValueInput),
			  QStringLiteral("Value"));

	// The type combo lists the pretty name of every supported type, in order
	const QStringList types =
		value.GetInputProperty(olive::ValueNode::kTypeInput,
							   QStringLiteral("combo_str"))
			.toStringList();
	ASSERT_EQ(types.size(), 11);
	EXPECT_EQ(types.at(0), QStringLiteral("Float"));
	EXPECT_EQ(types.at(1), QStringLiteral("Integer"));
	EXPECT_EQ(types.at(2), QStringLiteral("Rational"));
	EXPECT_EQ(types.at(3), QStringLiteral("Vector 2D"));
	EXPECT_EQ(types.at(4), QStringLiteral("Vector 3D"));
	EXPECT_EQ(types.at(5), QStringLiteral("Vector 4D"));
	EXPECT_EQ(types.at(6), QStringLiteral("Color"));
	EXPECT_EQ(types.at(7), QStringLiteral("Text"));
	EXPECT_EQ(types.at(8), QStringLiteral("Matrix"));
	EXPECT_EQ(types.at(9), QStringLiteral("Font"));
	EXPECT_EQ(types.at(10), QStringLiteral("Boolean"));
}

TEST(ValueNode, ChangingTypeSwitchesValueInputDataType)
{
	olive::ValueNode value;

	EXPECT_EQ(value.GetInputDataType(olive::ValueNode::kValueInput),
			  olive::NodeValue::kFloat);

	value.SetStandardValue(olive::ValueNode::kTypeInput, 1);
	EXPECT_EQ(value.GetInputDataType(olive::ValueNode::kValueInput),
			  olive::NodeValue::kInt);

	value.SetStandardValue(olive::ValueNode::kTypeInput, 3);
	EXPECT_EQ(value.GetInputDataType(olive::ValueNode::kValueInput),
			  olive::NodeValue::kVec2);

	value.SetStandardValue(olive::ValueNode::kTypeInput, 6);
	EXPECT_EQ(value.GetInputDataType(olive::ValueNode::kValueInput),
			  olive::NodeValue::kColor);

	value.SetStandardValue(olive::ValueNode::kTypeInput, 10);
	EXPECT_EQ(value.GetInputDataType(olive::ValueNode::kValueInput),
			  olive::NodeValue::kBoolean);

	value.SetStandardValue(olive::ValueNode::kTypeInput, 0);
	EXPECT_EQ(value.GetInputDataType(olive::ValueNode::kValueInput),
			  olive::NodeValue::kFloat);
}

TEST(ValueNode, ValuePassesThroughFloat)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::ValueNode *value = AddNode<olive::ValueNode>(&project);
	value->SetStandardValue(olive::ValueNode::kValueInput, 3.5);

	const olive::NodeValueTable table = GenerateTableAt(value, olive::core::rational(0));
	const olive::NodeValue result = table.Get(olive::NodeValue::kFloat);
	ASSERT_EQ(result.type(), olive::NodeValue::kFloat);
	EXPECT_DOUBLE_EQ(result.toDouble(), 3.5);
}

TEST(ValueNode, ValuePassesThroughVectorAfterTypeSwitch)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::ValueNode *value = AddNode<olive::ValueNode>(&project);

	// Switch the value input to kVec3 (index 4) and set a vector value
	value->SetStandardValue(olive::ValueNode::kTypeInput, 4);
	value->SetStandardValue(olive::ValueNode::kValueInput,
							QVector3D(1.0f, 2.0f, 3.0f));

	const olive::NodeValueTable table = GenerateTableAt(value, olive::core::rational(0));
	const olive::NodeValue result = table.Get(olive::NodeValue::kVec3);
	ASSERT_EQ(result.type(), olive::NodeValue::kVec3);
	const QVector3D vec = result.toVec3();
	EXPECT_FLOAT_EQ(vec.x(), 1.0f);
	EXPECT_FLOAT_EQ(vec.y(), 2.0f);
	EXPECT_FLOAT_EQ(vec.z(), 3.0f);
}
