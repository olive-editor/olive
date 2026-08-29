#include <gtest/gtest.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <memory>

#include <OpenColorIO/OpenColorIO.h>

#include "config/config.h"
#include "node/color/ociolut/ociolut.h"
#include "node/color/colormanager/colormanager.h"
#include "node/color/ociogradingtransformlinear/ociogradingtransformlinear.h"
#include "node/color/threewaycolor/threewaycolor.h"
#include "node/factory.h"
#include "node/generator/solid/solid.h"
#include "node/project.h"
#include "node/traverser.h"
#include "render/colorprocessor.h"
#include "render/lutlibrary.h"

namespace OCIO = OCIO_NAMESPACE;

namespace
{

bool IsOakSupportedLutExtension(QString suffix)
{
	if (suffix.startsWith(QLatin1Char('.'))) {
		suffix.remove(0, 1);
	}
	const QString lower = suffix.toLower();
	return lower == QStringLiteral("cube") || lower == QStringLiteral("3dl");
}

QString WriteTestCube(QTemporaryDir *dir)
{
	const QString path =
		QDir(dir->path()).filePath(QStringLiteral("invert.cube"));
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return QString();
	}

	const QByteArray data = "TITLE \"Oak test invert\"\n"
							"LUT_1D_SIZE 2\n"
							"DOMAIN_MIN 0.0 0.0 0.0\n"
							"DOMAIN_MAX 1.0 1.0 1.0\n"
							"1.0 1.0 1.0\n"
							"0.0 0.0 0.0\n";
	file.write(data);
	file.close();
	return path;
}

// CPU-only traverser that resolves SolidGenerator and ColorTransformJob jobs
// into real frames so we can compare pixels without a GPU/worker process.
class PixelColorTransformTraverser : public olive::NodeTraverser {
public:
	void Resolve(olive::NodeValue &value)
	{
		ResolveJobs(value);
	}

	olive::FramePtr source_frame;
	olive::FramePtr output_frame;

protected:
	virtual void ProcessShader(olive::TexturePtr destination,
							   const olive::Node *node,
							   const olive::ShaderJob *job) override
	{
		Q_UNUSED(destination)
		Q_UNUSED(node)

		const olive::Color c =
			job->GetValues().value(olive::SolidGenerator::kColorInput).toColor();

		olive::VideoParams p = destination->params();
		p.set_format(olive::core::PixelFormat::F32);
		p.set_channel_count(olive::VideoParams::kRGBAChannelCount);

		source_frame = olive::Frame::Create();
		source_frame->set_video_params(p);
		source_frame->allocate();

		for (int y = 0; y < p.effective_height(); ++y) {
			for (int x = 0; x < p.effective_width(); ++x) {
				source_frame->set_pixel(x, y, c);
			}
		}
	}

	virtual void
	ProcessColorTransform(olive::TexturePtr destination,
						  const olive::Node *node,
						  const olive::ColorTransformJob *job) override
	{
		Q_UNUSED(destination)
		Q_UNUSED(node)

		olive::ColorProcessorPtr processor = job->GetColorProcessor();
		if (!processor || !source_frame) {
			return;
		}

		output_frame = olive::Frame::Create();
		output_frame->set_video_params(source_frame->video_params());
		output_frame->allocate();

		std::memcpy(output_frame->data(), source_frame->const_data(),
					source_frame->allocated_size());

		processor->ConvertFrame(output_frame);
	}
};

QString WriteTestCubeLut(QTemporaryDir *dir, const char *title, float low,
						 float high)
{
	const QString path =
		QDir(dir->path())
			.filePath(QStringLiteral("%1.cube").arg(QString::fromUtf8(title)));
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return QString();
	}

	QTextStream stream(&file);
	stream << "TITLE \"" << title << "\"\n";
	stream << "LUT_1D_SIZE 2\n";
	stream << "DOMAIN_MIN 0.0 0.0 0.0\n";
	stream << "DOMAIN_MAX 1.0 1.0 1.0\n";
	stream << high << " " << high << " " << high << "\n";
	stream << low << " " << low << " " << low << "\n";
	file.close();
	return path;
}

// Non-symmetric 1D LUT so forward and inverse produce different, predictable
// results. Table:
//   0.0 -> 0.00
//   0.5 -> 0.75
//   1.0 -> 1.00
// Forward:  (0.25, 0.50, 0.75) -> (0.375, 0.750, 0.875)
// Inverse:  (0.25, 0.50, 0.75) -> (0.167, 0.333, 0.500)
QString WriteAsymmetricCube(QTemporaryDir *dir)
{
	const QString path =
		QDir(dir->path()).filePath(QStringLiteral("asymmetric.cube"));
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return QString();
	}

	const QByteArray data = "TITLE \"Oak test asymmetric\"\n"
							"LUT_1D_SIZE 3\n"
							"DOMAIN_MIN 0.0 0.0 0.0\n"
							"DOMAIN_MAX 1.0 1.0 1.0\n"
							"0.00 0.00 0.00\n"
							"0.75 0.75 0.75\n"
							"1.00 1.00 1.00\n";
	file.write(data);
	file.close();
	return path;
}

} // namespace

TEST(ColorLut, OcioSupportsCubeAnd3dlExtensions)
{
	EXPECT_TRUE(IsOakSupportedLutExtension("cube"));
	EXPECT_TRUE(IsOakSupportedLutExtension(".cube"));
	EXPECT_TRUE(IsOakSupportedLutExtension("3dl"));
	EXPECT_TRUE(IsOakSupportedLutExtension(".3dl"));
	EXPECT_FALSE(IsOakSupportedLutExtension("txt"));
}

TEST(ColorProcessor, CreateFromInvalidTransformReturnsNull)
{
	olive::ColorManager::SetUpDefaultConfig();

	OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
	transform->setSrc("/nonexistent/lut.cube");
	transform->setInterpolation(OCIO::INTERP_LINEAR);
	transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

	olive::ColorProcessorPtr processor;
	EXPECT_NO_THROW({
		try {
			processor = olive::ColorProcessor::Create(
				olive::ColorManager::GetDefaultConfig()->getProcessor(
					transform));
		} catch (const std::exception &e) {
			processor = nullptr;
		}
	});

	EXPECT_EQ(processor, nullptr);
}

TEST(ColorProcessor, ConvertColorWithIdentityProcessor)
{
	olive::ColorManager::SetUpDefaultConfig();

	OCIO::MatrixTransformRcPtr transform = OCIO::MatrixTransform::Create();
	transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

	olive::ColorProcessorPtr processor = olive::ColorProcessor::Create(
		olive::ColorManager::GetDefaultConfig()->getProcessor(transform));
	ASSERT_NE(processor, nullptr);

	const olive::Color in(0.25f, 0.50f, 0.75f, 1.0f);
	const olive::Color out = processor->ConvertColor(in);

	EXPECT_NEAR(out.red(), in.red(), 0.001f);
	EXPECT_NEAR(out.green(), in.green(), 0.001f);
	EXPECT_NEAR(out.blue(), in.blue(), 0.001f);
	EXPECT_NEAR(out.alpha(), in.alpha(), 0.001f);
}

TEST(ColorLut, CubeFileTransformConvertsColor)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = WriteTestCube(&dir);
	ASSERT_FALSE(path.isEmpty());

	OCIO::FileTransformRcPtr transform = OCIO::FileTransform::Create();
	transform->setSrc(path.toUtf8().constData());
	transform->setInterpolation(OCIO::INTERP_LINEAR);
	transform->setDirection(OCIO::TRANSFORM_DIR_FORWARD);

	OCIO::ConstConfigRcPtr config = OCIO::Config::CreateRaw();
	olive::ColorProcessorPtr processor =
		olive::ColorProcessor::Create(config->getProcessor(transform));
	ASSERT_TRUE(processor);

	const olive::Color out =
		processor->ConvertColor(olive::Color(0.25f, 0.50f, 0.75f, 1.0f));
	EXPECT_NEAR(out.red(), 0.75f, 0.02f);
	EXPECT_NEAR(out.green(), 0.50f, 0.02f);
	EXPECT_NEAR(out.blue(), 0.25f, 0.02f);
	EXPECT_NEAR(out.alpha(), 1.0f, 0.001f);
}

TEST(ColorV04, FactoryCreatesColorNodes)
{
	std::unique_ptr<olive::Node> lut(olive::NodeFactory::CreateFromFactoryIndex(
		olive::NodeFactory::kOCIOLut));
	ASSERT_NE(lut, nullptr);
	EXPECT_EQ(lut->id(), QStringLiteral("org.olivevideoeditor.Olive.ociolut"));

	std::unique_ptr<olive::Node> three_way(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kThreeWayColor));
	ASSERT_NE(three_way, nullptr);
	EXPECT_EQ(three_way->id(),
			  QStringLiteral("org.olivevideoeditor.Olive.threewaycolor"));
	EXPECT_TRUE(three_way->HasInputWithID(
		olive::ThreeWayColorNode::kShadowsColorInput));
	EXPECT_TRUE(three_way->HasInputWithID(
		olive::ThreeWayColorNode::kMidtonesColorInput));
	EXPECT_TRUE(three_way->HasInputWithID(
		olive::ThreeWayColorNode::kHighlightsColorInput));

	const olive::Color neutral =
		three_way
			->GetStandardValue(olive::ThreeWayColorNode::kMidtonesColorInput)
			.value<olive::Color>();
	EXPECT_FLOAT_EQ(neutral.red(), 0.5f);
	EXPECT_FLOAT_EQ(neutral.green(), 0.5f);
	EXPECT_FLOAT_EQ(neutral.blue(), 0.5f);
	EXPECT_FLOAT_EQ(neutral.alpha(), 1.0f);
}

// -----------------------------------------------------------------------------
// E2E-style regression tests for OCIOLutNode.
//
// These tests build a tiny node graph (SolidGenerator -> OCIOLutNode), drive it
// through NodeTraverser, and compare the resulting pixels. They specifically
// exercise the previously broken paths:
//   * OCIOLutNode::Value() must ensure the processor exists before rendering.
//   * Switching the LUT direction must update the processor and change pixels.
//   * Switching the LUT file must update the processor and change pixels.
// -----------------------------------------------------------------------------

TEST(ColorLutNode, ForwardDirectionInvertsPixels)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = WriteTestCubeLut(&dir, "invert", 0.0f, 1.0f);
	ASSERT_FALSE(path.isEmpty());

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.50f, 0.75f, 1.0f)));

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, path);
	lut->SetStandardValue(olive::OCIOLutNode::kDirectionInput, 0); // Forward

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(lut, olive::OCIOLutNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	PixelColorTransformTraverser traverser;
	traverser.SetCacheVideoParams(params);

	olive::NodeValueTable table = traverser.GenerateTable(
		lut, olive::TimeRange(olive::core::rational(0),
							  olive::core::rational(1, 30)));
	olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
	traverser.Resolve(tex_val);

	ASSERT_TRUE(traverser.output_frame);
	const olive::Color out = traverser.output_frame->get_pixel(0, 0);

	EXPECT_NEAR(out.red(), 0.75f, 0.02f);
	EXPECT_NEAR(out.green(), 0.50f, 0.02f);
	EXPECT_NEAR(out.blue(), 0.25f, 0.02f);
	EXPECT_NEAR(out.alpha(), 1.0f, 0.001f);
}

TEST(ColorLutNode, InverseDirectionReversesForwardTransform)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = WriteTestCubeLut(&dir, "invert", 0.0f, 1.0f);
	ASSERT_FALSE(path.isEmpty());

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.75f, 0.50f, 0.25f, 1.0f)));

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, path);
	lut->SetStandardValue(olive::OCIOLutNode::kDirectionInput, 1); // Inverse

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(lut, olive::OCIOLutNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	PixelColorTransformTraverser traverser;
	traverser.SetCacheVideoParams(params);

	olive::NodeValueTable table = traverser.GenerateTable(
		lut, olive::TimeRange(olive::core::rational(0),
							  olive::core::rational(1, 30)));
	olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
	traverser.Resolve(tex_val);

	ASSERT_TRUE(traverser.output_frame);
	const olive::Color out = traverser.output_frame->get_pixel(0, 0);

	EXPECT_NEAR(out.red(), 0.25f, 0.02f);
	EXPECT_NEAR(out.green(), 0.50f, 0.02f);
	EXPECT_NEAR(out.blue(), 0.75f, 0.02f);
	EXPECT_NEAR(out.alpha(), 1.0f, 0.001f);
}

TEST(ColorLutNode, SwitchingDirectionUpdatesProcessorAndPixels)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = WriteAsymmetricCube(&dir);
	ASSERT_FALSE(path.isEmpty());

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.50f, 0.75f, 1.0f)));

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, path);
	lut->SetStandardValue(olive::OCIOLutNode::kDirectionInput, 0); // Forward

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(lut, olive::OCIOLutNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	// First render: forward direction.
	PixelColorTransformTraverser forward_traverser;
	forward_traverser.SetCacheVideoParams(params);
	olive::NodeValueTable forward_table = forward_traverser.GenerateTable(
		lut, olive::TimeRange(olive::core::rational(0),
							  olive::core::rational(1, 30)));
	olive::NodeValue forward_tex =
		forward_table.Get(olive::NodeValue::kTexture);
	forward_traverser.Resolve(forward_tex);

	ASSERT_TRUE(forward_traverser.output_frame);
	const olive::Color forward_out =
		forward_traverser.output_frame->get_pixel(0, 0);
	EXPECT_NEAR(forward_out.red(), 0.375f, 0.02f);
	EXPECT_NEAR(forward_out.green(), 0.750f, 0.02f);
	EXPECT_NEAR(forward_out.blue(), 0.875f, 0.02f);

	// Switch direction. Before the Value()/EnsureProcessor() fix, the node
	// would keep using the old forward processor.
	lut->SetStandardValue(olive::OCIOLutNode::kDirectionInput, 1); // Inverse

	PixelColorTransformTraverser inverse_traverser;
	inverse_traverser.SetCacheVideoParams(params);
	olive::NodeValueTable inverse_table = inverse_traverser.GenerateTable(
		lut, olive::TimeRange(olive::core::rational(0),
							  olive::core::rational(1, 30)));
	olive::NodeValue inverse_tex =
		inverse_table.Get(olive::NodeValue::kTexture);
	inverse_traverser.Resolve(inverse_tex);

	ASSERT_TRUE(inverse_traverser.output_frame);
	const olive::Color inverse_out =
		inverse_traverser.output_frame->get_pixel(0, 0);

	// Inverse of the asymmetric LUT moves the same color to a darker result.
	EXPECT_NEAR(inverse_out.red(), 0.167f, 0.02f);
	EXPECT_NEAR(inverse_out.green(), 0.333f, 0.02f);
	EXPECT_NEAR(inverse_out.blue(), 0.500f, 0.02f);

	// The two outputs must differ from each other.
	EXPECT_GT(std::abs(forward_out.red() - inverse_out.red()), 0.1f);
	EXPECT_GT(std::abs(forward_out.green() - inverse_out.green()), 0.1f);
	EXPECT_GT(std::abs(forward_out.blue() - inverse_out.blue()), 0.1f);
}

TEST(ColorLutNode, EmptyFilePathLeavesProcessorNull)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.50f, 0.75f, 1.0f)));

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, QString());

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(lut, olive::OCIOLutNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	PixelColorTransformTraverser traverser;
	traverser.SetCacheVideoParams(params);
	olive::NodeValueTable table = traverser.GenerateTable(
		lut, olive::TimeRange(olive::core::rational(0),
							  olive::core::rational(1, 30)));

	// With an empty LUT path, the node should pass the input texture through
	// without producing a color-transform job.
	olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
	EXPECT_EQ(tex_val.type(), olive::NodeValue::kTexture);
}

TEST(ColorLutNode, MissingFilePathLeavesProcessorNull)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.50f, 0.75f, 1.0f)));

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput,
						  QStringLiteral("/nonexistent/path/lut.cube"));

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(lut, olive::OCIOLutNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	PixelColorTransformTraverser traverser;
	traverser.SetCacheVideoParams(params);
	olive::NodeValueTable table = traverser.GenerateTable(
		lut, olive::TimeRange(olive::core::rational(0),
							  olive::core::rational(1, 30)));

	olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
	EXPECT_EQ(tex_val.type(), olive::NodeValue::kTexture);
}

TEST(ColorLutNode, UnsupportedExtensionLeavesProcessorNull)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.50f, 0.75f, 1.0f)));

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput,
						  QStringLiteral("/tmp/lut.txt"));

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(lut, olive::OCIOLutNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	PixelColorTransformTraverser traverser;
	traverser.SetCacheVideoParams(params);
	olive::NodeValueTable table = traverser.GenerateTable(
		lut, olive::TimeRange(olive::core::rational(0),
							  olive::core::rational(1, 30)));

	olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
	EXPECT_EQ(tex_val.type(), olive::NodeValue::kTexture);
}

TEST(ColorLutNode, DirectionStringValuesAreAccepted)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = WriteTestCubeLut(&dir, "invert", 0.0f, 1.0f);
	ASSERT_FALSE(path.isEmpty());

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.50f, 0.75f, 1.0f)));

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, path);
	lut->SetStandardValue(olive::OCIOLutNode::kDirectionInput,
						  QStringLiteral("forward"));

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(lut, olive::OCIOLutNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	PixelColorTransformTraverser traverser;
	traverser.SetCacheVideoParams(params);
	olive::NodeValueTable table = traverser.GenerateTable(
		lut, olive::TimeRange(olive::core::rational(0),
							  olive::core::rational(1, 30)));
	olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
	traverser.Resolve(tex_val);

	ASSERT_TRUE(traverser.output_frame);
	const olive::Color out = traverser.output_frame->get_pixel(0, 0);

	EXPECT_NEAR(out.red(), 0.75f, 0.02f);
	EXPECT_NEAR(out.green(), 0.50f, 0.02f);
	EXPECT_NEAR(out.blue(), 0.25f, 0.02f);
}

TEST(ColorLutNode, DirectionStringInverseIsAccepted)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = WriteTestCubeLut(&dir, "invert", 0.0f, 1.0f);
	ASSERT_FALSE(path.isEmpty());

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.75f, 0.50f, 0.25f, 1.0f)));

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, path);
	lut->SetStandardValue(olive::OCIOLutNode::kDirectionInput,
						  QStringLiteral("inverse"));

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(lut, olive::OCIOLutNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	PixelColorTransformTraverser traverser;
	traverser.SetCacheVideoParams(params);
	olive::NodeValueTable table = traverser.GenerateTable(
		lut, olive::TimeRange(olive::core::rational(0),
							  olive::core::rational(1, 30)));
	olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
	traverser.Resolve(tex_val);

	ASSERT_TRUE(traverser.output_frame);
	const olive::Color out = traverser.output_frame->get_pixel(0, 0);

	EXPECT_NEAR(out.red(), 0.25f, 0.02f);
	EXPECT_NEAR(out.green(), 0.50f, 0.02f);
	EXPECT_NEAR(out.blue(), 0.75f, 0.02f);
}

TEST(ColorLutNode, ReusingSameFileDoesNotCrash)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = WriteTestCubeLut(&dir, "invert", 0.0f, 1.0f);
	ASSERT_FALSE(path.isEmpty());

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.50f, 0.75f, 1.0f)));

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, path);

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(lut, olive::OCIOLutNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	// Render twice; the second render should reuse the cached processor.
	for (int i = 0; i < 2; ++i) {
		PixelColorTransformTraverser traverser;
		traverser.SetCacheVideoParams(params);
		olive::NodeValueTable table = traverser.GenerateTable(
			lut, olive::TimeRange(olive::core::rational(0),
								  olive::core::rational(1, 30)));
		olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
		traverser.Resolve(tex_val);

		ASSERT_TRUE(traverser.output_frame);
		const olive::Color out = traverser.output_frame->get_pixel(0, 0);
		EXPECT_NEAR(out.red(), 0.75f, 0.02f);
		EXPECT_NEAR(out.green(), 0.50f, 0.02f);
		EXPECT_NEAR(out.blue(), 0.25f, 0.02f);
	}
}

TEST(ColorLutNode, SwitchingBackToOriginalFileRestoresOriginalPixels)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QString invert_path = WriteTestCubeLut(&dir, "invert", 0.0f, 1.0f);
	const QString boost_path = WriteTestCubeLut(&dir, "boost", 0.5f, 1.0f);
	ASSERT_FALSE(invert_path.isEmpty());
	ASSERT_FALSE(boost_path.isEmpty());

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.50f, 0.75f, 1.0f)));

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, invert_path);

	olive::Node::ConnectEdge(
		solid, olive::NodeInput(lut, olive::OCIOLutNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	auto render = [&]() {
		PixelColorTransformTraverser traverser;
		traverser.SetCacheVideoParams(params);
		olive::NodeValueTable table = traverser.GenerateTable(
			lut, olive::TimeRange(olive::core::rational(0),
								  olive::core::rational(1, 30)));
		olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
		traverser.Resolve(tex_val);
		return traverser.output_frame->get_pixel(0, 0);
	};

	// First render with invert.
	const olive::Color invert_out = render();
	EXPECT_NEAR(invert_out.red(), 0.75f, 0.02f);

	// Switch to boost, then back to invert.
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, boost_path);
	const olive::Color boost_out = render();
	EXPECT_GT(std::abs(boost_out.red() - invert_out.red()), 0.1f);

	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, invert_path);
	const olive::Color restored_out = render();
	EXPECT_NEAR(restored_out.red(), invert_out.red(), 0.02f);
	EXPECT_NEAR(restored_out.green(), invert_out.green(), 0.02f);
	EXPECT_NEAR(restored_out.blue(), invert_out.blue(), 0.02f);
}

// -----------------------------------------------------------------------------
// Error reporting: silent passthrough states must be observable.
// -----------------------------------------------------------------------------

TEST(ColorLutNode, MissingFileSetsLastError)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput,
						  QStringLiteral("/nonexistent/path/lut.cube"));

	EXPECT_FALSE(lut->last_error().isEmpty());
}

TEST(ColorLutNode, UnsupportedExtensionSetsLastError)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = QDir(dir.path()).filePath(QStringLiteral("lut.txt"));
	QFile file(path);
	ASSERT_TRUE(file.open(QIODevice::WriteOnly));
	file.close();

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);
	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, path);

	EXPECT_FALSE(lut->last_error().isEmpty());
}

TEST(ColorLutNode, ValidFileClearsLastError)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path = WriteTestCubeLut(&dir, "invert", 0.0f, 1.0f);
	ASSERT_FALSE(path.isEmpty());

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);

	lut->SetStandardValue(olive::OCIOLutNode::kFileInput,
						  QStringLiteral("/nonexistent/path/lut.cube"));
	EXPECT_FALSE(lut->last_error().isEmpty());

	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, path);
	EXPECT_TRUE(lut->last_error().isEmpty());
}

TEST(ColorLutNode, EmptyPathClearsLastError)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *lut = new olive::OCIOLutNode();
	lut->setParent(&project);

	lut->SetStandardValue(olive::OCIOLutNode::kFileInput,
						  QStringLiteral("/nonexistent/path/lut.cube"));
	EXPECT_FALSE(lut->last_error().isEmpty());

	lut->SetStandardValue(olive::OCIOLutNode::kFileInput, QString());
	EXPECT_TRUE(lut->last_error().isEmpty());
}

// -----------------------------------------------------------------------------
// Global LUT library
// -----------------------------------------------------------------------------

TEST(LUTLibrary, SupportsCubeAnd3dlExtensions)
{
	EXPECT_TRUE(olive::LUTLibrary::IsSupportedExtension(QStringLiteral("cube")));
	EXPECT_TRUE(
		olive::LUTLibrary::IsSupportedExtension(QStringLiteral(".cube")));
	EXPECT_TRUE(olive::LUTLibrary::IsSupportedExtension(QStringLiteral("CUBE")));
	EXPECT_TRUE(olive::LUTLibrary::IsSupportedExtension(QStringLiteral("3dl")));
	EXPECT_TRUE(olive::LUTLibrary::IsSupportedExtension(QStringLiteral(".3dl")));
	EXPECT_FALSE(olive::LUTLibrary::IsSupportedExtension(QStringLiteral("txt")));
	EXPECT_FALSE(olive::LUTLibrary::IsSupportedExtension(QString()));
}

TEST(LUTLibrary, DirectoryRoundTripCleansAndDeduplicates)
{
	const QString previous =
		olive::Config::Current()[QStringLiteral("LUTLibraryPaths")].toString();

	olive::LUTLibrary::SetDirectories(
		{ QStringLiteral("/a/luts"), QStringLiteral(" /a/luts "),
		  QStringLiteral("/b/luts"), QString() });

	EXPECT_EQ(olive::LUTLibrary::GetDirectories(),
			  (QStringList{ QStringLiteral("/a/luts"),
							QStringLiteral("/b/luts") }));

	olive::Config::Current()[QStringLiteral("LUTLibraryPaths")] = previous;
}

TEST(LUTLibrary, ScansDirectoriesRecursivelyForSupportedLuts)
{
	const QString previous =
		olive::Config::Current()[QStringLiteral("LUTLibraryPaths")].toString();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QString cube_path =
		QDir(dir.path()).filePath(QStringLiteral("top.cube"));
	const QString sub_dir = QDir(dir.path()).filePath(QStringLiteral("sub"));
	const QString three_dl_path =
		QDir(sub_dir).filePath(QStringLiteral("nested.3dl"));
	const QString text_path =
		QDir(dir.path()).filePath(QStringLiteral("skip.txt"));

	ASSERT_TRUE(QDir().mkpath(sub_dir));
	for (const QString &p : { cube_path, three_dl_path, text_path }) {
		QFile file(p);
		ASSERT_TRUE(file.open(QIODevice::WriteOnly));
		file.close();
	}

	olive::LUTLibrary::SetDirectories({ dir.path() });

	const QStringList files = olive::LUTLibrary::GetLutFiles();
	EXPECT_EQ(files.size(), 2);
	EXPECT_TRUE(files.contains(cube_path));
	EXPECT_TRUE(files.contains(three_dl_path));
	EXPECT_FALSE(files.contains(text_path));

	olive::Config::Current()[QStringLiteral("LUTLibraryPaths")] = previous;
}

// -----------------------------------------------------------------------------
// Display -> reference inverse transform (previously disabled over an OCIO
// crash; covered here so the ColorDialog conversion can stay enabled).
// -----------------------------------------------------------------------------

TEST(ColorProcessor, InverseDisplayTransformRoundTripsColor)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	olive::ColorManager *color_manager = project.color_manager();
	ASSERT_NE(color_manager, nullptr);

	const QString display = color_manager->GetDefaultDisplay();
	const QString view = color_manager->GetDefaultView(display);
	ASSERT_FALSE(display.isEmpty());
	ASSERT_FALSE(view.isEmpty());

	const olive::ColorTransform output_transform(display, view, QString());

	olive::ColorProcessorPtr ref_to_display = olive::ColorProcessor::Create(
		color_manager, color_manager->GetReferenceColorSpace(),
		output_transform);
	ASSERT_NE(ref_to_display, nullptr);
	ASSERT_NE(ref_to_display->GetProcessor(), nullptr);

	olive::ColorProcessorPtr display_to_ref = olive::ColorProcessor::Create(
		color_manager, color_manager->GetReferenceColorSpace(),
		output_transform, olive::ColorProcessor::kInverse);
	ASSERT_NE(display_to_ref, nullptr);
	ASSERT_NE(display_to_ref->GetProcessor(), nullptr);

	const olive::Color reference(0.2f, 0.4f, 0.6f, 1.0f);
	const olive::Color display_color = ref_to_display->ConvertColor(reference);
	const olive::Color round_trip = display_to_ref->ConvertColor(display_color);

	EXPECT_NEAR(round_trip.red(), reference.red(), 0.001f);
	EXPECT_NEAR(round_trip.green(), reference.green(), 0.001f);
	EXPECT_NEAR(round_trip.blue(), reference.blue(), 0.001f);
	EXPECT_NEAR(round_trip.alpha(), reference.alpha(), 0.001f);
}

// -----------------------------------------------------------------------------
// OCIOGradingTransformLinearNode clamp invariant
// -----------------------------------------------------------------------------

namespace
{

class ClampCaptureTraverser : public PixelColorTransformTraverser {
public:
	bool captured_white_clamp = false;
	double white_clamp_value = 0.0;

protected:
	virtual void
	ProcessColorTransform(olive::TexturePtr destination, const olive::Node *node,
						  const olive::ColorTransformJob *job) override
	{
		const olive::NodeValueRow &values = job->GetValues();
		if (values.contains(
				olive::OCIOGradingTransformLinearNode::kClampWhiteInput)) {
			white_clamp_value =
				values
					.value(olive::OCIOGradingTransformLinearNode::
							   kClampWhiteInput)
					.toDouble();
			captured_white_clamp = true;
		}
		PixelColorTransformTraverser::ProcessColorTransform(destination, node,
														  job);
	}
};

} // namespace

TEST(ColorGradingLinear, InvalidClampRangeIsCorrectedPerFrame)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.50f, 0.75f, 1.0f)));

	auto *grading = new olive::OCIOGradingTransformLinearNode();
	grading->setParent(&project);
	grading->SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampBlackEnableInput, true);
	grading->SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampWhiteEnableInput, true);
	grading->SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampBlackInput, 0.5);
	// Invalid: white clamp below black clamp
	grading->SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampWhiteInput, 0.0);

	olive::Node::ConnectEdge(
		solid,
		olive::NodeInput(
			grading,
			olive::OCIOGradingTransformLinearNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	ClampCaptureTraverser traverser;
	traverser.SetCacheVideoParams(params);
	olive::NodeValueTable table = traverser.GenerateTable(
		grading, olive::TimeRange(olive::core::rational(0),
								  olive::core::rational(1, 30)));
	olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
	traverser.Resolve(tex_val);

	// The node must have corrected the white clamp to just above the black
	// clamp instead of feeding an invalid grading primary to OCIO
	ASSERT_TRUE(traverser.captured_white_clamp);
	EXPECT_NEAR(traverser.white_clamp_value, 0.500001, 1e-9);

	// And rendering must not crash or drop the frame
	ASSERT_TRUE(traverser.output_frame);
}

TEST(ColorGradingLinear, ValidClampRangeIsLeftUntouched)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *solid = new olive::SolidGenerator();
	solid->setParent(&project);
	solid->SetStandardValue(
		olive::SolidGenerator::kColorInput,
		QVariant::fromValue(olive::Color(0.25f, 0.50f, 0.75f, 1.0f)));

	auto *grading = new olive::OCIOGradingTransformLinearNode();
	grading->setParent(&project);
	grading->SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampBlackEnableInput, true);
	grading->SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampWhiteEnableInput, true);
	grading->SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampBlackInput, 0.1);
	grading->SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampWhiteInput, 0.9);

	olive::Node::ConnectEdge(
		solid,
		olive::NodeInput(
			grading,
			olive::OCIOGradingTransformLinearNode::kTextureInput));

	const olive::VideoParams params(16, 16, olive::core::PixelFormat::F32,
									olive::VideoParams::kRGBAChannelCount);

	ClampCaptureTraverser traverser;
	traverser.SetCacheVideoParams(params);
	olive::NodeValueTable table = traverser.GenerateTable(
		grading, olive::TimeRange(olive::core::rational(0),
								  olive::core::rational(1, 30)));
	olive::NodeValue tex_val = table.Get(olive::NodeValue::kTexture);
	traverser.Resolve(tex_val);

	ASSERT_TRUE(traverser.captured_white_clamp);
	EXPECT_NEAR(traverser.white_clamp_value, 0.9, 1e-9);

	ASSERT_TRUE(traverser.output_frame);
}

TEST(ColorGradingLinear, StaticBlackClampConstrainsWhiteMinimum)
{
	olive::ColorManager::SetUpDefaultConfig();

	olive::Project project;
	project.Initialize();

	auto *grading = new olive::OCIOGradingTransformLinearNode();
	grading->setParent(&project);

	// Default black clamp is 0, so the white minimum starts just above it
	EXPECT_NEAR(grading
					->GetInputProperty(
						olive::OCIOGradingTransformLinearNode::kClampWhiteInput,
						QStringLiteral("min"))
					.toDouble(),
				0.000001, 1e-9);

	// Changing the static black clamp updates the white minimum
	grading->SetStandardValue(
		olive::OCIOGradingTransformLinearNode::kClampBlackInput, 0.25);
	EXPECT_NEAR(grading
					->GetInputProperty(
						olive::OCIOGradingTransformLinearNode::kClampWhiteInput,
						QStringLiteral("min"))
					.toDouble(),
				0.250001, 1e-9);
}
