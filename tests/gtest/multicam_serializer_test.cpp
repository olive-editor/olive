#include <gtest/gtest.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QXmlStreamReader>

#include "node/color/colormanager/colormanager.h"
#include "node/factory.h"
#include "node/globals.h"
#include "node/input/multicam/multicamnode.h"
#include "node/output/track/track.h"
#include "node/output/track/tracklist.h"
#include "node/project.h"
#include "node/project/footage/footagedescription.h"
#include "node/project/sequence/sequence.h"
#include "node/project/serializer/serializer.h"
#include "render/diskmanager.h"

namespace
{

// Project save/load and cache paths go through the DiskManager singleton
void EnsureDiskManager(bool *created)
{
	*created = (olive::DiskManager::instance() == nullptr);
	if (*created) {
		olive::DiskManager::CreateInstance();
	}
}

void ReleaseDiskManager(bool created)
{
	if (created) {
		olive::DiskManager::DestroyInstance();
	}
}

} // namespace

TEST(MultiCamNode, DefaultState)
{
	olive::MultiCamNode node;

	EXPECT_EQ(node.Name(), QStringLiteral("Multi-Cam"));
	EXPECT_EQ(node.id(), QStringLiteral("org.olivevideoeditor.Olive.multicam"));
	EXPECT_TRUE(node.Category().contains(olive::Node::kCategoryTimeline));

	// The "arraystart" property only hints the UI; the sources array itself
	// starts empty
	EXPECT_EQ(node.GetSourceCount(), 0);
	EXPECT_EQ(node.GetCurrentSource(), 0);

	// Sequence type selector stays hidden until a sequence is connected
	EXPECT_TRUE(node.GetInputFlags(olive::MultiCamNode::kSequenceTypeInput) &
				olive::kInputFlagHidden);
}

TEST(MultiCamNode, ActiveElementsSelectCurrentSource)
{
	olive::MultiCamNode node;
	node.InputArrayResize(olive::MultiCamNode::kSourcesInput, 3);
	ASSERT_EQ(node.GetSourceCount(), 3);

	node.SetStandardValue(olive::MultiCamNode::kCurrentInput, 1);

	olive::Node::ActiveElements active = node.GetActiveElementsAtTime(
		olive::MultiCamNode::kSourcesInput, olive::TimeRange());
	EXPECT_EQ(active.mode(), olive::Node::ActiveElements::kSpecified);
	ASSERT_EQ(active.elements().size(), 1);
	EXPECT_EQ(active.elements().front(), 1);

	// Any other input defers to the base implementation
	olive::Node::ActiveElements all = node.GetActiveElementsAtTime(
		olive::MultiCamNode::kCurrentInput, olive::TimeRange());
	EXPECT_EQ(all.mode(), olive::Node::ActiveElements::kAllElements);
}

TEST(MultiCamNode, ActiveElementsOutOfRangeAreEmpty)
{
	olive::MultiCamNode node;
	node.InputArrayResize(olive::MultiCamNode::kSourcesInput, 3);

	node.SetStandardValue(olive::MultiCamNode::kCurrentInput, 5);
	EXPECT_EQ(node.GetActiveElementsAtTime(olive::MultiCamNode::kSourcesInput,
										   olive::TimeRange())
				  .mode(),
			  olive::Node::ActiveElements::kNoElements);

	node.SetStandardValue(olive::MultiCamNode::kCurrentInput, -1);
	EXPECT_EQ(node.GetActiveElementsAtTime(olive::MultiCamNode::kSourcesInput,
										   olive::TimeRange())
				  .mode(),
			  olive::Node::ActiveElements::kNoElements);
}

TEST(MultiCamNode, RowsAndColumnsGrowToFitSources)
{
	const struct {
		int sources;
		int rows;
		int cols;
	} kCases[] = { { 1, 1, 1 }, { 2, 1, 2 }, { 3, 2, 2 }, { 4, 2, 2 },
				   { 5, 2, 3 }, { 6, 2, 3 }, { 9, 3, 3 }, { 12, 3, 4 } };

	for (const auto &c : kCases) {
		int rows = 0, cols = 0;
		olive::MultiCamNode::GetRowsAndColumns(c.sources, &rows, &cols);
		EXPECT_EQ(rows, c.rows) << "sources=" << c.sources;
		EXPECT_EQ(cols, c.cols) << "sources=" << c.sources;
	}

	// The grid always fits all sources and stays as square as possible
	for (int s = 1; s <= 16; s++) {
		int rows = 0, cols = 0;
		olive::MultiCamNode::GetRowsAndColumns(s, &rows, &cols);
		EXPECT_GE(rows * cols, s);
		EXPECT_LE(rows, cols);
	}
}

TEST(MultiCamNode, RowColumnIndexRoundTrip)
{
	int row = -1, col = -1;
	olive::MultiCamNode::IndexToRowCols(5, 2, 3, &row, &col);
	EXPECT_EQ(row, 1);
	EXPECT_EQ(col, 2);
	EXPECT_EQ(olive::MultiCamNode::RowsColsToIndex(1, 2, 2, 3), 5);

	const int kRows = 3;
	const int kCols = 4;
	for (int i = 0; i < kRows * kCols; i++) {
		olive::MultiCamNode::IndexToRowCols(i, kRows, kCols, &row, &col);
		EXPECT_EQ(olive::MultiCamNode::RowsColsToIndex(row, col, kRows, kCols),
				  i);
	}
}

TEST(MultiCamNode, RetranslateSetsInputNamesAndComboStrings)
{
	olive::MultiCamNode node;
	node.Retranslate();

	EXPECT_EQ(node.GetInputName(olive::MultiCamNode::kCurrentInput),
			  QStringLiteral("Current"));
	EXPECT_EQ(node.GetInputName(olive::MultiCamNode::kSourcesInput),
			  QStringLiteral("Sources"));
	EXPECT_EQ(node.GetInputName(olive::MultiCamNode::kSequenceInput),
			  QStringLiteral("Sequence"));
	EXPECT_EQ(node.GetInputName(olive::MultiCamNode::kSequenceTypeInput),
			  QStringLiteral("Sequence Type"));

	EXPECT_EQ(
		node.GetComboBoxStrings(olive::MultiCamNode::kSequenceTypeInput),
		(QStringList{ QStringLiteral("Video"), QStringLiteral("Audio") }));

	// No sources yet, so no labels
	EXPECT_TRUE(node.GetComboBoxStrings(olive::MultiCamNode::kCurrentInput)
					.isEmpty());
}

TEST(MultiCamNode, IgnoreInputsForRenderingSkipsSequenceInput)
{
	olive::MultiCamNode node;

	EXPECT_TRUE(node.IgnoreInputsForRendering().contains(
		olive::MultiCamNode::kSequenceInput));
	EXPECT_FALSE(node.IgnoreInputsForRendering().contains(
		olive::MultiCamNode::kSourcesInput));
}

TEST(MultiCamNode, ValuePushesFirstSourceArrayElement)
{
	olive::MultiCamNode node;

	olive::NodeValueArray sources;
	sources[0] = olive::NodeValue(olive::NodeValue::kText,
								  QStringLiteral("cam A"), &node);
	sources[1] = olive::NodeValue(olive::NodeValue::kText,
								  QStringLiteral("cam B"), &node);

	olive::NodeValueRow row;
	row.insert(olive::MultiCamNode::kSourcesInput,
			   olive::NodeValue(olive::NodeValue::kNone,
								QVariant::fromValue(sources), &node, true));

	olive::NodeValueTable table;
	node.Value(row, olive::NodeGlobals(), &table);

	// Value() forwards the first array element; the traverser filters the
	// array down to the active element before Value() is ever called
	ASSERT_EQ(table.Count(), 1);
	EXPECT_EQ(table.at(0).toString(), QStringLiteral("cam A"));
}

TEST(MultiCamNode, ValueWithoutSourcesLeavesTableEmpty)
{
	olive::MultiCamNode node;

	olive::NodeValueTable table;
	node.Value(olive::NodeValueRow(), olive::NodeGlobals(), &table);

	EXPECT_TRUE(table.isEmpty());
}

TEST(MultiCamNode, ConnectedSequenceProvidesTrackSources)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *sequence = new olive::Sequence();
	sequence->setParent(&project);
	auto *node = new olive::MultiCamNode();
	node->setParent(&project);
	node->SetSequenceType(olive::Track::kVideo);

	olive::Node::ConnectEdge(
		sequence, olive::NodeInput(node, olive::MultiCamNode::kSequenceInput));

	// Connecting a sequence exposes the type selector
	EXPECT_FALSE(node->GetInputFlags(olive::MultiCamNode::kSequenceTypeInput) &
				 olive::kInputFlagHidden);

	// The sequence has no tracks yet
	EXPECT_EQ(node->GetSourceCount(), 0);

	olive::TrackList *video_list = sequence->track_list(olive::Track::kVideo);

	video_list->ArrayAppend();
	auto *track_a = new olive::Track();
	track_a->setParent(&project);
	olive::Node::ConnectEdge(track_a, video_list->track_input(0));

	video_list->ArrayAppend();
	auto *track_b = new olive::Track();
	track_b->setParent(&project);
	olive::Node::ConnectEdge(track_b, video_list->track_input(1));

	// Sources are now pulled from the sequence's track list
	ASSERT_EQ(node->GetSourceCount(), 2);
	EXPECT_EQ(node->GetConnectedRenderOutput(olive::MultiCamNode::kSourcesInput,
											 0),
			  track_a);
	EXPECT_EQ(node->GetConnectedRenderOutput(olive::MultiCamNode::kSourcesInput,
											 1),
			  track_b);
	EXPECT_TRUE(
		node->IsInputConnectedForRender(olive::MultiCamNode::kSourcesInput, 0));
	EXPECT_TRUE(
		node->IsInputConnectedForRender(olive::MultiCamNode::kSourcesInput, 1));

	// Past the end of the track list the overrides defer to the base class
	EXPECT_FALSE(
		node->IsInputConnectedForRender(olive::MultiCamNode::kSourcesInput, 2));
	EXPECT_EQ(node->GetConnectedRenderOutput(olive::MultiCamNode::kSourcesInput,
											 2),
			  nullptr);

	node->SetStandardValue(olive::MultiCamNode::kCurrentInput, 1);
	olive::Node::ActiveElements active = node->GetActiveElementsAtTime(
		olive::MultiCamNode::kSourcesInput, olive::TimeRange());
	EXPECT_EQ(active.mode(), olive::Node::ActiveElements::kSpecified);
	ASSERT_EQ(active.elements().size(), 1);
	EXPECT_EQ(active.elements().front(), 1);

	// Retranslate names each angle after its track
	node->Retranslate();
	EXPECT_EQ(node->GetComboBoxStrings(olive::MultiCamNode::kCurrentInput),
			  (QStringList{ QStringLiteral("1: Video Track 0"),
							QStringLiteral("2: Video Track 1") }));

	// Disconnecting hides the type selector and falls back to the sources array
	olive::Node::DisconnectEdge(
		sequence, olive::NodeInput(node, olive::MultiCamNode::kSequenceInput));
	EXPECT_TRUE(node->GetInputFlags(olive::MultiCamNode::kSequenceTypeInput) &
				olive::kInputFlagHidden);
	// With nothing appended to the sources array, it falls back to zero
	EXPECT_EQ(node->GetSourceCount(), 0);
}

TEST(MultiCamNode, SequenceTypeSelectsTrackList)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *sequence = new olive::Sequence();
	sequence->setParent(&project);
	auto *node = new olive::MultiCamNode();
	node->setParent(&project);

	olive::Node::ConnectEdge(
		sequence, olive::NodeInput(node, olive::MultiCamNode::kSequenceInput));

	node->SetSequenceType(olive::Track::kAudio);
	EXPECT_EQ(node->GetSourceCount(), 0);

	olive::TrackList *audio_list = sequence->track_list(olive::Track::kAudio);
	audio_list->ArrayAppend();
	auto *track = new olive::Track();
	track->setParent(&project);
	olive::Node::ConnectEdge(track, audio_list->track_input(0));

	EXPECT_EQ(node->GetSourceCount(), 1);
	EXPECT_EQ(node->GetConnectedRenderOutput(olive::MultiCamNode::kSourcesInput,
											 0),
			  track);

	// Switching the type swaps which track list feeds the sources
	node->SetSequenceType(olive::Track::kVideo);
	EXPECT_EQ(node->GetSourceCount(), 0);
}

TEST(FootageDescription, DefaultStateIsInvalid)
{
	olive::FootageDescription desc;

	EXPECT_TRUE(desc.decoder().isEmpty());
	EXPECT_EQ(desc.GetStreamCount(), 0);
	EXPECT_TRUE(desc.GetVideoStreams().isEmpty());
	EXPECT_TRUE(desc.GetAudioStreams().isEmpty());
	EXPECT_TRUE(desc.GetSubtitleStreams().isEmpty());
	EXPECT_FALSE(desc.HasSourceStartTime());
	EXPECT_FALSE(desc.IsValid());
}

TEST(FootageDescription, ValidityRequiresDecoderAndStream)
{
	olive::VideoParams video(640, 480, olive::rational(1, 24),
							 olive::core::PixelFormat::U8, 4);
	video.set_stream_index(0);

	// A stream without a decoder name is not enough
	olive::FootageDescription no_decoder;
	no_decoder.AddVideoStream(video);
	EXPECT_FALSE(no_decoder.IsValid());

	// A decoder without any stream is not enough either
	olive::FootageDescription desc(QStringLiteral("fakedecoder"));
	EXPECT_FALSE(desc.IsValid());

	desc.AddVideoStream(video);
	EXPECT_TRUE(desc.IsValid());
}

TEST(FootageDescription, StreamTypeLookup)
{
	olive::FootageDescription desc(QStringLiteral("fakedecoder"));

	olive::VideoParams video(1920, 1080, olive::rational(1, 24),
							 olive::core::PixelFormat::U8, 4);
	video.set_stream_index(0);
	desc.AddVideoStream(video);

	olive::core::AudioParams audio(48000, olive::core::kChannelLayoutStereo,
								   olive::core::SampleFormat::F32P);
	audio.set_stream_index(1);
	desc.AddAudioStream(audio);

	olive::SubtitleParams subs;
	subs.set_stream_index(2);
	subs.push_back(olive::Subtitle(olive::TimeRange(olive::rational(0),
													olive::rational(3)),
								   QStringLiteral("subtitle text")));
	desc.AddSubtitleStream(subs);

	desc.SetStreamCount(3);

	EXPECT_EQ(desc.decoder(), QStringLiteral("fakedecoder"));
	EXPECT_EQ(desc.GetStreamCount(), 3);

	EXPECT_TRUE(desc.StreamIsVideo(0));
	EXPECT_FALSE(desc.StreamIsVideo(1));
	EXPECT_TRUE(desc.StreamIsAudio(1));
	EXPECT_FALSE(desc.StreamIsAudio(2));
	EXPECT_TRUE(desc.StreamIsSubtitle(2));
	EXPECT_FALSE(desc.StreamIsSubtitle(0));

	EXPECT_TRUE(desc.HasStreamIndex(0));
	EXPECT_TRUE(desc.HasStreamIndex(1));
	EXPECT_TRUE(desc.HasStreamIndex(2));
	EXPECT_FALSE(desc.HasStreamIndex(3));

	EXPECT_EQ(desc.GetTypeOfStream(0), olive::Track::kVideo);
	EXPECT_EQ(desc.GetTypeOfStream(1), olive::Track::kAudio);
	EXPECT_EQ(desc.GetTypeOfStream(2), olive::Track::kSubtitle);
	EXPECT_EQ(desc.GetTypeOfStream(99), olive::Track::kNone);

	ASSERT_EQ(desc.GetVideoStreams().size(), 1);
	ASSERT_EQ(desc.GetAudioStreams().size(), 1);
	ASSERT_EQ(desc.GetSubtitleStreams().size(), 1);
}

TEST(FootageDescription, SaveLoadRoundTrip)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path =
		QDir(dir.path()).filePath(QStringLiteral("streamcache.xml"));

	olive::FootageDescription desc(QStringLiteral("fakedecoder"));

	olive::VideoParams video(1920, 1080, olive::rational(1, 24),
							 olive::core::PixelFormat::U8, 4);
	video.set_stream_index(0);
	video.set_duration(48);
	desc.AddVideoStream(video);

	olive::core::AudioParams audio(48000, olive::core::kChannelLayoutStereo,
								   olive::core::SampleFormat::F32P);
	audio.set_stream_index(1);
	audio.set_duration(96000);
	desc.AddAudioStream(audio);

	olive::SubtitleParams subs;
	subs.set_stream_index(2);
	subs.push_back(olive::Subtitle(olive::TimeRange(olive::rational(0),
													olive::rational(3)),
								   QStringLiteral("subtitle text")));
	desc.AddSubtitleStream(subs);

	desc.SetStreamCount(3);

	ASSERT_TRUE(desc.Save(path));

	olive::FootageDescription loaded;
	ASSERT_TRUE(loaded.Load(path));

	EXPECT_TRUE(loaded.IsValid());
	EXPECT_EQ(loaded.decoder(), QStringLiteral("fakedecoder"));
	EXPECT_EQ(loaded.GetStreamCount(), 3);

	ASSERT_EQ(loaded.GetVideoStreams().size(), 1);
	const olive::VideoParams &loaded_video = loaded.GetVideoStreams().first();
	EXPECT_EQ(loaded_video.width(), 1920);
	EXPECT_EQ(loaded_video.height(), 1080);
	EXPECT_EQ(loaded_video.stream_index(), 0);
	EXPECT_EQ(loaded_video.duration(), video.duration());
	EXPECT_EQ(loaded_video.time_base(), video.time_base());

	ASSERT_EQ(loaded.GetAudioStreams().size(), 1);
	const olive::core::AudioParams &loaded_audio =
		loaded.GetAudioStreams().first();
	EXPECT_EQ(loaded_audio.sample_rate(), 48000);
	EXPECT_EQ(loaded_audio.channel_layout(), olive::core::kChannelLayoutStereo);
	EXPECT_EQ(loaded_audio.stream_index(), 1);
	EXPECT_EQ(loaded_audio.duration(), audio.duration());

	ASSERT_EQ(loaded.GetSubtitleStreams().size(), 1);
	const olive::SubtitleParams &loaded_subs =
		loaded.GetSubtitleStreams().first();
	EXPECT_EQ(loaded_subs.stream_index(), 2);
	ASSERT_EQ(loaded_subs.size(), 1);
	EXPECT_EQ(loaded_subs.front().text(), QStringLiteral("subtitle text"));
	EXPECT_EQ(loaded_subs.front().time().out(), olive::rational(3));
}

TEST(FootageDescription, LoadMissingFileFailsAndResetsState)
{
	olive::FootageDescription desc(QStringLiteral("stale"));
	olive::VideoParams video(640, 480, olive::rational(1, 24),
							 olive::core::PixelFormat::U8, 4);
	video.set_stream_index(0);
	desc.AddVideoStream(video);
	ASSERT_TRUE(desc.IsValid());

	EXPECT_FALSE(
		desc.Load(QStringLiteral("/definitely/nonexistent/cache.xml")));

	// A failed load must not leave stale streams behind
	EXPECT_TRUE(desc.decoder().isEmpty());
	EXPECT_TRUE(desc.GetVideoStreams().isEmpty());
	EXPECT_FALSE(desc.IsValid());
}

TEST(FootageDescription, LoadRejectsMismatchedVersion)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString path =
		QDir(dir.path()).filePath(QStringLiteral("legacy-cache.xml"));

	// No version attribute: treated as the original unversioned cache format,
	// which is always discarded so the footage can be re-probed
	QFile file(path);
	ASSERT_TRUE(file.open(QFile::WriteOnly));
	file.write("<streamcache><decoder>fakedecoder</decoder></streamcache>");
	file.close();

	olive::FootageDescription desc(QStringLiteral("stale"));
	olive::VideoParams video(640, 480, olive::rational(1, 24),
							 olive::core::PixelFormat::U8, 4);
	video.set_stream_index(0);
	desc.AddVideoStream(video);

	EXPECT_FALSE(desc.Load(path));
	EXPECT_TRUE(desc.decoder().isEmpty());
	EXPECT_FALSE(desc.IsValid());
}

TEST(ProjectSerializer, FileRoundTripPreservesMultiCamNode)
{
	olive::ColorManager::SetUpDefaultConfig();
	bool created_disk_manager = false;
	EnsureDiskManager(&created_disk_manager);
	olive::NodeFactory::Initialize();
	olive::ProjectSerializer::Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString filename =
		QDir(dir.path()).filePath(QStringLiteral("multicam.ove"));

	olive::Project project;
	project.Initialize();
	auto *node = new olive::MultiCamNode();
	node->SetLabel(QStringLiteral("Angles"));
	node->SetStandardValue(olive::MultiCamNode::kCurrentInput, 2);
	node->setParent(&project);

	olive::ProjectSerializer::Result save_result =
		olive::ProjectSerializer::Save(olive::ProjectSerializer::SaveData(
										   olive::ProjectSerializer::kProject,
										   &project, filename),
									   false);
	ASSERT_EQ(save_result.code(), olive::ProjectSerializer::kSuccess);
	ASSERT_TRUE(QFile::exists(filename));

	// An uncompressed project is plain XML
	QFile raw(filename);
	ASSERT_TRUE(raw.open(QFile::ReadOnly));
	EXPECT_TRUE(raw.read(5).startsWith("<?xml"));
	raw.close();

	olive::Project loaded_project;
	olive::ProjectSerializer::Result load_result =
		olive::ProjectSerializer::Load(&loaded_project, filename,
									   olive::ProjectSerializer::kProject);
	ASSERT_EQ(load_result.code(), olive::ProjectSerializer::kSuccess);

	olive::MultiCamNode *loaded_node = nullptr;
	foreach (olive::Node *n, loaded_project.nodes()) {
		if ((loaded_node = dynamic_cast<olive::MultiCamNode *>(n))) {
			break;
		}
	}
	ASSERT_NE(loaded_node, nullptr);
	EXPECT_EQ(loaded_node->GetLabel(), QStringLiteral("Angles"));
	EXPECT_EQ(loaded_node->GetCurrentSource(), 2);

	olive::ProjectSerializer::Destroy();
	olive::NodeFactory::Destroy();
	ReleaseDiskManager(created_disk_manager);
}

TEST(ProjectSerializer, CompressedFileRoundTrip)
{
	olive::ColorManager::SetUpDefaultConfig();
	bool created_disk_manager = false;
	EnsureDiskManager(&created_disk_manager);
	olive::NodeFactory::Initialize();
	olive::ProjectSerializer::Initialize();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString filename =
		QDir(dir.path()).filePath(QStringLiteral("compressed.ove"));

	olive::Project project;
	project.Initialize();
	auto *node = new olive::MultiCamNode();
	node->SetLabel(QStringLiteral("Compressed"));
	node->setParent(&project);

	olive::ProjectSerializer::Result save_result =
		olive::ProjectSerializer::Save(olive::ProjectSerializer::SaveData(
										   olive::ProjectSerializer::kProject,
										   &project, filename),
									   true);
	ASSERT_EQ(save_result.code(), olive::ProjectSerializer::kSuccess);

	// Compressed projects are marked with the OVEC signature
	QFile raw(filename);
	ASSERT_TRUE(raw.open(QFile::ReadOnly));
	EXPECT_EQ(raw.read(4), QByteArray("OVEC", 4));
	raw.close();

	olive::Project loaded_project;
	olive::ProjectSerializer::Result load_result =
		olive::ProjectSerializer::Load(&loaded_project, filename,
									   olive::ProjectSerializer::kProject);
	ASSERT_EQ(load_result.code(), olive::ProjectSerializer::kSuccess);

	bool found = false;
	foreach (olive::Node *n, loaded_project.nodes()) {
		if (dynamic_cast<olive::MultiCamNode *>(n)) {
			found = true;
			EXPECT_EQ(n->GetLabel(), QStringLiteral("Compressed"));
			break;
		}
	}
	EXPECT_TRUE(found);

	olive::ProjectSerializer::Destroy();
	olive::NodeFactory::Destroy();
	ReleaseDiskManager(created_disk_manager);
}

TEST(ProjectSerializer, LoadNonexistentFileFails)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;

	olive::ProjectSerializer::Result result = olive::ProjectSerializer::Load(
		&project, QStringLiteral("/definitely/nonexistent/project.ove"),
		olive::ProjectSerializer::kProject);

	EXPECT_EQ(result.code(), olive::ProjectSerializer::kFileError);
	EXPECT_FALSE(result.GetDetails().isEmpty());
}

TEST(ProjectSerializer, LoadGarbageXmlReportsUnknownVersion)
{
	olive::ColorManager::SetUpDefaultConfig();

	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	const QString filename =
		QDir(dir.path()).filePath(QStringLiteral("garbage.ove"));
	QFile file(filename);
	ASSERT_TRUE(file.open(QFile::WriteOnly));
	file.write("this is not a project file");
	file.close();

	olive::Project project;
	olive::ProjectSerializer::Result result = olive::ProjectSerializer::Load(
		&project, filename, olive::ProjectSerializer::kProject);

	EXPECT_EQ(result.code(), olive::ProjectSerializer::kUnknownVersion);
}

TEST(ProjectSerializer, LoadOlderVersionReportsTooOld)
{
	olive::ProjectSerializer::Initialize();

	// 190219 predates every registered serializer
	QXmlStreamReader reader(
		QStringLiteral("<olive version=\"190219\"><foo/></olive>"));
	olive::ProjectSerializer::Result result = olive::ProjectSerializer::Load(
		nullptr, &reader, olive::ProjectSerializer::kOnlyNodes);

	EXPECT_EQ(result.code(), olive::ProjectSerializer::kProjectTooOld);

	olive::ProjectSerializer::Destroy();
}

TEST(ProjectSerializer, LoadNewerVersionReportsTooNew)
{
	olive::ProjectSerializer::Initialize();

	QXmlStreamReader reader(
		QStringLiteral("<olive version=\"999999\"><foo/></olive>"));
	olive::ProjectSerializer::Result result = olive::ProjectSerializer::Load(
		nullptr, &reader, olive::ProjectSerializer::kOnlyNodes);

	EXPECT_EQ(result.code(), olive::ProjectSerializer::kProjectTooNew);

	olive::ProjectSerializer::Destroy();
}

TEST(ProjectSerializer, LoadWithoutVersionReportsUnknown)
{
	// A project element without a version attribute
	QXmlStreamReader no_version(QStringLiteral("<olive><foo/></olive>"));
	olive::ProjectSerializer::Result result = olive::ProjectSerializer::Load(
		nullptr, &no_version, olive::ProjectSerializer::kOnlyNodes);
	EXPECT_EQ(result.code(), olive::ProjectSerializer::kUnknownVersion);

	// No recognizable root element at all
	QXmlStreamReader wrong_root(
		QStringLiteral("<unrelated><foo/></unrelated>"));
	result = olive::ProjectSerializer::Load(
		nullptr, &wrong_root, olive::ProjectSerializer::kOnlyNodes);
	EXPECT_EQ(result.code(), olive::ProjectSerializer::kUnknownVersion);
}

TEST(ProjectSerializer, CheckCompressedIDDetectsSignature)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QString compressed =
		QDir(dir.path()).filePath(QStringLiteral("compressed.ove"));
	QFile f(compressed);
	ASSERT_TRUE(f.open(QFile::WriteOnly));
	f.write("OVEC");
	f.write("payload");
	f.close();

	QFile check(compressed);
	ASSERT_TRUE(check.open(QFile::ReadOnly));
	EXPECT_TRUE(olive::ProjectSerializer::CheckCompressedID(&check));
	check.close();

	const QString plain =
		QDir(dir.path()).filePath(QStringLiteral("plain.ove"));
	QFile p(plain);
	ASSERT_TRUE(p.open(QFile::WriteOnly));
	p.write("<?xml version=\"1.0\"?>");
	p.close();

	QFile check_plain(plain);
	ASSERT_TRUE(check_plain.open(QFile::ReadOnly));
	EXPECT_FALSE(olive::ProjectSerializer::CheckCompressedID(&check_plain));
	check_plain.close();
}

TEST(ProjectSerializer, SaveToInvalidDirectoryFails)
{
	olive::ColorManager::SetUpDefaultConfig();
	bool created_disk_manager = false;
	EnsureDiskManager(&created_disk_manager);
	olive::ProjectSerializer::Initialize();

	olive::Project project;
	project.Initialize();

	olive::ProjectSerializer::Result result = olive::ProjectSerializer::Save(
		olive::ProjectSerializer::SaveData(
			olive::ProjectSerializer::kProject, &project,
			QStringLiteral("/definitely/nonexistent/project.ove")),
		false);

	EXPECT_EQ(result.code(), olive::ProjectSerializer::kFileError);
	EXPECT_FALSE(result.GetDetails().isEmpty());

	olive::ProjectSerializer::Destroy();
	ReleaseDiskManager(created_disk_manager);
}
