#include <gtest/gtest.h>

#include <memory>

#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "core.h"
#include "dialog/footageproperties/footageproperties.h"
#include "dialog/footagerelink/footagerelinkdialog.h"
#include "dialog/keyframeproperties/keyframeproperties.h"
#include "dialog/markerproperties/markerpropertiesdialog.h"
#include "dialog/projectproperties/projectproperties.h"
#include "dialog/sequence/sequence.h"
#include "dialog/sequence/sequencedialogparametertab.h"
#include "dialog/sequence/sequencedialogpresettab.h"
#include "dialog/sequence/sequencepreset.h"
#include "dialog/speedduration/speeddurationdialog.h"
#include "node/block/clip/clip.h"
#include "node/color/colormanager/colormanager.h"
#include "node/math/math/math.h"
#include "node/output/track/track.h"
#include "node/project.h"
#include "node/project/footage/footage.h"
#include "node/project/sequence/sequence.h"
#include "render/diskmanager.h"
#include "timeline/timelinemarker.h"
#include "widget/colorlabelmenu/colorcodingcombobox.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/rationalslider.h"

namespace
{

// Several of these dialogs push undo commands to the global undo stack on
// accept(), which requires the Core singleton (see project_factory_test.cpp)
void EnsureAppSingletons()
{
	if (!olive::Core::instance()) {
		new olive::Core(olive::Core::CoreParams()); // intentionally leaked
	}
	if (!olive::DiskManager::instance()) {
		olive::DiskManager::CreateInstance();
	}
}

void ClearUndoStack()
{
	if (olive::Core::instance()) {
		olive::Core::instance()->undo_stack()->clear();
	}
}

std::unique_ptr<olive::Project> CreateProject()
{
	olive::ColorManager::SetUpDefaultConfig();

	auto project = std::make_unique<olive::Project>();
	project->Initialize();
	return project;
}

// Redirects QStandardPaths (used for the sequence preset file, which
// PresetManager rewrites on destruction) to a disposable test location
class StandardPathsTestModeGuard {
public:
	StandardPathsTestModeGuard()
	{
		QStandardPaths::setTestModeEnabled(true);
	}

	~StandardPathsTestModeGuard()
	{
		QStandardPaths::setTestModeEnabled(false);
	}
};

olive::ClipBlock *CreateClip(olive::Project *project,
							 const olive::rational &length)
{
	auto *clip = new olive::ClipBlock();
	clip->setParent(project);
	clip->set_length_and_media_out(length);
	return clip;
}

olive::Track *CreateTrackWithClip(olive::Project *project,
								  olive::ClipBlock *clip)
{
	auto *track = new olive::Track();
	track->setParent(project);
	track->AppendBlock(clip);
	return track;
}

} // namespace

//
// speedduration
//
TEST(DialogSpeedDuration, InitialValuesReflectSingleClip)
{
	EnsureAppSingletons();
	auto project = CreateProject();
	auto *clip = CreateClip(project.get(), olive::rational(4));
	CreateTrackWithClip(project.get(), clip);

	olive::SpeedDurationDialog dialog({ clip }, olive::rational(1, 24));

	auto *speed_slider = dialog.findChild<olive::FloatSlider *>();
	auto *dur_slider = dialog.findChild<olive::RationalSlider *>();
	ASSERT_NE(speed_slider, nullptr);
	ASSERT_NE(dur_slider, nullptr);

	EXPECT_DOUBLE_EQ(speed_slider->GetValue(), 1.0);
	EXPECT_EQ(dur_slider->GetValue(), olive::rational(4));
	EXPECT_FALSE(speed_slider->IsTristate());
	EXPECT_FALSE(dur_slider->IsTristate());
}

TEST(DialogSpeedDuration, LinkedSpeedChangeUpdatesDuration)
{
	EnsureAppSingletons();
	auto project = CreateProject();
	auto *clip = CreateClip(project.get(), olive::rational(4));
	CreateTrackWithClip(project.get(), clip);

	olive::SpeedDurationDialog dialog({ clip }, olive::rational(1, 24));

	auto *speed_slider = dialog.findChild<olive::FloatSlider *>();
	auto *dur_slider = dialog.findChild<olive::RationalSlider *>();

	// Programmatic SetValue() does not emit ValueChanged (only user edits
	// do), so emit the signal explicitly to drive the linked update
	speed_slider->SetValue(2.0);
	emit speed_slider->ValueChanged(2.0);
	EXPECT_EQ(dur_slider->GetValue(), olive::rational(2));

	speed_slider->SetValue(0.5);
	emit speed_slider->ValueChanged(0.5);
	EXPECT_EQ(dur_slider->GetValue(), olive::rational(8));
}

TEST(DialogSpeedDuration, LinkedDurationChangeUpdatesSpeed)
{
	EnsureAppSingletons();
	auto project = CreateProject();
	auto *clip = CreateClip(project.get(), olive::rational(4));
	CreateTrackWithClip(project.get(), clip);

	olive::SpeedDurationDialog dialog({ clip }, olive::rational(1, 24));

	auto *speed_slider = dialog.findChild<olive::FloatSlider *>();
	auto *dur_slider = dialog.findChild<olive::RationalSlider *>();

	// Programmatic SetValue() does not emit ValueChanged (only user edits
	// do), so emit the signal explicitly to drive the linked update
	dur_slider->SetValue(olive::rational(2));
	emit dur_slider->ValueChanged(olive::rational(2));
	EXPECT_DOUBLE_EQ(speed_slider->GetValue(), 2.0);

	dur_slider->SetValue(olive::rational(16));
	emit dur_slider->ValueChanged(olive::rational(16));
	EXPECT_DOUBLE_EQ(speed_slider->GetValue(), 0.25);
}

TEST(DialogSpeedDuration, AcceptAppliesSpeedAndLength)
{
	EnsureAppSingletons();
	auto project = CreateProject();
	auto *clip = CreateClip(project.get(), olive::rational(4));
	CreateTrackWithClip(project.get(), clip);

	{
		olive::SpeedDurationDialog dialog({ clip }, olive::rational(1, 24));

		// Doubling the speed with the link checked halves the duration
		auto *speed_slider = dialog.findChild<olive::FloatSlider *>();
		speed_slider->SetValue(2.0);
		emit speed_slider->ValueChanged(2.0);

		dialog.accept();
	}

	EXPECT_DOUBLE_EQ(clip->speed(), 2.0);
	EXPECT_EQ(clip->length(), olive::rational(2));

	ClearUndoStack();
}

TEST(DialogSpeedDuration, DifferingSpeedsAcrossClipsProduceTristate)
{
	EnsureAppSingletons();
	auto project = CreateProject();
	auto *clip_a = CreateClip(project.get(), olive::rational(4));
	auto *clip_b = CreateClip(project.get(), olive::rational(4));
	CreateTrackWithClip(project.get(), clip_a);
	clip_b->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);
	CreateTrackWithClip(project.get(), clip_b);

	olive::SpeedDurationDialog dialog({ clip_a, clip_b },
									  olive::rational(1, 24));

	EXPECT_TRUE(dialog.findChild<olive::FloatSlider *>()->IsTristate());
	// Durations are identical, so the duration slider must not be tristate
	EXPECT_FALSE(dialog.findChild<olive::RationalSlider *>()->IsTristate());
}

TEST(DialogSpeedDuration, AcceptDerivesPerClipSpeedFromDuration)
{
	EnsureAppSingletons();
	auto project = CreateProject();
	auto *clip_a = CreateClip(project.get(), olive::rational(4));
	auto *clip_b = CreateClip(project.get(), olive::rational(4));
	CreateTrackWithClip(project.get(), clip_a);
	clip_b->SetStandardValue(olive::ClipBlock::kSpeedInput, 2.0);
	CreateTrackWithClip(project.get(), clip_b);

	{
		olive::SpeedDurationDialog dialog({ clip_a, clip_b },
										  olive::rational(1, 24));
		// Speed is tristate, so accept() must compute each clip's speed
		// from its own length/speed ratio: speed = old_speed * old_len / new_len
		dialog.findChild<olive::RationalSlider *>()->SetValue(
			olive::rational(2));
		dialog.accept();
	}

	EXPECT_EQ(clip_a->length(), olive::rational(2));
	EXPECT_EQ(clip_b->length(), olive::rational(2));
	EXPECT_DOUBLE_EQ(clip_a->speed(), 2.0);
	EXPECT_DOUBLE_EQ(clip_b->speed(), 4.0);

	ClearUndoStack();
}

//
// keyframeproperties
//
TEST(DialogKeyframeProperties, SingleKeyAcceptWritesAllFields)
{
	EnsureAppSingletons();
	auto project = CreateProject();

	auto *node = new olive::MathNode();
	node->setParent(project.get());
	auto *key = new olive::NodeKeyframe(olive::rational(0), 1.0,
										olive::NodeKeyframe::kLinear, 0, -1,
										olive::MathNode::kParamAIn);
	key->setParent(node);

	// The dialog stores the keyframe vector by reference, so it must outlive it
	std::vector<olive::NodeKeyframe *> keys = { key };

	{
		olive::KeyframePropertiesDialog dialog(keys, olive::rational(1, 24));

		auto *time_slider = dialog.findChild<olive::RationalSlider *>();
		auto *type_select = dialog.findChild<QComboBox *>();
		auto *bezier_group = dialog.findChild<QGroupBox *>();
		ASSERT_NE(time_slider, nullptr);
		ASSERT_NE(type_select, nullptr);
		ASSERT_NE(bezier_group, nullptr);

		// Initial state reflects the keyframe
		EXPECT_TRUE(time_slider->isEnabled());
		EXPECT_EQ(time_slider->GetValue(), olive::rational(0));
		ASSERT_EQ(type_select->count(), 3);
		EXPECT_EQ(type_select->currentData().toInt(), olive::NodeKeyframe::kLinear);
		EXPECT_FALSE(bezier_group->isEnabled());

		// Switching to Bezier enables the bezier handle editors
		type_select->setCurrentIndex(2);
		ASSERT_EQ(type_select->currentData().toInt(),
				  olive::NodeKeyframe::kBezier);
		EXPECT_TRUE(bezier_group->isEnabled());

		time_slider->SetValue(olive::rational(1, 2));

		const QList<olive::FloatSlider *> sliders =
			dialog.findChildren<olive::FloatSlider *>();
		ASSERT_EQ(sliders.size(), 4);
		sliders.at(0)->SetValue(0.1); // bezier in x
		sliders.at(1)->SetValue(0.2); // bezier in y
		sliders.at(2)->SetValue(0.3); // bezier out x
		sliders.at(3)->SetValue(0.4); // bezier out y

		dialog.accept();
	}

	EXPECT_EQ(key->time(), olive::rational(1, 2));
	EXPECT_EQ(key->type(), olive::NodeKeyframe::kBezier);
	EXPECT_DOUBLE_EQ(key->bezier_control_in().x(), 0.1);
	EXPECT_DOUBLE_EQ(key->bezier_control_in().y(), 0.2);
	EXPECT_DOUBLE_EQ(key->bezier_control_out().x(), 0.3);
	EXPECT_DOUBLE_EQ(key->bezier_control_out().y(), 0.4);

	ClearUndoStack();
}

TEST(DialogKeyframeProperties, MixedTypesAddPlaceholderItem)
{
	EnsureAppSingletons();
	auto project = CreateProject();

	auto *node = new olive::MathNode();
	node->setParent(project.get());
	auto *key_a = new olive::NodeKeyframe(olive::rational(0), 1.0,
										  olive::NodeKeyframe::kLinear, 0, -1,
										  olive::MathNode::kParamAIn);
	auto *key_b = new olive::NodeKeyframe(olive::rational(1), 2.0,
										  olive::NodeKeyframe::kHold, 0, -1,
										  olive::MathNode::kParamAIn);
	key_a->setParent(node);
	key_b->setParent(node);

	// The dialog stores the keyframe vector by reference, so it must outlive it
	std::vector<olive::NodeKeyframe *> keys = { key_a, key_b };

	{
		olive::KeyframePropertiesDialog dialog(keys, olive::rational(1, 24));

		auto *type_select = dialog.findChild<QComboBox *>();
		// An "--" placeholder item with data -1 is prepended for mixed types
		ASSERT_EQ(type_select->count(), 4);
		EXPECT_EQ(type_select->itemData(0).toInt(), -1);
		EXPECT_EQ(type_select->currentIndex(), 0);

		dialog.accept();
	}

	// Accepting with the placeholder selected must not change key types
	EXPECT_EQ(key_a->type(), olive::NodeKeyframe::kLinear);
	EXPECT_EQ(key_b->type(), olive::NodeKeyframe::kHold);

	ClearUndoStack();
}

TEST(DialogKeyframeProperties, KeysOnSameTrackDisableTimeEdit)
{
	EnsureAppSingletons();
	auto project = CreateProject();

	auto *node = new olive::MathNode();
	node->setParent(project.get());
	auto *key_a = new olive::NodeKeyframe(olive::rational(0), 1.0,
										  olive::NodeKeyframe::kLinear, 0, -1,
										  olive::MathNode::kParamAIn);
	auto *key_b = new olive::NodeKeyframe(olive::rational(1), 2.0,
										  olive::NodeKeyframe::kLinear, 0, -1,
										  olive::MathNode::kParamAIn);
	key_a->setParent(node);
	key_b->setParent(node);

	// The dialog stores the keyframe vector by reference, so it must outlive it
	std::vector<olive::NodeKeyframe *> keys = { key_a, key_b };

	olive::KeyframePropertiesDialog dialog(keys, olive::rational(1, 24));

	// Moving two keys of the same track in time could reorder them, so the
	// time editor must be disabled
	EXPECT_FALSE(dialog.findChild<olive::RationalSlider *>()->isEnabled());
}

//
// markerproperties
//
TEST(DialogMarkerProperties, SingleMarkerAcceptWritesFields)
{
	EnsureAppSingletons();

	olive::TimelineMarker marker(
		3,
		olive::TimeRange(olive::rational(1), olive::rational(2)),
		QStringLiteral("Marker A"));

	{
		olive::MarkerPropertiesDialog dialog({ &marker },
											 olive::rational(1, 24));

		auto *label_edit = dialog.findChild<QLineEdit *>();
		auto *color_menu = dialog.findChild<olive::ColorCodingComboBox *>();
		const QList<olive::RationalSlider *> sliders =
			dialog.findChildren<olive::RationalSlider *>();
		ASSERT_NE(label_edit, nullptr);
		ASSERT_NE(color_menu, nullptr);
		ASSERT_EQ(sliders.size(), 2);

		EXPECT_EQ(label_edit->text(), QStringLiteral("Marker A"));
		EXPECT_EQ(color_menu->GetSelectedColor(), 3);
		EXPECT_EQ(sliders.at(0)->GetValue(), olive::rational(1));
		EXPECT_EQ(sliders.at(1)->GetValue(), olive::rational(2));

		label_edit->setText(QStringLiteral("Renamed"));
		sliders.at(0)->SetValue(olive::rational(1, 2));
		sliders.at(1)->SetValue(olive::rational(3, 2));

		dialog.accept();
	}

	EXPECT_EQ(marker.name(), QStringLiteral("Renamed"));
	EXPECT_EQ(marker.time().in(), olive::rational(1, 2));
	EXPECT_EQ(marker.time().out(), olive::rational(3, 2));
	EXPECT_EQ(marker.color(), 3);

	ClearUndoStack();
}

TEST(DialogMarkerProperties, MultipleMarkersDisableTimeAndShowPlaceholder)
{
	EnsureAppSingletons();

	olive::TimelineMarker marker_a(
		1,
		olive::TimeRange(olive::rational(1), olive::rational(2)),
		QStringLiteral("Alpha"));
	olive::TimelineMarker marker_b(
		2,
		olive::TimeRange(olive::rational(5), olive::rational(6)),
		QStringLiteral("Beta"));

	{
		olive::MarkerPropertiesDialog dialog({ &marker_a, &marker_b },
											 olive::rational(1, 24));

		auto *label_edit = dialog.findChild<QLineEdit *>();
		auto *color_menu = dialog.findChild<olive::ColorCodingComboBox *>();
		const QList<olive::RationalSlider *> sliders =
			dialog.findChildren<olive::RationalSlider *>();

		// Time cannot be edited for multiple markers
		EXPECT_FALSE(sliders.at(0)->isEnabled());
		EXPECT_TRUE(sliders.at(0)->IsTristate());
		EXPECT_FALSE(sliders.at(1)->isEnabled());
		EXPECT_TRUE(sliders.at(1)->IsTristate());

		// Differing names show a placeholder instead of text
		EXPECT_TRUE(label_edit->text().isEmpty());
		EXPECT_FALSE(label_edit->placeholderText().isEmpty());

		// Differing colors are represented by -1
		EXPECT_EQ(color_menu->GetSelectedColor(), -1);

		dialog.accept();
	}

	// Nothing should have been written back
	EXPECT_EQ(marker_a.name(), QStringLiteral("Alpha"));
	EXPECT_EQ(marker_b.name(), QStringLiteral("Beta"));
	EXPECT_EQ(marker_a.color(), 1);
	EXPECT_EQ(marker_b.color(), 2);

	ClearUndoStack();
}

//
// sequence
//
TEST(DialogSequencePreset, SaveLoadRoundTrip)
{
	olive::SequencePreset preset(QStringLiteral("Test Preset"), 1920, 1080,
								 olive::rational(24, 1), olive::rational(1, 1),
								 olive::VideoParams::kInterlacedTopFirst, 48000,
								 olive::core::kChannelLayoutStereo, 2,
								 olive::PixelFormat::F16, true);

	QByteArray xml;
	QBuffer buffer(&xml);
	buffer.open(QIODevice::WriteOnly);
	QXmlStreamWriter writer(&buffer);
	writer.writeStartDocument();
	writer.writeStartElement(QStringLiteral("preset"));
	preset.Save(&writer);
	writer.writeEndElement();
	writer.writeEndDocument();
	buffer.close();

	olive::SequencePreset loaded;
	QBuffer read_buffer(&xml);
	read_buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&read_buffer);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name().toString(), QStringLiteral("preset"));
	loaded.Load(&reader);

	EXPECT_EQ(loaded.GetName(), QStringLiteral("Test Preset"));
	EXPECT_EQ(loaded.width(), 1920);
	EXPECT_EQ(loaded.height(), 1080);
	EXPECT_EQ(loaded.frame_rate(), olive::rational(24, 1));
	EXPECT_EQ(loaded.pixel_aspect(), olive::rational(1, 1));
	EXPECT_EQ(loaded.interlacing(), olive::VideoParams::kInterlacedTopFirst);
	EXPECT_EQ(loaded.sample_rate(), 48000);
	EXPECT_EQ(loaded.channel_layout(), olive::core::kChannelLayoutStereo);
	EXPECT_EQ(loaded.preview_divider(), 2);
	EXPECT_EQ(loaded.preview_format(), olive::PixelFormat::F16);
	EXPECT_TRUE(loaded.preview_autocache());
}

TEST(DialogSequencePreset, LoadsLegacyInterlacingElement)
{
	// Older builds wrote the interlacing element as "interlacing_"; such
	// preset files must still load
	QByteArray xml = R"(<preset>
<name>Legacy</name>
<width>1280</width>
<height>720</height>
<framerate>24/1</framerate>
<pixelaspect>1/1</pixelaspect>
<interlacing_>2</interlacing_>
<samplerate>44100</samplerate>
<chlayout>3</chlayout>
<divider>1</divider>
<format>4</format>
<autocache>0</autocache>
</preset>)";

	olive::SequencePreset loaded;
	QBuffer read_buffer(&xml);
	read_buffer.open(QIODevice::ReadOnly);
	QXmlStreamReader reader(&read_buffer);
	ASSERT_TRUE(reader.readNextStartElement());
	ASSERT_EQ(reader.name().toString(), QStringLiteral("preset"));
	loaded.Load(&reader);

	EXPECT_EQ(loaded.GetName(), QStringLiteral("Legacy"));
	EXPECT_EQ(loaded.width(), 1280);
	EXPECT_EQ(loaded.interlacing(),
			  static_cast<olive::VideoParams::Interlacing>(2));
}

TEST(DialogSequenceParameterTab, ReflectsSequenceParameters)
{
	auto project = CreateProject();

	auto *sequence = new olive::Sequence();
	sequence->setParent(project.get());
	sequence->SetVideoParams(olive::VideoParams(
		1920, 1080, olive::rational(1001, 30000), olive::PixelFormat::F32,
		olive::VideoParams::kInternalChannelCount, olive::rational(1, 1),
		olive::VideoParams::kInterlaceNone, 2));
	sequence->SetAudioParams(olive::AudioParams(
		48000, olive::core::kChannelLayoutStereo,
		olive::Sequence::kDefaultSampleFormat));

	olive::SequenceDialogParameterTab tab(sequence);

	EXPECT_EQ(tab.GetSelectedVideoWidth(), 1920);
	EXPECT_EQ(tab.GetSelectedVideoHeight(), 1080);
	EXPECT_EQ(tab.GetSelectedVideoFrameRate(), olive::rational(30000, 1001));
	EXPECT_EQ(tab.GetSelectedVideoPixelAspect(), olive::rational(1, 1));
	EXPECT_EQ(tab.GetSelectedVideoInterlacingMode(),
			  olive::VideoParams::kInterlaceNone);
	EXPECT_EQ(tab.GetSelectedPreviewResolution(), 2);
	EXPECT_EQ(tab.GetSelectedPreviewFormat(), olive::PixelFormat::F32);
	EXPECT_EQ(tab.GetSelectedAudioSampleRate(), 48000);
	EXPECT_EQ(tab.GetSelectedAudioChannelLayout(),
			  olive::core::kChannelLayoutStereo);
}

TEST(DialogSequenceParameterTab, PresetChangedAppliesValues)
{
	auto project = CreateProject();

	auto *sequence = new olive::Sequence();
	sequence->setParent(project.get());
	sequence->SetVideoParams(olive::VideoParams(
		1920, 1080, olive::rational(1, 24), olive::PixelFormat::F32,
		olive::VideoParams::kInternalChannelCount, olive::rational(1, 1),
		olive::VideoParams::kInterlaceNone, 1));
	sequence->SetAudioParams(olive::AudioParams(
		48000, olive::core::kChannelLayoutStereo,
		olive::Sequence::kDefaultSampleFormat));

	olive::SequenceDialogParameterTab tab(sequence);

	tab.PresetChanged(olive::SequencePreset(
		QStringLiteral("Preset"), 1280, 720, olive::rational(24, 1),
		olive::rational(1, 1), olive::VideoParams::kInterlacedTopFirst, 44100,
		olive::core::kChannelLayoutStereo, 4, olive::PixelFormat::F16, false));

	EXPECT_EQ(tab.GetSelectedVideoWidth(), 1280);
	EXPECT_EQ(tab.GetSelectedVideoHeight(), 720);
	EXPECT_EQ(tab.GetSelectedVideoFrameRate(), olive::rational(24, 1));
	EXPECT_EQ(tab.GetSelectedVideoInterlacingMode(),
			  olive::VideoParams::kInterlacedTopFirst);
	EXPECT_EQ(tab.GetSelectedAudioSampleRate(), 44100);
	EXPECT_EQ(tab.GetSelectedPreviewResolution(), 4);
	EXPECT_EQ(tab.GetSelectedPreviewFormat(), olive::PixelFormat::F16);
}

TEST(DialogSequenceDialog, AcceptNonUndoableAppliesParameters)
{
	StandardPathsTestModeGuard test_mode;
	EnsureAppSingletons();
	auto project = CreateProject();

	auto *sequence = new olive::Sequence();
	sequence->setParent(project.get());
	sequence->SetLabel(QStringLiteral("Seq A"));
	sequence->SetVideoParams(olive::VideoParams(
		1920, 1080, olive::rational(1, 24), olive::PixelFormat::F32,
		olive::VideoParams::kInternalChannelCount, olive::rational(1, 1),
		olive::VideoParams::kInterlaceNone, 1));
	sequence->SetAudioParams(olive::AudioParams(
		48000, olive::core::kChannelLayoutStereo,
		olive::Sequence::kDefaultSampleFormat));

	{
		olive::SequenceDialog dialog(sequence, olive::SequenceDialog::kExisting);
		dialog.SetUndoable(false);

		auto *tab = dialog.findChild<olive::SequenceDialogParameterTab *>();
		ASSERT_NE(tab, nullptr);
		tab->PresetChanged(olive::SequencePreset(
			QStringLiteral("Preset"), 1280, 720, olive::rational(24, 1),
			olive::rational(1, 1), olive::VideoParams::kInterlacedTopFirst,
			44100, olive::core::kChannelLayoutStereo, 4,
			olive::PixelFormat::F32, false));

		auto *name_field = dialog.findChild<QLineEdit *>();
		ASSERT_NE(name_field, nullptr);
		name_field->setText(QStringLiteral("Seq B"));

		dialog.accept();
		EXPECT_EQ(dialog.result(), QDialog::Accepted);
	}

	EXPECT_EQ(sequence->GetLabel(), QStringLiteral("Seq B"));
	EXPECT_EQ(sequence->GetVideoParams().width(), 1280);
	EXPECT_EQ(sequence->GetVideoParams().height(), 720);
	EXPECT_EQ(sequence->GetVideoParams().frame_rate(), olive::rational(24, 1));
	EXPECT_EQ(sequence->GetVideoParams().interlacing(),
			  olive::VideoParams::kInterlacedTopFirst);
	EXPECT_EQ(sequence->GetVideoParams().divider(), 4);
	EXPECT_EQ(sequence->GetAudioParams().sample_rate(), 44100);
}

TEST(DialogSequenceDialog, AcceptUndoablePushesCommand)
{
	StandardPathsTestModeGuard test_mode;
	EnsureAppSingletons();
	auto project = CreateProject();

	auto *sequence = new olive::Sequence();
	sequence->setParent(project.get());
	sequence->SetLabel(QStringLiteral("Seq A"));
	sequence->SetVideoParams(olive::VideoParams(
		1920, 1080, olive::rational(1, 24), olive::PixelFormat::F32,
		olive::VideoParams::kInternalChannelCount, olive::rational(1, 1),
		olive::VideoParams::kInterlaceNone, 1));
	sequence->SetAudioParams(olive::AudioParams(
		48000, olive::core::kChannelLayoutStereo,
		olive::Sequence::kDefaultSampleFormat));

	{
		olive::SequenceDialog dialog(sequence, olive::SequenceDialog::kExisting);

		auto *tab = dialog.findChild<olive::SequenceDialogParameterTab *>();
		ASSERT_NE(tab, nullptr);
		tab->PresetChanged(olive::SequencePreset(
			QStringLiteral("Preset"), 640, 360, olive::rational(24, 1),
			olive::rational(1, 1), olive::VideoParams::kInterlaceNone, 48000,
			olive::core::kChannelLayoutStereo, 1, olive::PixelFormat::F32,
			false));

		dialog.accept();
		EXPECT_EQ(dialog.result(), QDialog::Accepted);
	}

	EXPECT_EQ(sequence->GetVideoParams().width(), 640);
	EXPECT_EQ(sequence->GetVideoParams().height(), 360);

	ClearUndoStack();
}

TEST(DialogSequenceDialog, PresetTabListsDefaultPresets)
{
	StandardPathsTestModeGuard test_mode;
	EnsureAppSingletons();
	auto project = CreateProject();

	auto *sequence = new olive::Sequence();
	sequence->setParent(project.get());

	olive::SequenceDialog dialog(sequence, olive::SequenceDialog::kExisting);

	auto *tree = dialog.findChild<QTreeWidget *>();
	ASSERT_NE(tree, nullptr);
	// "My Presets" plus the standard preset folders
	EXPECT_GE(tree->topLevelItemCount(), 4);
}

//
// footageproperties
//
TEST(DialogFootageProperties, AcceptRenamesAndSetsSourceStartTime)
{
	EnsureAppSingletons();
	auto project = CreateProject();

	auto *footage = new olive::Footage();
	footage->setParent(project.get());
	footage->SetLabel(QStringLiteral("Clip A"));
	footage->set_filename(QStringLiteral("/tmp/oak-nonexistent.mp4"));

	{
		olive::FootagePropertiesDialog dialog(nullptr, footage);

		auto *name_field = dialog.findChild<QLineEdit *>();
		auto *start_enable = dialog.findChild<QCheckBox *>();
		auto *start_spin = dialog.findChild<QDoubleSpinBox *>();
		ASSERT_NE(name_field, nullptr);
		ASSERT_NE(start_enable, nullptr);
		ASSERT_NE(start_spin, nullptr);

		EXPECT_EQ(name_field->text(), QStringLiteral("Clip A"));
		EXPECT_FALSE(start_enable->isChecked());
		EXPECT_FALSE(start_spin->isEnabled());

		name_field->setText(QStringLiteral("Clip B"));
		start_enable->setChecked(true);
		EXPECT_TRUE(start_spin->isEnabled());
		start_spin->setValue(12.5);

		// accept() is a private slot, invoke it through the meta-object
		QMetaObject::invokeMethod(&dialog, "accept");
	}

	EXPECT_EQ(footage->GetLabel(), QStringLiteral("Clip B"));
	EXPECT_TRUE(footage->HasSourceStartTime());
	EXPECT_EQ(footage->source_start_time(), olive::rational(25, 2));
	EXPECT_EQ(footage->source_start_time_source(), QStringLiteral("manual"));

	{
		// Unchecking the box must clear the source start time again
		olive::FootagePropertiesDialog dialog(nullptr, footage);

		auto *start_enable = dialog.findChild<QCheckBox *>();
		ASSERT_NE(start_enable, nullptr);
		EXPECT_TRUE(start_enable->isChecked());
		start_enable->setChecked(false);

		// accept() is a private slot, invoke it through the meta-object
		QMetaObject::invokeMethod(&dialog, "accept");
	}

	EXPECT_FALSE(footage->HasSourceStartTime());

	ClearUndoStack();
}

//
// footagerelink
//
TEST(DialogFootageRelink, TableListsFootageAndFilenames)
{
	auto project = CreateProject();

	auto *footage_a = new olive::Footage();
	footage_a->setParent(project.get());
	footage_a->SetLabel(QStringLiteral("Footage A"));
	footage_a->set_filename(QStringLiteral("/old/path/a.mp4"));

	auto *footage_b = new olive::Footage();
	footage_b->setParent(project.get());
	footage_b->SetLabel(QStringLiteral("Footage B"));
	footage_b->set_filename(QStringLiteral("/old/path/b.mp4"));

	olive::FootageRelinkDialog dialog({ footage_a, footage_b });

	auto *table = dialog.findChild<QTreeWidget *>();
	ASSERT_NE(table, nullptr);
	ASSERT_EQ(table->topLevelItemCount(), 2);
	EXPECT_EQ(table->topLevelItem(0)->text(0), QStringLiteral("Footage A"));
	EXPECT_EQ(table->topLevelItem(0)->text(1),
			  QStringLiteral("/old/path/a.mp4"));
	EXPECT_EQ(table->topLevelItem(1)->text(0), QStringLiteral("Footage B"));
	EXPECT_EQ(table->topLevelItem(1)->text(1),
			  QStringLiteral("/old/path/b.mp4"));
}

//
// projectproperties
//
TEST(DialogProjectProperties, OcioValidationTogglesOnInvalidFilename)
{
	EnsureAppSingletons();
	auto project = CreateProject();

	olive::ProjectPropertiesDialog dialog(project.get(), nullptr);

	EXPECT_TRUE(dialog.windowTitle().contains(project->name()));

	// The first QLineEdit in the dialog is the OCIO config filename
	auto *ocio_edit = dialog.findChild<QLineEdit *>();
	ASSERT_NE(ocio_edit, nullptr);

	// A bad config path flags the line edit as invalid (red text). Use a
	// nonexistent file inside a temp dir: a fixed absolute path is not
	// guaranteed to stay nonexistent on every platform (e.g. an earlier test
	// may have created parts of it on a writable drive).
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());
	ocio_edit->setText(dir.filePath(QStringLiteral("nonexistent.ocio")));
	EXPECT_TRUE(ocio_edit->styleSheet().contains(QStringLiteral("red")));

	// Restoring an empty (default) filename clears the error again
	ocio_edit->setText(QString());
	EXPECT_TRUE(ocio_edit->styleSheet().isEmpty());

	// The default config provides selectable input color spaces
	bool found_populated_combo = false;
	foreach (QComboBox *combo, dialog.findChildren<QComboBox *>()) {
		if (combo->count() > 2) {
			found_populated_combo = true;
			break;
		}
	}
	EXPECT_TRUE(found_populated_combo);
}

TEST(DialogProjectProperties, AcceptWithDefaultsClosesDialog)
{
	EnsureAppSingletons();
	auto project = CreateProject();

	olive::ProjectPropertiesDialog dialog(project.get(), nullptr);
	dialog.accept();

	EXPECT_EQ(dialog.result(), QDialog::Accepted);
}
