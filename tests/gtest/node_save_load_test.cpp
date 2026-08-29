#include <gtest/gtest.h>

#include <memory>

#include <QPointF>
#include <QString>
#include <QUuid>
#include <QVector>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "common/xmlutils.h"
#include "core.h"
#include "node/color/colormanager/colormanager.h"
#include "node/factory.h"
#include "node/generator/solid/solid.h"
#include "node/generator/text/textv3.h"
#include "node/keyframe.h"
#include "node/keying/chromakey/chromakey.h"
#include "node/math/math/math.h"
#include "node/node.h"
#include "node/project.h"
#include "node/project/folder/folder.h"
#include "node/serializeddata.h"
#include "node/splitvalue.h"
#include "render/diskmanager.h"

namespace
{

// Serializes a single node into a standalone XML document, mirroring how
// Project::Save wraps Node::Save in a "node" element
QString SaveNodeXml(const olive::Node *node)
{
	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("node"));
	node->Save(&writer);
	writer.writeEndElement(); // node
	writer.writeEndDocument();
	return xml;
}

// Loads a document produced by SaveNodeXml into an existing node
bool LoadNodeXml(olive::Node *node, const QString &xml,
				 olive::SerializedData *data)
{
	QXmlStreamReader reader(xml);
	if (!reader.readNextStartElement()) {
		return false;
	}
	if (reader.name() != QStringLiteral("node")) {
		return false;
	}
	return node->Load(&reader, data);
}

olive::Node *FindNodeById(olive::Project *project, const QString &id)
{
	for (olive::Node *n : project->nodes()) {
		if (n->id() == id) {
			return n;
		}
	}
	return nullptr;
}

// Node that round-trips a custom payload through SaveCustom/LoadCustom and
// records LoadFinishedEvent
class CustomDataNode : public olive::Node {
public:
	CustomDataNode()
	{
		AddInput(QStringLiteral("Value"), olive::NodeValue::kFloat);
	}

	NODE_DEFAULT_FUNCTIONS(CustomDataNode)

	virtual QString Name() const override
	{
		return QStringLiteral("CustomDataNode");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.customdatanode");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return { kCategoryUnknown };
	}

	virtual QString Description() const override
	{
		return QStringLiteral("Node with custom serialized data");
	}

	void Value(const olive::NodeValueRow &, const olive::NodeGlobals &,
			   olive::NodeValueTable *) const override
	{
	}

	virtual void SaveCustom(QXmlStreamWriter *writer) const override
	{
		writer->writeTextElement(QStringLiteral("greeting"), greeting_);
	}

	virtual bool LoadCustom(QXmlStreamReader *reader,
							olive::SerializedData *data) override
	{
		Q_UNUSED(data)

		while (olive::XMLReadNextStartElement(reader)) {
			if (reader->name() == QStringLiteral("greeting")) {
				greeting_ = reader->readElementText();
			} else if (reader->name() == QStringLiteral("explode")) {
				reader->skipCurrentElement();
				return false;
			} else {
				reader->skipCurrentElement();
			}
		}

		return true;
	}

	virtual void LoadFinishedEvent() override
	{
		load_finished_called_ = true;
	}

	QString greeting_;
	bool load_finished_called_ = false;
};

} // namespace

class NodeSaveLoadTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		olive::ColorManager::SetUpDefaultConfig();

		// Cache UUID changes resolve a cache path through the DiskManager
		// singleton, which itself touches Core (same pattern as
		// project_factory_test)
		if (!olive::Core::instance()) {
			new olive::Core(olive::Core::CoreParams()); // intentionally leaked
		}
		if (!olive::DiskManager::instance()) {
			olive::DiskManager::CreateInstance();
		}

		project_ = std::make_unique<olive::Project>();
		project_->Initialize();
	}

	template <typename T> T *AddNode()
	{
		T *node = new T();
		node->setParent(project_.get());
		return node;
	}

	std::unique_ptr<olive::Project> project_;
};

TEST_F(NodeSaveLoadTest, StandardValuesLabelAndColorRoundTrip)
{
	auto *src = AddNode<olive::MathNode>();
	src->SetLabel(QStringLiteral("Labeled"));
	src->SetOverrideColor(3);
	src->SetStandardValue(olive::MathNode::kParamAIn, 3.5);
	src->SetStandardValue(olive::MathNode::kParamBIn, -2.25);
	src->SetOperation(olive::MathNode::kOpMultiply);

	const QString xml = SaveNodeXml(src);
	EXPECT_TRUE(xml.contains(QStringLiteral("version=\"1\"")));
	EXPECT_TRUE(xml.contains(
		QStringLiteral("id=\"org.olivevideoeditor.Olive.math\"")));

	olive::MathNode loaded;
	olive::SerializedData data;
	ASSERT_TRUE(LoadNodeXml(&loaded, xml, &data));

	EXPECT_EQ(loaded.GetLabel(), QStringLiteral("Labeled"));
	EXPECT_EQ(loaded.GetOverrideColor(), 3);
	EXPECT_DOUBLE_EQ(
		loaded.GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 3.5);
	EXPECT_DOUBLE_EQ(
		loaded.GetStandardValue(olive::MathNode::kParamBIn).toDouble(), -2.25);
	EXPECT_EQ(int(loaded.GetOperation()), int(olive::MathNode::kOpMultiply));

	// The "ptr" attribute maps the serialized address to the loaded instance
	EXPECT_EQ(data.node_ptrs.value(reinterpret_cast<quintptr>(src)), &loaded);

	// A non-keyframable input never reports keyframing after load
	EXPECT_FALSE(loaded.IsInputKeyframing(olive::MathNode::kMethodIn));
}

TEST_F(NodeSaveLoadTest, ArrayElementsAndPerElementKeyframingRoundTrip)
{
	auto *src = AddNode<olive::TextGeneratorV3>();
	src->InputArrayResize(olive::TextGeneratorV3::kArgsInput, 2);
	src->SetStandardValue(
		olive::NodeInput(src, olive::TextGeneratorV3::kArgsInput, 0),
		QStringLiteral("first"));
	src->SetStandardValue(
		olive::NodeInput(src, olive::TextGeneratorV3::kArgsInput, 1),
		QStringLiteral("second"));

	// Only element 1 is keyframed
	src->SetInputIsKeyframing(olive::TextGeneratorV3::kArgsInput, true, 1);
	auto *key = new olive::NodeKeyframe(
		olive::rational(2), QStringLiteral("keyed"), olive::NodeKeyframe::kLinear,
		0, 1, olive::TextGeneratorV3::kArgsInput);
	key->setParent(src);

	const QString xml = SaveNodeXml(src);

	olive::TextGeneratorV3 loaded;
	olive::SerializedData data;
	ASSERT_TRUE(LoadNodeXml(&loaded, xml, &data));

	// The subelement count attribute resized the array on load
	ASSERT_EQ(loaded.InputArraySize(olive::TextGeneratorV3::kArgsInput), 2);
	EXPECT_EQ(loaded.GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput, 0)
				  .at(0)
				  .toString(),
			  QStringLiteral("first"));
	EXPECT_EQ(loaded.GetSplitStandardValue(olive::TextGeneratorV3::kArgsInput, 1)
				  .at(0)
				  .toString(),
			  QStringLiteral("second"));

	EXPECT_FALSE(
		loaded.IsInputKeyframing(olive::TextGeneratorV3::kArgsInput, 0));
	EXPECT_TRUE(loaded.IsInputKeyframing(olive::TextGeneratorV3::kArgsInput, 1));

	const QVector<olive::NodeKeyframeTrack> &tracks =
		loaded.GetKeyframeTracks(olive::TextGeneratorV3::kArgsInput, 1);
	ASSERT_EQ(tracks.at(0).size(), 1);
	EXPECT_EQ(tracks.at(0).first()->time(), olive::rational(2));
	EXPECT_EQ(tracks.at(0).first()->value().toString(),
			  QStringLiteral("keyed"));
	EXPECT_EQ(tracks.at(0).first()->element(), 1);
}

TEST_F(NodeSaveLoadTest, KeyframesAllTypesAndColorPropertiesRoundTrip)
{
	auto *src = AddNode<olive::SolidGenerator>();

	olive::SplitValue color;
	color.append(0.25);
	color.append(0.5);
	color.append(0.75);
	color.append(1.0);
	src->SetSplitStandardValue(olive::SolidGenerator::kColorInput, color, -1);

	src->SetInputIsKeyframing(olive::SolidGenerator::kColorInput, true);

	auto *linear = new olive::NodeKeyframe(
		olive::rational(0), 0.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::SolidGenerator::kColorInput);
	linear->setParent(src);
	auto *bezier = new olive::NodeKeyframe(
		olive::rational(5), 1.0, olive::NodeKeyframe::kBezier, 0, -1,
		olive::SolidGenerator::kColorInput);
	bezier->setParent(src);
	bezier->set_bezier_control_in(QPointF(0.25, -1.5));
	bezier->set_bezier_control_out(QPointF(2.5, 0.75));
	auto *hold = new olive::NodeKeyframe(
		olive::rational(3), 0.5, olive::NodeKeyframe::kHold, 2, -1,
		olive::SolidGenerator::kColorInput);
	hold->setParent(src);

	// Color inputs additionally serialize their color management properties
	src->SetInputProperty(olive::SolidGenerator::kColorInput,
						  QStringLiteral("col_input"), QStringLiteral("ACEScg"));
	src->SetInputProperty(olive::SolidGenerator::kColorInput,
						  QStringLiteral("col_display"), QStringLiteral("sRGB"));
	src->SetInputProperty(olive::SolidGenerator::kColorInput,
						  QStringLiteral("col_view"), QStringLiteral("Filmic"));
	src->SetInputProperty(olive::SolidGenerator::kColorInput,
						  QStringLiteral("col_look"), QStringLiteral("None"));

	const QString xml = SaveNodeXml(src);

	olive::SolidGenerator loaded;
	olive::SerializedData data;
	ASSERT_TRUE(LoadNodeXml(&loaded, xml, &data));

	EXPECT_TRUE(loaded.IsInputKeyframing(olive::SolidGenerator::kColorInput));

	const QVector<olive::NodeKeyframeTrack> &tracks =
		loaded.GetKeyframeTracks(olive::SolidGenerator::kColorInput, -1);
	ASSERT_EQ(tracks.size(), 4);

	// Track 0 holds the linear and bezier keys, sorted by time
	ASSERT_EQ(tracks.at(0).size(), 2);
	EXPECT_EQ(tracks.at(0).at(0)->time(), olive::rational(0));
	EXPECT_EQ(tracks.at(0).at(0)->type(), olive::NodeKeyframe::kLinear);
	EXPECT_DOUBLE_EQ(tracks.at(0).at(0)->value().toDouble(), 0.0);
	EXPECT_EQ(tracks.at(0).at(1)->time(), olive::rational(5));
	EXPECT_EQ(tracks.at(0).at(1)->type(), olive::NodeKeyframe::kBezier);
	EXPECT_DOUBLE_EQ(tracks.at(0).at(1)->value().toDouble(), 1.0);
	EXPECT_DOUBLE_EQ(tracks.at(0).at(1)->bezier_control_in().x(), 0.25);
	EXPECT_DOUBLE_EQ(tracks.at(0).at(1)->bezier_control_in().y(), -1.5);
	EXPECT_DOUBLE_EQ(tracks.at(0).at(1)->bezier_control_out().x(), 2.5);
	EXPECT_DOUBLE_EQ(tracks.at(0).at(1)->bezier_control_out().y(), 0.75);

	// Track 1 was left empty, track 2 holds the single hold key
	EXPECT_TRUE(tracks.at(1).isEmpty());
	ASSERT_EQ(tracks.at(2).size(), 1);
	EXPECT_EQ(tracks.at(2).first()->time(), olive::rational(3));
	EXPECT_EQ(tracks.at(2).first()->type(), olive::NodeKeyframe::kHold);
	EXPECT_DOUBLE_EQ(tracks.at(2).first()->value().toDouble(), 0.5);
	EXPECT_TRUE(tracks.at(3).isEmpty());

	// The per-track standard values survive as well
	const olive::SplitValue loaded_color =
		loaded.GetSplitStandardValue(olive::SolidGenerator::kColorInput, -1);
	ASSERT_EQ(loaded_color.size(), 4);
	EXPECT_DOUBLE_EQ(loaded_color.at(0).toDouble(), 0.25);
	EXPECT_DOUBLE_EQ(loaded_color.at(1).toDouble(), 0.5);
	EXPECT_DOUBLE_EQ(loaded_color.at(2).toDouble(), 0.75);
	EXPECT_DOUBLE_EQ(loaded_color.at(3).toDouble(), 1.0);

	EXPECT_EQ(loaded.GetInputProperty(olive::SolidGenerator::kColorInput,
									  QStringLiteral("col_input"))
				  .toString(),
			  QStringLiteral("ACEScg"));
	EXPECT_EQ(loaded.GetInputProperty(olive::SolidGenerator::kColorInput,
									  QStringLiteral("col_display"))
				  .toString(),
			  QStringLiteral("sRGB"));
	EXPECT_EQ(loaded.GetInputProperty(olive::SolidGenerator::kColorInput,
									  QStringLiteral("col_view"))
				  .toString(),
			  QStringLiteral("Filmic"));
	EXPECT_EQ(loaded.GetInputProperty(olive::SolidGenerator::kColorInput,
									  QStringLiteral("col_look"))
				  .toString(),
			  QStringLiteral("None"));
}

TEST_F(NodeSaveLoadTest, ValueHintsRoundTrip)
{
	auto *src = AddNode<olive::MathNode>();
	src->SetValueHintForInput(
		olive::MathNode::kParamAIn,
		olive::Node::ValueHint(
			{ olive::NodeValue::kVec2, olive::NodeValue::kTexture }, 3,
			QStringLiteral("tag")));
	src->SetValueHintForInput(olive::MathNode::kParamBIn,
							  olive::Node::ValueHint(QStringLiteral("elem")), 2);

	const QString xml = SaveNodeXml(src);

	olive::MathNode loaded;
	olive::SerializedData data;
	ASSERT_TRUE(LoadNodeXml(&loaded, xml, &data));

	const olive::Node::ValueHint hint =
		loaded.GetValueHintForInput(olive::MathNode::kParamAIn);
	ASSERT_EQ(hint.types().size(), 2);
	EXPECT_EQ(hint.types().at(0), olive::NodeValue::kVec2);
	EXPECT_EQ(hint.types().at(1), olive::NodeValue::kTexture);
	EXPECT_EQ(hint.index(), 3);
	EXPECT_EQ(hint.tag(), QStringLiteral("tag"));

	// Hints are tracked per element
	EXPECT_EQ(loaded.GetValueHintForInput(olive::MathNode::kParamBIn, 2).tag(),
			  QStringLiteral("elem"));
	EXPECT_EQ(loaded.GetValueHintForInput(olive::MathNode::kParamBIn, 1).tag(),
			  QString());
	EXPECT_EQ(loaded.GetValueHints().size(), 2);
}

TEST_F(NodeSaveLoadTest, CacheUuidsRoundTrip)
{
	olive::MathNode src;

	const QUuid audio_uuid(
		QStringLiteral("{11111111-1111-1111-1111-111111111111}"));
	const QUuid video_uuid(
		QStringLiteral("{22222222-2222-2222-2222-222222222222}"));
	const QUuid thumb_uuid(
		QStringLiteral("{33333333-3333-3333-3333-333333333333}"));
	const QUuid waveform_uuid(
		QStringLiteral("{44444444-4444-4444-4444-444444444444}"));

	src.audio_playback_cache()->SetUuid(audio_uuid);
	src.video_frame_cache()->SetUuid(video_uuid);
	src.thumbnail_cache()->SetUuid(thumb_uuid);
	src.waveform_cache()->SetUuid(waveform_uuid);

	const QString xml = SaveNodeXml(&src);

	olive::MathNode loaded;
	olive::SerializedData data;
	ASSERT_TRUE(LoadNodeXml(&loaded, xml, &data));

	EXPECT_EQ(loaded.audio_playback_cache()->GetUuid(), audio_uuid);
	EXPECT_EQ(loaded.video_frame_cache()->GetUuid(), video_uuid);
	EXPECT_EQ(loaded.thumbnail_cache()->GetUuid(), thumb_uuid);
	EXPECT_EQ(loaded.waveform_cache()->GetUuid(), waveform_uuid);
}

TEST_F(NodeSaveLoadTest, CustomDataAndLoadFinishedEventRoundTrip)
{
	CustomDataNode src;
	src.greeting_ = QStringLiteral("hello custom");

	const QString xml = SaveNodeXml(&src);
	EXPECT_TRUE(xml.contains(QStringLiteral("hello custom")));

	CustomDataNode loaded;
	olive::SerializedData data;
	ASSERT_TRUE(LoadNodeXml(&loaded, xml, &data));

	EXPECT_EQ(loaded.greeting_, QStringLiteral("hello custom"));
	EXPECT_TRUE(loaded.load_finished_called_);

	// A LoadCustom failure propagates out of Node::Load
	const QString fail_xml = QStringLiteral(
		"<node><custom><explode/></custom></node>");
	CustomDataNode failing;
	olive::SerializedData fail_data;
	QXmlStreamReader reader(fail_xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("node"));
	EXPECT_FALSE(failing.Load(&reader, &fail_data));
}

TEST_F(NodeSaveLoadTest, UnknownElementsAndVersionAreSkipped)
{
	olive::MathNode node;

	// Unknown elements are skipped at every level of the node format, and an
	// unrecognized version attribute does not fail the load
	const QString xml = QStringLiteral(
		"<node version=\"999\" id=\"org.olivevideoeditor.Olive.math\">"
		"<mystery><nested attr=\"1\"/></mystery>"
		"<label>kept</label>"
		"<links><strange/></links>"
		"<connections>"
		"<strange/>"
		"<connection input=\"param_a_in\" element=\"-1\">"
		"<weird/>"
		"<output>12345</output>"
		"</connection>"
		"</connections>"
		"<hints><strange/></hints>"
		"<context><strange/></context>"
		"<caches>"
		"<mysterycache>{00000000-0000-0000-0000-000000000000}</mysterycache>"
		"</caches>"
		"<input id=\"param_a_in\"><oddstandard/></input>"
		"</node>");

	olive::SerializedData data;
	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("node"));
	EXPECT_TRUE(node.Load(&reader, &data));

	EXPECT_EQ(node.GetLabel(), QStringLiteral("kept"));

	// The one well-formed connection was still recorded
	ASSERT_EQ(data.desired_connections.size(), 1);
	EXPECT_EQ(data.desired_connections.first().input.input(),
			  olive::MathNode::kParamAIn);
	EXPECT_EQ(data.desired_connections.first().input.element(), -1);
	EXPECT_EQ(data.desired_connections.first().output_node, quintptr(12345));

	// The malformed input left the default value untouched
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 0.0);
}

TEST_F(NodeSaveLoadTest, LoadInputWithMissingOrUnknownIdIsSkipped)
{
	olive::MathNode node;

	// An input with no id and an input whose id does not exist on the node
	// both make LoadInput fail internally, but Node::Load ignores that return
	// value and carries on
	const QString xml = QStringLiteral(
		"<node>"
		"<input><primary><standard><track>9</track></standard></primary></input>"
		"<input id=\"no_such_input\">"
		"<primary><standard><track>9</track></standard></primary>"
		"</input>"
		"</node>");

	olive::SerializedData data;
	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("node"));
	EXPECT_TRUE(node.Load(&reader, &data));

	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::MathNode::kParamAIn).toDouble(), 0.0);
	EXPECT_DOUBLE_EQ(
		node.GetStandardValue(olive::MathNode::kParamBIn).toDouble(), 0.0);
}

TEST_F(NodeSaveLoadTest, ConnectionsLinksAndPositionsResolveAfterProjectLoad)
{
	olive::NodeFactory::Initialize();

	auto *src = AddNode<olive::SolidGenerator>();
	auto *dst = AddNode<olive::MathNode>();
	auto *text = AddNode<olive::TextGeneratorV3>();
	text->InputArrayResize(olive::TextGeneratorV3::kArgsInput, 2);

	olive::Node::ConnectEdge(
		src, olive::NodeInput(dst, olive::MathNode::kParamAIn));
	olive::Node::ConnectEdge(
		dst, olive::NodeInput(text, olive::TextGeneratorV3::kArgsInput, 1));
	olive::Node::Link(src, dst);

	olive::Folder *root = project_->root();
	root->SetNodePositionInContext(
		src, olive::Node::Position(QPointF(10.0, 20.0), true));
	root->SetNodePositionInContext(
		dst, olive::Node::Position(QPointF(-3.5, 7.25), false));

	QString xml;
	QXmlStreamWriter writer(&xml);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("project"));
	project_->Save(&writer);
	writer.writeEndElement(); // project
	writer.writeEndDocument();

	// The project being loaded into must not be Initialize()d: Load()
	// re-resolves the root folder from the saved settings
	olive::Project loaded;
	olive::SerializedData data;
	{
		QXmlStreamReader reader(xml);
		ASSERT_TRUE(reader.readNextStartElement());
		ASSERT_EQ(reader.name(), QStringLiteral("project"));
		data = loaded.Load(&reader);
	}

	// Root folder plus the three nodes created above
	ASSERT_EQ(loaded.nodes().size(), 4);

	olive::Node *loaded_src = FindNodeById(&loaded, src->id());
	olive::Node *loaded_dst = FindNodeById(&loaded, dst->id());
	olive::Node *loaded_text = FindNodeById(&loaded, text->id());
	ASSERT_NE(loaded_src, nullptr);
	ASSERT_NE(loaded_dst, nullptr);
	ASSERT_NE(loaded_text, nullptr);

	// Both edges were recorded against the serialized addresses, including
	// the array element index on the text input
	ASSERT_EQ(data.desired_connections.size(), 2);
	bool found_math_edge = false;
	bool found_text_edge = false;
	for (const auto &sc : data.desired_connections) {
		if (sc.input.node() == loaded_dst) {
			EXPECT_EQ(sc.input.input(), olive::MathNode::kParamAIn);
			EXPECT_EQ(sc.input.element(), -1);
			EXPECT_EQ(sc.output_node, reinterpret_cast<quintptr>(src));
			found_math_edge = true;
		} else if (sc.input.node() == loaded_text) {
			EXPECT_EQ(sc.input.input(), olive::TextGeneratorV3::kArgsInput);
			EXPECT_EQ(sc.input.element(), 1);
			EXPECT_EQ(sc.output_node, reinterpret_cast<quintptr>(dst));
			found_text_edge = true;
		}
	}
	EXPECT_TRUE(found_math_edge);
	EXPECT_TRUE(found_text_edge);

	// Both nodes wrote their side of the link
	EXPECT_EQ(data.block_links.size(), 2);

	// The root folder recorded positions for the two placed nodes
	olive::Folder *loaded_root = loaded.root();
	ASSERT_NE(loaded_root, nullptr);
	EXPECT_EQ(data.positions.value(loaded_root).size(), 2);

	// Resolve the deferred state the same way
	// ProjectSerializer230220::PostConnect does
	for (const auto &sc : data.desired_connections) {
		if (olive::Node *out = data.node_ptrs.value(sc.output_node)) {
			olive::Node::ConnectEdge(out, sc.input);
		}
	}
	for (const auto &link : data.block_links) {
		olive::Node::Link(link.block, data.node_ptrs.value(link.link));
	}
	for (olive::Node *n : loaded.nodes()) {
		n->PostLoadEvent(&data);
	}

	EXPECT_EQ(loaded_dst->GetConnectedOutput(olive::MathNode::kParamAIn),
			  loaded_src);
	EXPECT_EQ(loaded_text->GetConnectedOutput(
				  olive::TextGeneratorV3::kArgsInput, 1),
			  loaded_dst);
	EXPECT_TRUE(olive::Node::AreLinked(loaded_src, loaded_dst));
	EXPECT_TRUE(olive::Node::AreLinked(loaded_dst, loaded_src));

	EXPECT_EQ(loaded_root->GetNodePositionInContext(loaded_src),
			  QPointF(10.0, 20.0));
	EXPECT_TRUE(loaded_root->IsNodeExpandedInContext(loaded_src));
	EXPECT_EQ(loaded_root->GetNodePositionInContext(loaded_dst),
			  QPointF(-3.5, 7.25));
	EXPECT_FALSE(loaded_root->IsNodeExpandedInContext(loaded_dst));

	olive::NodeFactory::Destroy();
}

TEST_F(NodeSaveLoadTest, LegacyMisspelledChromaKeyIDsAreMapped)
{
	auto *src = AddNode<olive::ChromaKeyNode>();
	src->SetStandardValue(olive::ChromaKeyNode::kUpperToleranceInput, 42.0);
	src->SetStandardValue(olive::ChromaKeyNode::kLowerToleranceInput, 7.0);

	auto *math = AddNode<olive::MathNode>();
	olive::Node::ConnectEdge(
		math, olive::NodeInput(src, olive::ChromaKeyNode::kUpperToleranceInput));

	QString xml = SaveNodeXml(src);

	// Simulate an old project file written with the misspelled "tolerence" IDs
	xml.replace(QStringLiteral("upper_tolerance_in"),
				QStringLiteral("upper_tolerence_in"));
	xml.replace(QStringLiteral("lower_tolerance_in"),
				QStringLiteral("lower_tolerence_in"));

	olive::ChromaKeyNode loaded;
	olive::SerializedData data;
	ASSERT_TRUE(LoadNodeXml(&loaded, xml, &data));

	EXPECT_DOUBLE_EQ(
		loaded.GetStandardValue(olive::ChromaKeyNode::kUpperToleranceInput)
			.toDouble(),
		42.0);
	EXPECT_DOUBLE_EQ(
		loaded.GetStandardValue(olive::ChromaKeyNode::kLowerToleranceInput)
			.toDouble(),
		7.0);

	// Connections to the renamed inputs are remapped too
	ASSERT_EQ(data.desired_connections.size(), 1);
	EXPECT_EQ(data.desired_connections.first().input.input(),
			  olive::ChromaKeyNode::kUpperToleranceInput);
}
