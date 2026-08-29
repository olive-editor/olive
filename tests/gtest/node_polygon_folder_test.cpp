#include <gtest/gtest.h>

#include <QBuffer>
#include <QPointF>
#include <QVector2D>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core.h"
#include "codec/frame.h"
#include "node/color/colormanager/colormanager.h"
#include "node/factory.h"
#include "node/generator/polygon/polygon.h"
#include "node/generator/shape/generatorwithmerge.h"
#include "node/generator/shape/shapenodebase.h"
#include "node/generator/solid/solid.h"
#include "node/generator/text/textv1.h"
#include "node/generator/text/textv2.h"
#include "node/gizmo/line.h"
#include "node/gizmo/path.h"
#include "node/gizmo/point.h"
#include "node/globals.h"
#include "node/nodeundo.h"
#include "node/project.h"
#include "node/project/folder/folder.h"
#include "node/project/serializer/serializer.h"
#include "node/traverser.h"
#include "olive/core/util/color.h"
#include "render/diskmanager.h"
#include "render/job/generatejob.h"
#include "render/loopmode.h"
#include "render/texture.h"

namespace
{

// Node that pushes a fixed dummy texture, used to feed the base input of
// merge-capable generators without any renderer.
class ConstantTextureNode : public olive::Node {
public:
	ConstantTextureNode() = default;

	NODE_DEFAULT_FUNCTIONS(ConstantTextureNode)

	virtual QString Name() const override
	{
		return QStringLiteral("Test Texture");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.constant_texture");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return { kCategoryGenerator };
	}

	void SetTexture(const olive::TexturePtr &texture)
	{
		texture_ = texture;
	}

	virtual void Value(const olive::NodeValueRow &value,
					   const olive::NodeGlobals &globals,
					   olive::NodeValueTable *table) const override
	{
		Q_UNUSED(value)
		Q_UNUSED(globals)

		table->Push(olive::NodeValue(olive::NodeValue::kTexture, texture_, this));
	}

private:
	olive::TexturePtr texture_;
};

template <typename T> T *AddNode(olive::Project *project)
{
	T *node = new T();
	node->setParent(project);
	return node;
}

olive::TimeRange FirstFrame()
{
	return olive::TimeRange(olive::rational(0), olive::rational(1, 30));
}

// A fresh traverser per call: NodeTraverser caches tables per node/range, so
// reusing one would return stale results after changing standard values.
olive::NodeValueTable GenerateTable(const olive::Node *node,
									const olive::VideoParams &vparams)
{
	olive::NodeTraverser traverser;
	traverser.SetCacheVideoParams(vparams);
	return traverser.GenerateTable(node, FirstFrame());
}

olive::NodeValueRow GenerateRow(const olive::Node *node)
{
	olive::NodeTraverser traverser;
	return traverser.GenerateRow(node, FirstFrame());
}

olive::TexturePtr GetOutputTexture(const olive::NodeValueTable &table)
{
	return table.Get(olive::NodeValue::kTexture).toTexture();
}

// Project save/load touches the DiskManager singleton, which itself touches Core
void EnsureAppSingletons()
{
	if (!olive::Core::instance()) {
		new olive::Core(olive::Core::CoreParams()); // intentionally leaked
	}
	if (!olive::DiskManager::instance()) {
		olive::DiskManager::CreateInstance();
	}
}

} // namespace

TEST(Folder, MetadataAndChildInputDefinition)
{
	olive::Folder folder;
	EXPECT_EQ(folder.id(), QStringLiteral("org.olivevideoeditor.Olive.folder"));
	EXPECT_EQ(folder.Name(), QStringLiteral("Folder"));
	EXPECT_FALSE(folder.Description().isEmpty());
	EXPECT_TRUE(folder.Category().contains(olive::Node::kCategoryProject));
	EXPECT_TRUE(folder.IsItem());

	// The child input is a non-keyframable array that accepts any node
	EXPECT_TRUE(folder.HasInputWithID(olive::Folder::kChildInput));
	EXPECT_TRUE(folder.InputIsArray(olive::Folder::kChildInput));
	EXPECT_EQ(int(folder.GetInputDataType(olive::Folder::kChildInput)),
			  int(olive::NodeValue::kNone));
	EXPECT_FALSE(folder.IsInputKeyframable(olive::Folder::kChildInput));

	// Folders provide their own icon; every other data type falls through to
	// the Node base implementation
	EXPECT_TRUE(folder.data(olive::Node::ICON).isValid());
	EXPECT_FALSE(folder.data(olive::Node::TOOLTIP).isValid());
}

TEST(Folder, RetranslateSetsChildInputName)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *folder = AddNode<olive::Folder>(&project);
	folder->Retranslate();

	EXPECT_EQ(folder->GetInputName(olive::Folder::kChildInput),
			  QStringLiteral("Children"));
}

TEST(Folder, AddChildAppendsAndEmitsSignals)
{
	// Declared before the project so teardown signals never outlive them
	QVector<olive::Node *> inserted_items;
	QVector<int> inserted_indices;
	int insert_ends = 0;

	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::Folder *folder = project.root();
	auto *child = AddNode<olive::SolidGenerator>(&project);

	QObject::connect(folder, &olive::Folder::BeginInsertItem,
					 [&inserted_items, &inserted_indices](olive::Node *n,
														 int index) {
						 inserted_items.append(n);
						 inserted_indices.append(index);
					 });
	QObject::connect(folder, &olive::Folder::EndInsertItem,
					 [&insert_ends]() { ++insert_ends; });

	olive::FolderAddChild(folder, child).redo_now();

	ASSERT_EQ(folder->item_child_count(), 1);
	EXPECT_EQ(folder->item_child(0), child);
	EXPECT_EQ(folder->children().first(), child);
	EXPECT_EQ(folder->index_of_child(child), 0);
	EXPECT_EQ(folder->index_of_child_in_array(child), 0);
	EXPECT_EQ(child->folder(), folder);

	// The insert index is always the append position: the internal model only
	// ever appends, sorting is left to a proxy model (see folder.cpp)
	ASSERT_EQ(inserted_items.size(), 1);
	EXPECT_EQ(inserted_items.first(), child);
	EXPECT_EQ(inserted_indices.first(), 0);
	EXPECT_EQ(insert_ends, 1);
}

TEST(Folder, AddChildUndoRemovesChildAndEmitsSignals)
{
	// Declared before the project so teardown signals never outlive them
	QVector<olive::Node *> removed_items;
	QVector<int> removed_indices;
	int remove_ends = 0;

	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::Folder *folder = project.root();
	auto *child = AddNode<olive::SolidGenerator>(&project);

	QObject::connect(folder, &olive::Folder::BeginRemoveItem,
					 [&removed_items, &removed_indices](olive::Node *n,
														int index) {
						 removed_items.append(n);
						 removed_indices.append(index);
					 });
	QObject::connect(folder, &olive::Folder::EndRemoveItem,
					 [&remove_ends]() { ++remove_ends; });

	olive::FolderAddChild add(folder, child);
	add.redo_now();
	ASSERT_EQ(folder->item_child_count(), 1);

	add.undo_now();

	EXPECT_EQ(folder->item_child_count(), 0);
	EXPECT_EQ(folder->index_of_child(child), -1);
	EXPECT_EQ(folder->index_of_child_in_array(child), -1);
	EXPECT_EQ(child->folder(), nullptr);

	ASSERT_EQ(removed_items.size(), 1);
	EXPECT_EQ(removed_items.first(), child);
	EXPECT_EQ(removed_indices.first(), 0);
	EXPECT_EQ(remove_ends, 1);
}

TEST(Folder, RemoveElementCommandRemovesAndRestores)
{
	// Declared before the project so teardown signals never outlive them
	QVector<olive::Node *> removed_items;
	QVector<int> removed_indices;

	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::Folder *folder = project.root();
	auto *first = AddNode<olive::Folder>(&project);
	auto *second = AddNode<olive::Folder>(&project);
	olive::FolderAddChild(folder, first).redo_now();
	olive::FolderAddChild(folder, second).redo_now();
	ASSERT_EQ(folder->item_child_count(), 2);

	QObject::connect(folder, &olive::Folder::BeginRemoveItem,
					 [&removed_items, &removed_indices](olive::Node *n,
														int index) {
						 removed_items.append(n);
						 removed_indices.append(index);
					 });

	olive::Folder::RemoveElementCommand remove(folder, first);
	remove.redo_now();

	EXPECT_EQ(folder->item_child_count(), 1);
	EXPECT_EQ(folder->item_child(0), second);
	EXPECT_EQ(first->folder(), nullptr);
	// RemoveElementCommand removes the edge and the array element as separate
	// subcommands, each of which fires BeginRemoveItem
	ASSERT_EQ(removed_items.size(), 2);
	EXPECT_EQ(removed_items.first(), first);
	EXPECT_EQ(removed_indices.first(), 0);

	remove.undo_now();

	// The connection is restored at its original array element, but
	// Folder::InputConnectedEvent only ever appends to the internal model, so
	// the restored child lands at the end of the children list
	ASSERT_EQ(folder->item_child_count(), 2);
	EXPECT_EQ(folder->item_child(0), second);
	EXPECT_EQ(folder->item_child(1), first);
	EXPECT_EQ(folder->index_of_child(first), 1);
	EXPECT_EQ(folder->index_of_child_in_array(first), 0);
	EXPECT_EQ(first->folder(), folder);
}

TEST(Folder, RemoveElementCommandIgnoresForeignChild)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::Folder *folder = project.root();
	auto *child = AddNode<olive::Folder>(&project);
	auto *stranger = AddNode<olive::Folder>(&project);
	olive::FolderAddChild(folder, child).redo_now();
	ASSERT_EQ(folder->item_child_count(), 1);

	// A node that was never added has no array element, so the command is a
	// no-op rather than an error
	olive::Folder::RemoveElementCommand remove(folder, stranger);
	remove.redo_now();
	EXPECT_EQ(folder->item_child_count(), 1);
	EXPECT_EQ(folder->item_child(0), child);

	remove.undo_now();
	EXPECT_EQ(folder->item_child_count(), 1);
}

TEST(Folder, GetChildWithNameFindsNestedChildren)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::Folder *folder = project.root();
	auto *sub = AddNode<olive::Folder>(&project);
	sub->SetLabel(QStringLiteral("Sub"));
	auto *nested = AddNode<olive::SolidGenerator>(&project);
	nested->SetLabel(QStringLiteral("Nested"));

	olive::FolderAddChild(folder, sub).redo_now();
	olive::FolderAddChild(sub, nested).redo_now();

	// Lookup by label recurses into subfolders
	EXPECT_EQ(folder->GetChildWithName(QStringLiteral("Sub")), sub);
	EXPECT_EQ(folder->GetChildWithName(QStringLiteral("Nested")), nested);
	EXPECT_TRUE(folder->ChildExistsWithName(QStringLiteral("Nested")));

	EXPECT_EQ(folder->GetChildWithName(QStringLiteral("Missing")), nullptr);
	EXPECT_FALSE(folder->ChildExistsWithName(QStringLiteral("Missing")));
}

TEST(Folder, HasChildRecursiveFindsNestedChildren)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::Folder *folder = project.root();
	auto *sub = AddNode<olive::Folder>(&project);
	auto *nested = AddNode<olive::Folder>(&project);
	auto *outsider = AddNode<olive::Folder>(&project);

	olive::FolderAddChild(folder, sub).redo_now();
	olive::FolderAddChild(sub, nested).redo_now();

	EXPECT_TRUE(folder->HasChildRecursive(sub));
	EXPECT_TRUE(folder->HasChildRecursive(nested));
	EXPECT_FALSE(folder->HasChildRecursive(outsider));
	EXPECT_FALSE(folder->HasChildRecursive(folder));
}

TEST(Folder, ListChildrenOfTypeRecursesIntoSubfolders)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	olive::Folder *folder = project.root();
	auto *sub = AddNode<olive::Folder>(&project);
	auto *nested = AddNode<olive::Folder>(&project);
	auto *item = AddNode<olive::SolidGenerator>(&project);

	olive::FolderAddChild(folder, sub).redo_now();
	olive::FolderAddChild(sub, nested).redo_now();
	olive::FolderAddChild(folder, item).redo_now();

	// Folders are collected recursively while other items are skipped
	const QVector<olive::Folder *> folders =
		folder->ListChildrenOfType<olive::Folder>();
	ASSERT_EQ(folders.size(), 2);
	EXPECT_TRUE(folders.contains(sub));
	EXPECT_TRUE(folders.contains(nested));
	EXPECT_FALSE(folders.contains(static_cast<olive::Node *>(item)));

	const QVector<olive::SolidGenerator *> solids =
		folder->ListChildrenOfType<olive::SolidGenerator>();
	ASSERT_EQ(solids.size(), 1);
	EXPECT_EQ(solids.first(), item);
}

TEST(Folder, ChildStructureSurvivesSerializationRoundTrip)
{
	EnsureAppSingletons();
	olive::ColorManager::SetUpDefaultConfig();
	olive::NodeFactory::Initialize();
	// Guard against serializer instances left over by another test
	olive::ProjectSerializer::Destroy();
	olive::ProjectSerializer::Initialize();

	olive::Project project;
	project.Initialize();

	auto *sub = AddNode<olive::Folder>(&project);
	sub->SetLabel(QStringLiteral("Sub"));
	olive::FolderAddChild(project.root(), sub).redo_now();

	auto *nested = AddNode<olive::Folder>(&project);
	nested->SetLabel(QStringLiteral("Nested"));
	olive::FolderAddChild(sub, nested).redo_now();

	olive::ProjectSerializer::SaveData save_data(
		olive::ProjectSerializer::kProject, &project, QString());

	QByteArray xml;
	QBuffer buffer(&xml);
	buffer.open(QIODevice::WriteOnly);
	QXmlStreamWriter writer(&buffer);
	ASSERT_EQ(olive::ProjectSerializer::Save(&writer, save_data).code(),
			  olive::ProjectSerializer::kSuccess);
	buffer.close();

	olive::Project loaded_project;
	QBuffer read_buffer(&xml);
	read_buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&read_buffer);
	olive::ProjectSerializer::Result result = olive::ProjectSerializer::Load(
		&loaded_project, &reader, olive::ProjectSerializer::kProject);
	ASSERT_EQ(result.code(), olive::ProjectSerializer::kSuccess);

	// The child connections of kChildInput are re-established on load,
	// rebuilding the folder hierarchy
	olive::Folder *loaded_root = loaded_project.root();
	ASSERT_NE(loaded_root, nullptr);
	ASSERT_EQ(loaded_root->item_child_count(), 1);

	olive::Node *loaded_sub = loaded_root->item_child(0);
	EXPECT_EQ(loaded_sub->GetLabel(), QStringLiteral("Sub"));
	EXPECT_EQ(loaded_sub->folder(), loaded_root);

	auto *loaded_sub_folder = dynamic_cast<olive::Folder *>(loaded_sub);
	ASSERT_NE(loaded_sub_folder, nullptr);
	ASSERT_EQ(loaded_sub_folder->item_child_count(), 1);
	EXPECT_EQ(loaded_sub_folder->item_child(0)->GetLabel(),
			  QStringLiteral("Nested"));
	EXPECT_EQ(loaded_sub_folder->item_child(0)->folder(), loaded_sub_folder);

	EXPECT_EQ(loaded_root->GetChildWithName(QStringLiteral("Nested")),
			  loaded_sub_folder->item_child(0));
	EXPECT_TRUE(
		loaded_root->HasChildRecursive(loaded_sub_folder->item_child(0)));

	olive::ProjectSerializer::Destroy();
}

TEST(PolygonGenerator, GenerateFrameRasterizesDefaultPentagon)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::PolygonGenerator>(&project);

	const olive::VideoParams vparams(320, 240, olive::core::PixelFormat::U8,
									 olive::VideoParams::kRGBAChannelCount);
	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(vparams);
	frame->allocate();

	node->GenerateFrame(frame, olive::GenerateJob(GenerateRow(node)));

	auto pixel = [&frame](int x, int y) -> const uchar * {
		return reinterpret_cast<const uchar *>(frame->data()) +
			   y * frame->linesize_bytes() +
			   x * olive::VideoParams::kRGBAChannelCount;
	};

	// The pentagon is filled white: the frame center is well inside it
	const uchar *center = pixel(160, 120);
	EXPECT_EQ(int(center[0]), 255);
	EXPECT_EQ(int(center[1]), 255);
	EXPECT_EQ(int(center[2]), 255);
	EXPECT_EQ(int(center[3]), 255);

	const uchar *lower = pixel(160, 200);
	EXPECT_EQ(int(lower[3]), 255);

	// The corners are outside the pentagon and stay transparent
	for (int y : { 0, 239 }) {
		for (int x : { 0, 319 }) {
			const uchar *corner = pixel(x, y);
			EXPECT_EQ(int(corner[3]), 0) << "corner " << x << ", " << y;
		}
	}
}

TEST(PolygonGenerator, UpdateGizmoPositionsCreatesHandlesForEachPoint)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::PolygonGenerator>(&project);

	// Only the path gizmo exists before the first update
	ASSERT_EQ(node->GetGizmos().size(), 1);

	const olive::VideoParams vparams(320, 240, olive::core::PixelFormat::F32,
									 olive::VideoParams::kRGBAChannelCount);
	const olive::NodeGlobals globals(vparams, olive::core::AudioParams(),
									 olive::rational(0),
									 olive::LoopMode::kLoopModeOff);

	node->UpdateGizmoPositions(GenerateRow(node), globals);

	// Path gizmo + one position handle, two bezier handles and two bezier
	// lines per point of the default pentagon
	ASSERT_EQ(node->GetGizmos().size(), 1 + 5 + 10 + 10);

	// Without a base texture the gizmos are anchored at half the sequence
	// resolution on top of each point
	const double expected[5][2] = {
		{ 0, -135 }, { 135, -45 }, { 90, 120 }, { -90, 120 }, { -135, -45 }
	};
	for (int i = 0; i < 5; i++) {
		auto *position =
			static_cast<olive::PointGizmo *>(node->GetGizmos().at(1 + i));
		EXPECT_EQ(position->GetPoint(),
				  QPointF(expected[i][0] + 160, expected[i][1] + 120))
			<< "Wrong position handle for point " << i;
	}

	// Bezier handles default to the point position (zero control point offsets)
	auto *bezier = static_cast<olive::PointGizmo *>(node->GetGizmos().at(6));
	EXPECT_EQ(int(bezier->GetShape()), int(olive::PointGizmo::kCircle));
	EXPECT_EQ(bezier->GetPoint(), QPointF(160, -15));

	auto *line = static_cast<olive::LineGizmo *>(node->GetGizmos().at(16));
	EXPECT_EQ(line->GetLine(), QLineF(QPointF(160, -15), QPointF(160, -15)));

	auto *path = dynamic_cast<olive::PathGizmo *>(node->GetGizmos().first());
	ASSERT_NE(path, nullptr);
	// moveTo plus one cubic segment (3 elements) per edge of the pentagon
	EXPECT_EQ(path->GetPath().elementCount(), 16);

	// Shrinking the point array shrinks the gizmo vectors with it
	node->InputArrayResize(olive::PolygonGenerator::kPointsInput, 3);
	node->UpdateGizmoPositions(GenerateRow(node), globals);
	EXPECT_EQ(node->GetGizmos().size(), 1 + 3 + 6 + 6);

	// With a base texture connected the gizmos anchor at half the texture's
	// virtual resolution instead of the sequence's
	const olive::TexturePtr base = std::make_shared<olive::Texture>(
		olive::VideoParams(64, 48, olive::core::PixelFormat::U8,
						   olive::VideoParams::kRGBAChannelCount));
	olive::NodeValueRow row = GenerateRow(node);
	row.insert(olive::GeneratorWithMerge::kBaseInput,
			   olive::NodeValue(olive::NodeValue::kTexture, base));
	node->UpdateGizmoPositions(row, globals);

	auto *position =
		static_cast<olive::PointGizmo *>(node->GetGizmos().at(1));
	EXPECT_EQ(position->GetPoint(), QPointF(32, -111));
}

TEST(PolygonGenerator, DraggingPositionGizmoUpdatesPointTracks)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::PolygonGenerator>(&project);

	const olive::NodeValueRow row = GenerateRow(node);
	node->UpdateGizmoPositions(row, olive::NodeGlobals());
	ASSERT_EQ(node->GetGizmos().size(), 26);

	// The first position handle drives the X/Y tracks of the first point
	auto *gizmo = static_cast<olive::PointGizmo *>(node->GetGizmos().at(1));
	gizmo->DragStart(row, 0, 0, olive::rational(0));
	gizmo->DragMove(10, -20, Qt::NoModifier);

	EXPECT_DOUBLE_EQ(node->GetSplitStandardValueOnTrack(
						 olive::PolygonGenerator::kPointsInput, 0, 0)
						 .toDouble(),
					 10.0);
	EXPECT_DOUBLE_EQ(node->GetSplitStandardValueOnTrack(
						 olive::PolygonGenerator::kPointsInput, 1, 0)
						 .toDouble(),
					 -155.0);

	// The other points are untouched
	EXPECT_DOUBLE_EQ(node->GetSplitStandardValueOnTrack(
						 olive::PolygonGenerator::kPointsInput, 0, 1)
						 .toDouble(),
					 135.0);

	olive::MultiUndoCommand command;
	gizmo->DragEnd(&command);
}

TEST(TextGeneratorV2, MetadataIsCorrect)
{
	olive::TextGeneratorV2 node;
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.text2"));
	EXPECT_EQ(node.Name(), QStringLiteral("Text (Legacy)"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryGenerator));

	// Hidden from the create menu: superseded by TextGeneratorV3
	EXPECT_TRUE(node.GetFlags() & olive::Node::kDontShowInCreateMenu);
}

TEST(TextGeneratorV2, InputDefaults)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV2>(&project);

	EXPECT_EQ(node->GetStandardValue(olive::TextGeneratorV2::kTextInput)
				  .toString(),
			  QStringLiteral("Sample Text"));

	EXPECT_EQ(int(node->GetInputDataType(olive::TextGeneratorV2::kHtmlInput)),
			  int(olive::NodeValue::kBoolean));
	EXPECT_FALSE(node->GetStandardValue(olive::TextGeneratorV2::kHtmlInput)
					 .toBool());

	EXPECT_EQ(int(node->GetInputDataType(olive::TextGeneratorV2::kVAlignInput)),
			  int(olive::NodeValue::kCombo));
	EXPECT_EQ(node->GetStandardValue(olive::TextGeneratorV2::kVAlignInput)
				  .toInt(),
			  0);

	EXPECT_EQ(int(node->GetInputDataType(olive::TextGeneratorV2::kFontInput)),
			  int(olive::NodeValue::kFont));

	EXPECT_EQ(
		int(node->GetInputDataType(olive::TextGeneratorV2::kFontSizeInput)),
		int(olive::NodeValue::kFloat));
	EXPECT_DOUBLE_EQ(node->GetStandardValue(olive::TextGeneratorV2::kFontSizeInput)
						 .toDouble(),
					 72.0);

	// From ShapeNodeBase: white text on a 400x300 box
	const olive::core::Color color =
		node->GetStandardValue(olive::ShapeNodeBase::kColorInput)
			.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(color.red(), 1.0f);
	EXPECT_FLOAT_EQ(color.green(), 1.0f);
	EXPECT_FLOAT_EQ(color.blue(), 1.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);
	EXPECT_EQ(node->GetStandardValue(olive::ShapeNodeBase::kSizeInput)
				  .value<QVector2D>(),
			  QVector2D(400.0f, 300.0f));
}

TEST(TextGeneratorV2, RetranslateSetsNamesAndComboStrings)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV2>(&project);
	node->Retranslate();

	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV2::kTextInput),
			  QStringLiteral("Text"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV2::kHtmlInput),
			  QStringLiteral("Enable HTML"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV2::kFontInput),
			  QStringLiteral("Font"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV2::kFontSizeInput),
			  QStringLiteral("Font Size"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV2::kVAlignInput),
			  QStringLiteral("Vertical Align"));

	// Inherited names from ShapeNodeBase and GeneratorWithMerge
	EXPECT_EQ(node->GetInputName(olive::ShapeNodeBase::kPositionInput),
			  QStringLiteral("Position"));
	EXPECT_EQ(node->GetInputName(olive::ShapeNodeBase::kSizeInput),
			  QStringLiteral("Size"));
	EXPECT_EQ(node->GetInputName(olive::ShapeNodeBase::kColorInput),
			  QStringLiteral("Color"));
	EXPECT_EQ(node->GetInputName(olive::GeneratorWithMerge::kBaseInput),
			  QStringLiteral("Base"));

	const QStringList aligns =
		node->GetComboBoxStrings(olive::TextGeneratorV2::kVAlignInput);
	ASSERT_EQ(aligns.size(), 3);
	EXPECT_EQ(aligns.at(0), QStringLiteral("Top"));
	EXPECT_EQ(aligns.at(1), QStringLiteral("Center"));
	EXPECT_EQ(aligns.at(2), QStringLiteral("Bottom"));
}

TEST(TextGeneratorV2, ValuePushesFloatTextureWithGenerateJob)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV2>(&project);

	const olive::VideoParams vparams(320, 240, olive::core::PixelFormat::U8,
									 olive::VideoParams::kRGBAChannelCount);
	olive::NodeValueTable table = GenerateTable(node, vparams);

	// Text always renders to a 32-bit float buffer regardless of sequence depth
	olive::TexturePtr texture = GetOutputTexture(table);
	ASSERT_TRUE(texture);
	ASSERT_TRUE(texture->IsJob());
	EXPECT_EQ(texture->params().width(), vparams.width());
	EXPECT_EQ(texture->params().height(), vparams.height());
	EXPECT_EQ(int(texture->params().format()),
			  int(olive::core::PixelFormat::F32));

	auto *job = dynamic_cast<olive::GenerateJob *>(texture->job());
	ASSERT_TRUE(job);
	EXPECT_EQ(job->Get(olive::TextGeneratorV2::kTextInput).toString(),
			  QStringLiteral("Sample Text"));
	EXPECT_DOUBLE_EQ(job->Get(olive::TextGeneratorV2::kFontSizeInput).toDouble(),
					 72.0);
	EXPECT_EQ(job->Get(olive::TextGeneratorV2::kVAlignInput).toInt(), 0);
	EXPECT_EQ(job->Get(olive::ShapeNodeBase::kSizeInput).toVec2(),
			  QVector2D(400.0f, 300.0f));
}

TEST(TextGeneratorV2, EmptyTextPushesNothing)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV2>(&project);
	node->SetStandardValue(olive::TextGeneratorV2::kTextInput, QString());

	olive::NodeValueTable table = GenerateTable(
		node, olive::VideoParams(320, 240, olive::core::PixelFormat::U8,
								 olive::VideoParams::kRGBAChannelCount));

	EXPECT_TRUE(GetOutputTexture(table) == nullptr);
}

TEST(TextGeneratorV2, ValueIgnoresBaseInput)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV2>(&project);
	auto *constant = AddNode<ConstantTextureNode>(&project);

	const olive::TexturePtr base = std::make_shared<olive::Texture>(
		olive::VideoParams(64, 48, olive::core::PixelFormat::U8,
						   olive::VideoParams::kRGBAChannelCount));
	constant->SetTexture(base);
	olive::Node::ConnectEdge(constant,
							 olive::NodeInput(
								 node, olive::GeneratorWithMerge::kBaseInput));

	olive::NodeValueTable table = GenerateTable(
		node, olive::VideoParams(320, 240, olive::core::PixelFormat::F32,
								 olive::VideoParams::kRGBAChannelCount));

	// Unlike TextGeneratorV3, which composites its text over the base input,
	// the legacy V2 node never looks at it: the output is its own generate
	// job at sequence params, not a merge with the base
	olive::TexturePtr texture = GetOutputTexture(table);
	ASSERT_TRUE(texture);
	ASSERT_TRUE(texture->IsJob());
	EXPECT_EQ(texture->params().width(), 320);
	EXPECT_EQ(texture->params().height(), 240);
	EXPECT_TRUE(dynamic_cast<olive::GenerateJob *>(texture->job()));
}

TEST(TextGeneratorV2, GenerateFrameWithEmptyTextLeavesFrameTransparent)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV2>(&project);
	node->SetStandardValue(olive::TextGeneratorV2::kTextInput, QString());

	// Walk the vertical alignment switch and the HTML branch with empty text:
	// no glyphs are drawn, so the transplant loop writes pure zeros
	olive::NodeValueRow row = GenerateRow(node);
	for (int valign = 0; valign <= 2; valign++) {
		for (int html = 0; html <= 1; html++) {
			row[olive::TextGeneratorV2::kVAlignInput] =
				olive::NodeValue(olive::NodeValue::kCombo, valign);
			row[olive::TextGeneratorV2::kHtmlInput] =
				olive::NodeValue(olive::NodeValue::kBoolean, bool(html));

			olive::FramePtr frame = olive::Frame::Create();
			frame->set_video_params(
				olive::VideoParams(64, 48, olive::core::PixelFormat::F32,
								   olive::VideoParams::kRGBAChannelCount));
			frame->allocate();

			node->GenerateFrame(frame, olive::GenerateJob(row));

			const float *data = reinterpret_cast<const float *>(frame->data());
			const int pixel_count =
				frame->linesize_pixels() * frame->height() *
				olive::VideoParams::kRGBAChannelCount;
			float max_abs = 0.0f;
			for (int i = 0; i < pixel_count; i++) {
				max_abs = qMax(max_abs, qAbs(data[i]));
			}
			EXPECT_FLOAT_EQ(max_abs, 0.0f)
				<< "valign " << valign << ", html " << html;
		}
	}
}

TEST(TextGeneratorV2, GenerateFrameRasterizesTextInColor)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV2>(&project);
	node->SetStandardValue(
		olive::ShapeNodeBase::kColorInput,
		QVariant::fromValue(olive::core::Color(1.0f, 0.0f, 0.0f, 1.0f)));

	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(
		olive::VideoParams(320, 240, olive::core::PixelFormat::F32,
						   olive::VideoParams::kRGBAChannelCount));
	frame->allocate();

	node->GenerateFrame(frame, olive::GenerateJob(GenerateRow(node)));

	// The alpha mask of the rendered glyphs is tinted by the color input:
	// red premultiplied text has red == alpha and zero green/blue everywhere
	const float *data = reinterpret_cast<const float *>(frame->data());
	bool any_alpha = false;
	bool any_green_or_blue = false;
	bool red_matches_alpha = true;
	for (int y = 0; y < frame->height(); y++) {
		for (int x = 0; x < frame->width(); x++) {
			const float *px = data +
							  (y * frame->linesize_pixels() + x) *
								  olive::VideoParams::kRGBAChannelCount;
			any_alpha |= px[3] > 0.0f;
			any_green_or_blue |= (px[1] != 0.0f || px[2] != 0.0f);
			red_matches_alpha &= (px[0] == px[3]);
		}
	}
	EXPECT_TRUE(any_alpha);
	EXPECT_FALSE(any_green_or_blue);
	EXPECT_TRUE(red_matches_alpha);
}

TEST(TextGeneratorV1, MetadataIsCorrect)
{
	olive::TextGeneratorV1 node;
	EXPECT_EQ(node.id(),
			  QStringLiteral("org.olivevideoeditor.Olive.textgenerator"));
	EXPECT_EQ(node.Name(), QStringLiteral("Text (Legacy)"));
	EXPECT_FALSE(node.Description().isEmpty());
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryGenerator));

	// Hidden from the create menu: superseded by TextGeneratorV3
	EXPECT_TRUE(node.GetFlags() & olive::Node::kDontShowInCreateMenu);
}

TEST(TextGeneratorV1, InputDefaults)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV1>(&project);

	EXPECT_EQ(node->GetStandardValue(olive::TextGeneratorV1::kTextInput)
				  .toString(),
			  QStringLiteral("Sample Text"));

	EXPECT_EQ(int(node->GetInputDataType(olive::TextGeneratorV1::kHtmlInput)),
			  int(olive::NodeValue::kBoolean));
	EXPECT_FALSE(node->GetStandardValue(olive::TextGeneratorV1::kHtmlInput)
					 .toBool());

	EXPECT_EQ(int(node->GetInputDataType(olive::TextGeneratorV1::kColorInput)),
			  int(olive::NodeValue::kColor));
	const olive::core::Color color =
		node->GetStandardValue(olive::TextGeneratorV1::kColorInput)
			.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(color.red(), 1.0f);
	EXPECT_FLOAT_EQ(color.green(), 1.0f);
	EXPECT_FLOAT_EQ(color.blue(), 1.0f);
	EXPECT_FLOAT_EQ(color.alpha(), 1.0f);

	// Unlike V2, V1 defaults to centered vertical alignment
	EXPECT_EQ(int(node->GetInputDataType(olive::TextGeneratorV1::kVAlignInput)),
			  int(olive::NodeValue::kCombo));
	EXPECT_EQ(node->GetStandardValue(olive::TextGeneratorV1::kVAlignInput)
				  .toInt(),
			  1);

	EXPECT_EQ(
		int(node->GetInputDataType(olive::TextGeneratorV1::kFontSizeInput)),
		int(olive::NodeValue::kFloat));
	EXPECT_DOUBLE_EQ(node->GetStandardValue(olive::TextGeneratorV1::kFontSizeInput)
						 .toDouble(),
					 72.0);
}

TEST(TextGeneratorV1, RetranslateSetsNamesAndComboStrings)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV1>(&project);
	node->Retranslate();

	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV1::kTextInput),
			  QStringLiteral("Text"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV1::kHtmlInput),
			  QStringLiteral("Enable HTML"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV1::kFontInput),
			  QStringLiteral("Font"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV1::kFontSizeInput),
			  QStringLiteral("Font Size"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV1::kColorInput),
			  QStringLiteral("Color"));
	EXPECT_EQ(node->GetInputName(olive::TextGeneratorV1::kVAlignInput),
			  QStringLiteral("Vertical Align"));

	const QStringList aligns =
		node->GetComboBoxStrings(olive::TextGeneratorV1::kVAlignInput);
	ASSERT_EQ(aligns.size(), 3);
	EXPECT_EQ(aligns.at(0), QStringLiteral("Top"));
	EXPECT_EQ(aligns.at(1), QStringLiteral("Center"));
	EXPECT_EQ(aligns.at(2), QStringLiteral("Bottom"));
}

TEST(TextGeneratorV1, ValuePushesTextureAtSequenceParams)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV1>(&project);

	const olive::VideoParams vparams(320, 240, olive::core::PixelFormat::U8,
									 olive::VideoParams::kRGBAChannelCount);
	olive::NodeValueTable table = GenerateTable(node, vparams);

	// Unlike V2, V1 keeps the sequence pixel format for its output
	olive::TexturePtr texture = GetOutputTexture(table);
	ASSERT_TRUE(texture);
	ASSERT_TRUE(texture->IsJob());
	EXPECT_EQ(texture->params().width(), vparams.width());
	EXPECT_EQ(texture->params().height(), vparams.height());
	EXPECT_EQ(int(texture->params().format()),
			  int(olive::core::PixelFormat::U8));

	auto *job = dynamic_cast<olive::GenerateJob *>(texture->job());
	ASSERT_TRUE(job);
	EXPECT_EQ(job->Get(olive::TextGeneratorV1::kTextInput).toString(),
			  QStringLiteral("Sample Text"));
	EXPECT_DOUBLE_EQ(job->Get(olive::TextGeneratorV1::kFontSizeInput).toDouble(),
					 72.0);
}

TEST(TextGeneratorV1, EmptyTextPushesNothing)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV1>(&project);
	node->SetStandardValue(olive::TextGeneratorV1::kTextInput, QString());

	olive::NodeValueTable table = GenerateTable(
		node, olive::VideoParams(320, 240, olive::core::PixelFormat::U8,
								 olive::VideoParams::kRGBAChannelCount));

	EXPECT_TRUE(GetOutputTexture(table) == nullptr);
}

TEST(TextGeneratorV1, GenerateFrameWithEmptyTextLeavesFrameBlack)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV1>(&project);
	node->SetStandardValue(olive::TextGeneratorV1::kTextInput, QString());

	// Walk the vertical alignment switch and the HTML branch with empty text:
	// no glyphs are drawn, so every pixel is set to transparent black
	olive::NodeValueRow row = GenerateRow(node);
	for (int valign = 0; valign <= 2; valign++) {
		for (int html = 0; html <= 1; html++) {
			row[olive::TextGeneratorV1::kVAlignInput] =
				olive::NodeValue(olive::NodeValue::kCombo, valign);
			row[olive::TextGeneratorV1::kHtmlInput] =
				olive::NodeValue(olive::NodeValue::kBoolean, bool(html));

			olive::FramePtr frame = olive::Frame::Create();
			frame->set_video_params(
				olive::VideoParams(64, 48, olive::core::PixelFormat::F32,
								   olive::VideoParams::kRGBAChannelCount));
			frame->allocate();

			node->GenerateFrame(frame, olive::GenerateJob(row));

			const float *data = reinterpret_cast<const float *>(frame->data());
			const int pixel_count =
				frame->linesize_pixels() * frame->height() *
				olive::VideoParams::kRGBAChannelCount;
			float max_abs = 0.0f;
			for (int i = 0; i < pixel_count; i++) {
				max_abs = qMax(max_abs, qAbs(data[i]));
			}
			EXPECT_FLOAT_EQ(max_abs, 0.0f)
				<< "valign " << valign << ", html " << html;
		}
	}
}

TEST(TextGeneratorV1, GenerateFrameRasterizesTextPixels)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *node = AddNode<olive::TextGeneratorV1>(&project);

	olive::FramePtr frame = olive::Frame::Create();
	frame->set_video_params(
		olive::VideoParams(320, 240, olive::core::PixelFormat::F32,
						   olive::VideoParams::kRGBAChannelCount));
	frame->allocate();

	node->GenerateFrame(frame, olive::GenerateJob(GenerateRow(node)));

	// The default white text is written premultiplied: any covered pixel has
	// all channels equal to its alpha
	const float *data = reinterpret_cast<const float *>(frame->data());
	bool any_alpha = false;
	bool channels_match_alpha = true;
	for (int y = 0; y < frame->height(); y++) {
		for (int x = 0; x < frame->width(); x++) {
			const float *px = data +
							  (y * frame->linesize_pixels() + x) *
								  olive::VideoParams::kRGBAChannelCount;
			any_alpha |= px[3] > 0.0f;
			channels_match_alpha &=
				(px[0] == px[3] && px[1] == px[3] && px[2] == px[3]);
		}
	}
	EXPECT_TRUE(any_alpha);
	EXPECT_TRUE(channels_match_alpha);
}
