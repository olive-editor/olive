#include <gtest/gtest.h>

#include <memory>

#include <QAction>
#include <QDir>
#include <QFile>
#include <QList>
#include <QMenu>
#include <QPointF>
#include <QSet>
#include <QTemporaryDir>
#include <QUuid>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "node/block/clip/clip.h"
#include "node/block/gap/gap.h"
#include "node/color/colormanager/colormanager.h"
#include "node/factory.h"
#include "node/generator/solid/solid.h"
#include "node/generator/text/textv3.h"
#include "node/group/group.h"
#include "node/math/math/math.h"
#include "node/output/track/track.h"
#include "node/output/viewer/viewer.h"
#include "node/project.h"
#include "node/project/folder/folder.h"
#include "node/project/footage/footage.h"
#include "node/project/sequence/sequence.h"
#include "node/serializeddata.h"
#include "core.h"
#include "render/diskmanager.h"
#include "widget/menu/menu.h"

namespace
{

void CollectLeafActions(QMenu *menu, QList<QAction *> *leaves)
{
	for (QAction *action : menu->actions()) {
		if (action->menu()) {
			CollectLeafActions(action->menu(), leaves);
		} else if (!action->isSeparator()) {
			leaves->append(action);
		}
	}
}


// Project save/load and cache paths go through the DiskManager singleton,
// which itself touches Core
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

TEST(Project, FilenameNamePrettyAndSignals)
{
	olive::ColorManager::SetUpDefaultConfig();
	EnsureAppSingletons();
	olive::Project project;

	int name_changes = 0;
	QObject::connect(&project, &olive::Project::NameChanged,
					 [&name_changes]() { ++name_changes; });

	const QString filename = QStringLiteral("/tmp/some/dir/my_edit.ove");
	// Project::set_filename converts to native separators on Windows
	const QString stored_filename = QDir::toNativeSeparators(filename);
	project.set_filename(filename);
	EXPECT_EQ(project.filename(), stored_filename);
	EXPECT_EQ(project.name(), QStringLiteral("my_edit"));
	EXPECT_EQ(project.pretty_filename(), stored_filename);
	EXPECT_FALSE(project.is_new());
	EXPECT_EQ(name_changes, 1);

	// Each set_filename() call emits, even for the same value
	project.set_filename(filename);
	EXPECT_EQ(name_changes, 2);

	project.SetSavedURL(QStringLiteral("/tmp/some/dir"));
	EXPECT_EQ(project.GetSavedURL(), QStringLiteral("/tmp/some/dir"));

	// A filename alone does not mark the project modified
	EXPECT_FALSE(project.is_modified());
}

TEST(Project, ModifiedAndAutoRecoverySignals)
{
	olive::ColorManager::SetUpDefaultConfig();
	EnsureAppSingletons();
	olive::Project project;

	QVector<bool> states;
	QObject::connect(&project, &olive::Project::ModifiedChanged,
					 [&states](bool e) { states.append(e); });

	project.set_modified(true);
	EXPECT_TRUE(project.is_modified());
	EXPECT_FALSE(project.has_autorecovery_been_saved());

	project.set_modified(false);
	EXPECT_FALSE(project.is_modified());
	EXPECT_TRUE(project.has_autorecovery_been_saved());

	ASSERT_EQ(states.size(), 2);
	EXPECT_TRUE(states.at(0));
	EXPECT_FALSE(states.at(1));

	// The auto-recovery flag can also be controlled directly
	project.set_autorecovery_saved(false);
	EXPECT_FALSE(project.has_autorecovery_been_saved());
	project.set_autorecovery_saved(true);
	EXPECT_TRUE(project.has_autorecovery_been_saved());
}

TEST(Project, SettingsEmitSignalsAndColorSideEffects)
{
	olive::ColorManager::SetUpDefaultConfig();
	EnsureAppSingletons();
	olive::Project project;

	QVector<QString> changed_keys;
	QObject::connect(&project, &olive::Project::SettingChanged,
					 [&changed_keys](const QString &key, const QString &) {
						 changed_keys.append(key);
					 });

	QString reference_space;
	QObject::connect(project.color_manager(),
					 &olive::ColorManager::ReferenceSpaceChanged,
					 [&reference_space](const QString &s) {
						 reference_space = s;
					 });
	QString default_input;
	QObject::connect(project.color_manager(),
					 &olive::ColorManager::DefaultInputChanged,
					 [&default_input](const QString &s) { default_input = s; });

	project.SetSetting(QStringLiteral("plain"), QStringLiteral("value"));
	EXPECT_EQ(project.GetSetting(QStringLiteral("plain")),
			  QStringLiteral("value"));

	project.SetColorReferenceSpace(QStringLiteral("ACES - ACEScg"));
	EXPECT_EQ(project.GetColorReferenceSpace(),
			  QStringLiteral("ACES - ACEScg"));
	EXPECT_EQ(reference_space, QStringLiteral("ACES - ACEScg"));

	project.SetDefaultInputColorSpace(QStringLiteral("Linear Rec.709"));
	EXPECT_EQ(project.GetDefaultInputColorSpace(),
			  QStringLiteral("Linear Rec.709"));
	EXPECT_EQ(default_input, QStringLiteral("Linear Rec.709"));

	// A nonexistent config filename is stored; the failed OCIO load inside
	// ColorManager::UpdateConfigFromFilename() is swallowed
	project.SetColorConfigFilename(QStringLiteral("/nonexistent/config.ocio"));
	EXPECT_EQ(project.GetColorConfigFilename(),
			  QStringLiteral("/nonexistent/config.ocio"));

	EXPECT_TRUE(changed_keys.contains(QStringLiteral("plain")));
	EXPECT_TRUE(changed_keys.contains(olive::Project::kColorReferenceSpace));
	EXPECT_TRUE(
		changed_keys.contains(olive::Project::kDefaultInputColorSpaceKey));
	EXPECT_TRUE(changed_keys.contains(olive::Project::kColorConfigFilename));
}

TEST(Project, CachePathModes)
{
	const bool created_disk_manager =
		(olive::DiskManager::instance() == nullptr);
	if (created_disk_manager) {
		olive::DiskManager::CreateInstance();
	}
	const QString default_path =
		olive::DiskManager::instance()->GetDefaultCachePath();

	olive::ColorManager::SetUpDefaultConfig();
	EnsureAppSingletons();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString filename =
		QDir(dir.path()).filePath(QStringLiteral("proj.ove"));
	const QString alongside =
		QDir(dir.path()).filePath(QStringLiteral("cache"));

	olive::Project project;
	project.set_filename(filename);

	// Default mode always returns the application-wide cache path
	project.SetCacheLocationSetting(olive::Project::kCacheUseDefaultLocation);
	EXPECT_EQ(project.GetCacheLocationSetting(),
			  olive::Project::kCacheUseDefaultLocation);
	EXPECT_EQ(project.cache_path(), default_path);

	// Alongside mode returns a "cache" directory next to the project file
	project.SetCacheLocationSetting(
		olive::Project::kCacheStoreAlongsideProject);
	EXPECT_EQ(project.get_cache_alongside_project_path(), alongside);
	EXPECT_EQ(project.cache_path(), alongside);

	// Without a filename there is no alongside location, so it falls back
	olive::Project unsaved;
	unsaved.SetCacheLocationSetting(
		olive::Project::kCacheStoreAlongsideProject);
	EXPECT_TRUE(unsaved.get_cache_alongside_project_path().isEmpty());
	EXPECT_EQ(unsaved.cache_path(), default_path);

	// A non-empty custom path is used verbatim; an empty one falls back to
	// the default location (this branch used to be inverted)
	olive::Project custom;
	custom.SetCacheLocationSetting(olive::Project::kCacheCustomPath);
	custom.SetCustomCachePath(QStringLiteral("/tmp/oak-custom-cache"));
	EXPECT_EQ(custom.GetCustomCachePath(),
			  QStringLiteral("/tmp/oak-custom-cache"));
	EXPECT_EQ(custom.cache_path(), QStringLiteral("/tmp/oak-custom-cache"));

	olive::Project custom_empty;
	custom_empty.SetCacheLocationSetting(olive::Project::kCacheCustomPath);
	EXPECT_EQ(custom_empty.cache_path(), default_path);

	if (created_disk_manager) {
		olive::DiskManager::DestroyInstance();
	}
}

TEST(Project, NodeManagementSignalsAndClear)
{
	olive::ColorManager::SetUpDefaultConfig();
	EnsureAppSingletons();
	olive::Project project;
	project.Initialize();

	// The root folder created by Initialize() is part of the graph
	ASSERT_EQ(project.nodes().size(), 1);
	EXPECT_EQ(project.nodes().first(), project.root());

	int added = 0;
	int removed = 0;
	olive::Node *last_added = nullptr;
	QObject::connect(&project, &olive::Project::NodeAdded,
					 [&added, &last_added](olive::Node *n) {
						 ++added;
						 last_added = n;
					 });
	QObject::connect(&project, &olive::Project::NodeRemoved,
					 [&removed](olive::Node *) { ++removed; });

	auto *math = new olive::MathNode();
	math->setParent(&project);
	EXPECT_EQ(added, 1);
	EXPECT_EQ(last_added, math);
	EXPECT_TRUE(project.nodes().contains(math));
	EXPECT_EQ(project.nodes().size(), 2);

	delete math;
	EXPECT_EQ(removed, 1);
	EXPECT_FALSE(project.nodes().contains(math));
	EXPECT_EQ(project.nodes().size(), 1);

	// Clear() destructively removes every node from the graph
	auto *a = new olive::MathNode();
	a->setParent(&project);
	auto *b = new olive::MathNode();
	b->setParent(&project);
	ASSERT_EQ(project.nodes().size(), 3);
	project.Clear();
	EXPECT_TRUE(project.nodes().isEmpty());
	// One removal from the earlier delete, plus root + 2 nodes from Clear()
	EXPECT_EQ(removed, 4);
}

TEST(Project, ContextCounting)
{
	olive::ColorManager::SetUpDefaultConfig();
	EnsureAppSingletons();
	olive::Project project;
	project.Initialize();

	auto *node = new olive::MathNode();
	node->setParent(&project);
	auto *folder = new olive::Folder();
	folder->setParent(&project);

	EXPECT_EQ(project.GetNumberOfContextsNodeIsIn(node), 0);

	EXPECT_TRUE(folder->SetNodePositionInContext(
		node, olive::Node::Position(QPointF(1.0, 2.0), true)));
	EXPECT_EQ(project.GetNumberOfContextsNodeIsIn(node), 1);
	EXPECT_EQ(project.GetNumberOfContextsNodeIsIn(node, true), 1);

	// except_itself only excludes the queried node when it acts as its own
	// context
	node->SetNodePositionInContext(node,
								   olive::Node::Position(QPointF(), true));
	EXPECT_EQ(project.GetNumberOfContextsNodeIsIn(node, false), 2);
	EXPECT_EQ(project.GetNumberOfContextsNodeIsIn(node, true), 1);
}

TEST(Project, CopySettingsCopiesEntireMap)
{
	olive::ColorManager::SetUpDefaultConfig();
	EnsureAppSingletons();
	olive::Project from;
	olive::Project to;

	from.SetSetting(QStringLiteral("alpha"), QStringLiteral("1"));
	from.SetCustomCachePath(QStringLiteral("/tmp/x"));

	EXPECT_TRUE(to.GetSetting(QStringLiteral("alpha")).isEmpty());

	olive::Project::CopySettings(&from, &to);
	EXPECT_EQ(to.GetSetting(QStringLiteral("alpha")), QStringLiteral("1"));
	EXPECT_EQ(to.GetCustomCachePath(), QStringLiteral("/tmp/x"));
}

TEST(Project, SaveLoadRoundTripPreservesUuidSettingsAndRoot)
{
	olive::ColorManager::SetUpDefaultConfig();
	EnsureAppSingletons();
	olive::NodeFactory::Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path =
		QDir(dir.path()).filePath(QStringLiteral("roundtrip.ove"));

	const QUuid fixed_uuid(
		QStringLiteral("{12345678-1234-1234-1234-1234567890ab}"));

	{
		olive::Project project;
		project.Initialize();
		project.SetUuid(fixed_uuid);
		project.SetSetting(QStringLiteral("customkey"),
						   QStringLiteral("customvalue"));

		QFile file(path);
		ASSERT_TRUE(file.open(QFile::WriteOnly));
		QXmlStreamWriter writer(&file);
		writer.writeStartDocument();
		writer.writeStartElement(QStringLiteral("project"));
		project.Save(&writer);
		writer.writeEndElement();
		writer.writeEndDocument();
		file.close();
	}

	// The project being loaded into must not be Initialize()d: Load()
	// re-resolves the root folder from the saved settings and asserts it
	// does not exist yet
	olive::Project loaded;
	QFile in(path);
	ASSERT_TRUE(in.open(QFile::ReadOnly));
	QXmlStreamReader reader(&in);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("project"));
	olive::SerializedData data = loaded.Load(&reader);
	in.close();

	EXPECT_EQ(loaded.GetUuid(), fixed_uuid);
	EXPECT_EQ(loaded.GetSetting(QStringLiteral("customkey")),
			  QStringLiteral("customvalue"));

	// The root folder was re-created as a new instance and the root setting
	// now points at it
	ASSERT_NE(loaded.root(), nullptr);
	EXPECT_TRUE(loaded.nodes().contains(loaded.root()));
	EXPECT_EQ(loaded.GetSetting(olive::Project::kRootKey),
			  QString::number(reinterpret_cast<quintptr>(loaded.root())));
	EXPECT_FALSE(data.node_ptrs.isEmpty());

	olive::NodeFactory::Destroy();
}

TEST(Project, LoadSkipsUnknownAndEmptyNodeIds)
{
	olive::ColorManager::SetUpDefaultConfig();
	EnsureAppSingletons();
	olive::NodeFactory::Initialize();

	const QString xml = QStringLiteral(
		"<project>"
		"<nodes>"
		"<node id=\"org.example.doesnotexist\"></node>"
		"<node></node>"
		"<node id=\"org.olivevideoeditor.Olive.math\"></node>"
		"</nodes>"
		"</project>");

	olive::Project project;
	QXmlStreamReader reader(xml);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name(), QStringLiteral("project"));
	project.Load(&reader);

	// Only the node with a known, non-empty id made it into the graph
	ASSERT_EQ(project.nodes().size(), 1);
	EXPECT_EQ(project.nodes().first()->id(),
			  QStringLiteral("org.olivevideoeditor.Olive.math"));

	olive::NodeFactory::Destroy();
}

TEST(NodeFactory, CreateFromFactoryIndexReturnsNonNullUniqueIds)
{
	QSet<QString> ids;
	for (int i = 0; i < int(olive::NodeFactory::kInternalNodeCount); ++i) {
		const olive::NodeFactory::InternalID factory_id =
			static_cast<olive::NodeFactory::InternalID>(i);
		olive::Node *node =
			olive::NodeFactory::CreateFromFactoryIndex(factory_id);
		ASSERT_NE(node, nullptr) << "factory index" << i << "returned null";
		EXPECT_FALSE(node->id().isEmpty());
		EXPECT_FALSE(ids.contains(node->id()))
			<< "duplicate id" << node->id().toStdString();
		ids.insert(node->id());
		delete node;
	}
	EXPECT_EQ(ids.size(), int(olive::NodeFactory::kInternalNodeCount));
}

TEST(NodeFactory, CreateFromFactoryIndexReturnsExpectedTypes)
{
	std::unique_ptr<olive::Node> footage(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kProjectFootage));
	EXPECT_NE(dynamic_cast<olive::Footage *>(footage.get()), nullptr);

	std::unique_ptr<olive::Node> sequence(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kProjectSequence));
	EXPECT_NE(dynamic_cast<olive::Sequence *>(sequence.get()), nullptr);

	std::unique_ptr<olive::Node> folder(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kProjectFolder));
	EXPECT_NE(dynamic_cast<olive::Folder *>(folder.get()), nullptr);

	std::unique_ptr<olive::Node> track(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kTrackOutput));
	EXPECT_NE(dynamic_cast<olive::Track *>(track.get()), nullptr);

	std::unique_ptr<olive::Node> viewer(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kViewerOutput));
	EXPECT_NE(dynamic_cast<olive::ViewerOutput *>(viewer.get()), nullptr);

	std::unique_ptr<olive::Node> solid(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kSolidGenerator));
	EXPECT_NE(dynamic_cast<olive::SolidGenerator *>(solid.get()), nullptr);

	std::unique_ptr<olive::Node> math(
		olive::NodeFactory::CreateFromFactoryIndex(olive::NodeFactory::kMath));
	EXPECT_NE(dynamic_cast<olive::MathNode *>(math.get()), nullptr);

	std::unique_ptr<olive::Node> text(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kTextGeneratorV3));
	EXPECT_NE(dynamic_cast<olive::TextGeneratorV3 *>(text.get()), nullptr);

	std::unique_ptr<olive::Node> clip(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kClipBlock));
	EXPECT_NE(dynamic_cast<olive::ClipBlock *>(clip.get()), nullptr);

	std::unique_ptr<olive::Node> gap(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kGapBlock));
	EXPECT_NE(dynamic_cast<olive::GapBlock *>(gap.get()), nullptr);

	std::unique_ptr<olive::Node> group(
		olive::NodeFactory::CreateFromFactoryIndex(
			olive::NodeFactory::kGroupNode));
	EXPECT_NE(dynamic_cast<olive::NodeGroup *>(group.get()), nullptr);
}

TEST(NodeFactory, CreateFromFactoryIndexCountReturnsNull)
{
	EXPECT_EQ(olive::NodeFactory::CreateFromFactoryIndex(
				  olive::NodeFactory::kInternalNodeCount),
			  nullptr);
}

TEST(NodeFactory, LibraryRoundTripAfterInitialize)
{
	olive::NodeFactory::Initialize();

	for (int i = 0; i < int(olive::NodeFactory::kInternalNodeCount); ++i) {
		std::unique_ptr<olive::Node> probe(
			olive::NodeFactory::CreateFromFactoryIndex(
				static_cast<olive::NodeFactory::InternalID>(i)));
		ASSERT_NE(probe, nullptr);

		// Every internal type must be retrievable from the library by id
		EXPECT_EQ(olive::NodeFactory::GetNameFromID(probe->id()),
				  probe->Name())
			<< probe->id().toStdString();

		std::unique_ptr<olive::Node> copy(
			olive::NodeFactory::CreateFromID(probe->id()));
		ASSERT_NE(copy, nullptr) << probe->id().toStdString();
		EXPECT_EQ(copy->id(), probe->id());
		EXPECT_NE(copy.get(), probe.get());
	}

	// Unknown and empty ids fail gracefully
	EXPECT_EQ(olive::NodeFactory::CreateFromID(
				  QStringLiteral("org.example.nonexistent")),
			  nullptr);
	EXPECT_EQ(olive::NodeFactory::CreateFromID(QString()), nullptr);
	EXPECT_TRUE(olive::NodeFactory::GetNameFromID(
					QStringLiteral("org.example.nonexistent"))
					.isEmpty());
	EXPECT_TRUE(olive::NodeFactory::GetNameFromID(QString()).isEmpty());

	olive::NodeFactory::Destroy();
}

TEST(NodeFactory, CreateMenuWithNoneItem)
{
	olive::NodeFactory::Initialize();

	std::unique_ptr<olive::Menu> menu(
		olive::NodeFactory::CreateMenu(nullptr, true));
	ASSERT_NE(menu, nullptr);
	ASSERT_FALSE(menu->actions().isEmpty());

	// The "None" item is inserted at the very top and maps to nothing
	QAction *none_item = menu->actions().first();
	EXPECT_EQ(none_item->data().toInt(), -1);
	EXPECT_EQ(olive::NodeFactory::CreateFromMenuAction(none_item), nullptr);
	EXPECT_TRUE(olive::NodeFactory::GetIDFromMenuAction(none_item).isEmpty());

	// Leaf actions carry a library index that maps back to node ids
	QList<QAction *> leaves;
	CollectLeafActions(menu.get(), &leaves);
	ASSERT_GT(leaves.size(), 1);

	int created = 0;
	for (QAction *leaf : leaves) {
		if (leaf->data().toInt() < 0) {
			continue;
		}
		const QString id = olive::NodeFactory::GetIDFromMenuAction(leaf);
		EXPECT_FALSE(id.isEmpty());
		std::unique_ptr<olive::Node> node(
			olive::NodeFactory::CreateFromMenuAction(leaf));
		ASSERT_NE(node, nullptr);
		EXPECT_EQ(node->id(), id);
		++created;
	}
	EXPECT_GT(created, 0);

	olive::NodeFactory::Destroy();
}

TEST(NodeFactory, CreateMenuRestrictedToCategory)
{
	olive::NodeFactory::Initialize();

	std::unique_ptr<olive::Menu> menu(olive::NodeFactory::CreateMenu(
		nullptr, false, olive::Node::kCategoryMath));
	ASSERT_NE(menu, nullptr);

	QList<QAction *> leaves;
	CollectLeafActions(menu.get(), &leaves);
	ASSERT_FALSE(leaves.isEmpty());

	for (QAction *leaf : leaves) {
		std::unique_ptr<olive::Node> node(
			olive::NodeFactory::CreateFromMenuAction(leaf));
		ASSERT_NE(node, nullptr);
		EXPECT_TRUE(node->Category().contains(olive::Node::kCategoryMath))
			<< node->id().toStdString();
	}

	olive::NodeFactory::Destroy();
}

TEST(NodeFactory, LegacyDistortIdsResolveToRenamedNodes)
{
	olive::NodeFactory::Initialize();

	const QList<QPair<QString, QString>> legacy_ids = {
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

	for (const auto &pair : legacy_ids) {
		std::unique_ptr<olive::Node> node(
			olive::NodeFactory::CreateFromID(pair.first));
		ASSERT_NE(node, nullptr) << pair.first.toStdString();
		EXPECT_EQ(node->id(), pair.second);

		// The current id resolves directly as well
		std::unique_ptr<olive::Node> current(
			olive::NodeFactory::CreateFromID(pair.second));
		ASSERT_NE(current, nullptr) << pair.second.toStdString();
		EXPECT_EQ(current->id(), pair.second);
	}

	olive::NodeFactory::Destroy();
}
