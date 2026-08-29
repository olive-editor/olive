#include <gtest/gtest.h>

#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QStackedWidget>

#include "core.h"
#include "node/output/viewer/viewer.h"
#include "olive/core/util/timecodefunctions.h"
#include "render/diskmanager.h"
#include "widget/playbackcontrols/playbackcontrols.h"
#include "widget/slider/base/sliderbase.h"
#include "widget/slider/base/sliderlabel.h"
#include "widget/slider/rationalslider.h"
#include "widget/timeruler/timeruler.h"

using namespace olive;

namespace
{

// PlaybackControls and playhead seeking talk to the Core singleton
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

TEST(TimeRuler, ConstructionWithAndWithoutDecorations)
{
	TimeRuler plain;
	EXPECT_EQ(plain.GetMarkers(), nullptr);
	EXPECT_EQ(plain.GetWorkArea(), nullptr);

	// Text hidden, cache status shown
	TimeRuler decorated(false, true);
	decorated.SetCenteredText(true);
	SUCCEED();
}

TEST(TimeRuler, TimebaseAndScaleDriveTimePixelConversion)
{
	TimeRuler ruler;

	// Without a timebase everything collapses to zero
	EXPECT_DOUBLE_EQ(ruler.TimeToScene(rational(1)), 0.0);

	ruler.SetTimebase(rational(1, 30));
	ruler.SetScale(100.0);

	// One second at scale 100 lands at scene x=100, half a second at 50
	EXPECT_DOUBLE_EQ(ruler.TimeToScene(rational(1)), 100.0);
	EXPECT_DOUBLE_EQ(ruler.TimeToScene(rational(1, 2)), 50.0);

	// Inverse conversion returns whole frames in the ruler's timebase
	EXPECT_EQ(ruler.SceneToTime(100.0), rational(1));
	EXPECT_EQ(ruler.SceneToTime(50.0), rational(1, 2));

	// Fractional positions floor to the frame below (or ceil when negative)
	EXPECT_EQ(ruler.SceneToTime(51.0), rational(1, 2));
	EXPECT_EQ(ruler.SceneToTime(-51.0), rational(-1, 2));

	// Rounding mode snaps to the nearest frame instead
	EXPECT_EQ(ruler.SceneToTime(51.0, true), rational(1, 2));
	EXPECT_EQ(ruler.SceneToTime(80.0, true), rational(4, 5));
}

TEST(TimeRuler, SeekToScenePointSeeksConnectedViewer)
{
	EnsureAppSingletons();

	TimeRuler ruler;
	ruler.SetTimebase(rational(1, 30));
	ruler.SetScale(100.0);

	ViewerOutput viewer;
	ruler.SetViewerNode(&viewer);

	ruler.SeekToScenePoint(150.0);
	EXPECT_EQ(viewer.GetPlayhead(), rational(3, 2));

	// Positions before zero clamp to zero
	viewer.SetPlayhead(rational(5));
	ruler.SeekToScenePoint(-50.0);
	EXPECT_EQ(viewer.GetPlayhead(), rational(0));
}

TEST(TimeRuler, SeekToScenePointWithoutTimebaseIsNoOp)
{
	// No timebase and no viewer: must return before touching either
	TimeRuler ruler;
	ruler.SeekToScenePoint(150.0);
	SUCCEED();
}

TEST(TimeRuler, SeekToScenePointWithoutViewerIsNoOp)
{
	EnsureAppSingletons();

	// Timebase set but no viewer connected: previously dereferenced a null
	// GetViewerNode() and crashed.
	TimeRuler ruler;
	ruler.SetTimebase(rational(1, 30));
	ruler.SetScale(100.0);

	ruler.SeekToScenePoint(150.0);
	SUCCEED();
}

class PlaybackControlsTest : public ::testing::Test {
protected:
	void SetUp() override
	{
		EnsureAppSingletons();
	}

	// The play/pause stacked widget (SliderBase is also a QStackedWidget
	// and must be filtered out)
	static QStackedWidget *PlayPauseStack(PlaybackControls *controls)
	{
		foreach (QStackedWidget *s, controls->findChildren<QStackedWidget *>()) {
			if (!qobject_cast<SliderBase *>(s)) {
				return s;
			}
		}
		return nullptr;
	}

	// The play/pause buttons live inside the play/pause stacked widget
	static void PlayPauseButtons(PlaybackControls *controls,
								 QPushButton **play_btn, QPushButton **pause_btn)
	{
		QStackedWidget *stack = PlayPauseStack(controls);
		*play_btn = qobject_cast<QPushButton *>(stack->widget(0));
		*pause_btn = qobject_cast<QPushButton *>(stack->widget(1));
	}

	// The remaining buttons in creation order: go-to-start, previous frame,
	// next frame, go-to-end, video drag, audio drag
	static QList<QPushButton *> NavigationButtons(PlaybackControls *controls)
	{
		QPushButton *play_btn;
		QPushButton *pause_btn;
		PlayPauseButtons(controls, &play_btn, &pause_btn);

		QList<QPushButton *> buttons;
		foreach (QPushButton *b, controls->findChildren<QPushButton *>()) {
			if (b != play_btn && b != pause_btn) {
				buttons.append(b);
			}
		}
		return buttons;
	}
};

TEST_F(PlaybackControlsTest, NullTimebaseDisablesWidget)
{
	PlaybackControls controls;
	EXPECT_FALSE(controls.isEnabled());

	controls.SetTimebase(rational(1, 30));
	EXPECT_TRUE(controls.isEnabled());

	controls.SetTimebase(rational());
	EXPECT_FALSE(controls.isEnabled());
}

TEST_F(PlaybackControlsTest, ButtonsEmitCorrespondingSignals)
{
	PlaybackControls controls;
	controls.SetTimebase(rational(1, 30));

	QPushButton *play_btn;
	QPushButton *pause_btn;
	PlayPauseButtons(&controls, &play_btn, &pause_btn);
	ASSERT_NE(play_btn, nullptr);
	ASSERT_NE(pause_btn, nullptr);

	const QList<QPushButton *> buttons = NavigationButtons(&controls);
	ASSERT_EQ(buttons.size(), 6);

	QSignalSpy begin_spy(&controls, &PlaybackControls::BeginClicked);
	QSignalSpy prev_spy(&controls, &PlaybackControls::PrevFrameClicked);
	QSignalSpy play_spy(&controls, &PlaybackControls::PlayClicked);
	QSignalSpy pause_spy(&controls, &PlaybackControls::PauseClicked);
	QSignalSpy next_spy(&controls, &PlaybackControls::NextFrameClicked);
	QSignalSpy end_spy(&controls, &PlaybackControls::EndClicked);
	QSignalSpy video_spy(&controls, &PlaybackControls::VideoClicked);
	QSignalSpy audio_spy(&controls, &PlaybackControls::AudioClicked);

	buttons.at(0)->click();
	EXPECT_EQ(begin_spy.count(), 1);

	buttons.at(1)->click();
	EXPECT_EQ(prev_spy.count(), 1);

	play_btn->click();
	EXPECT_EQ(play_spy.count(), 1);

	pause_btn->click();
	EXPECT_EQ(pause_spy.count(), 1);

	buttons.at(2)->click();
	EXPECT_EQ(next_spy.count(), 1);

	buttons.at(3)->click();
	EXPECT_EQ(end_spy.count(), 1);

	buttons.at(4)->click();
	EXPECT_EQ(video_spy.count(), 1);

	buttons.at(5)->click();
	EXPECT_EQ(audio_spy.count(), 1);
}

TEST_F(PlaybackControlsTest, SetTimeUpdatesCurrentTimecodeWithoutEmitting)
{
	PlaybackControls controls;
	controls.SetTimebase(rational(1, 30));

	auto *slider = controls.findChild<RationalSlider *>();
	ASSERT_NE(slider, nullptr);

	QSignalSpy time_spy(&controls, &PlaybackControls::TimeChanged);

	controls.SetTime(rational(3, 2));
	EXPECT_EQ(slider->GetValue(), rational(3, 2));

	// Programmatic updates must not feed back into TimeChanged
	EXPECT_EQ(time_spy.count(), 0);
}

TEST_F(PlaybackControlsTest, SetEndTimeFormatsEndTimecodeLabel)
{
	PlaybackControls controls;
	controls.SetTimebase(rational(1, 30));

	// The only plain QLabel is the end timecode; the current-time slider
	// uses a SliderLabel (a QLabel subclass) internally
	QLabel *end_label = nullptr;
	foreach (QLabel *l, controls.findChildren<QLabel *>()) {
		if (!qobject_cast<SliderLabel *>(l)) {
			end_label = l;
			break;
		}
	}
	ASSERT_NE(end_label, nullptr);

	controls.SetEndTime(rational(30));
	const QString expected = QString::fromStdString(
		core::Timecode::time_to_timecode(rational(30), rational(1, 30),
										 Core::instance()->GetTimecodeDisplay()));
	EXPECT_EQ(end_label->text(), expected);
}

TEST_F(PlaybackControlsTest, PlayPauseStackSwitchesVisibleButton)
{
	PlaybackControls controls;

	QStackedWidget *stack = PlayPauseStack(&controls);
	ASSERT_NE(stack, nullptr);

	QPushButton *play_btn;
	QPushButton *pause_btn;
	PlayPauseButtons(&controls, &play_btn, &pause_btn);
	ASSERT_NE(play_btn, nullptr);
	ASSERT_NE(pause_btn, nullptr);

	// The play button is the default page
	EXPECT_EQ(stack->currentWidget(), play_btn);

	controls.ShowPauseButton();
	EXPECT_EQ(stack->currentWidget(), pause_btn);

	controls.ShowPlayButton();
	EXPECT_EQ(stack->currentWidget(), play_btn);
}

TEST_F(PlaybackControlsTest, AudioVideoDragButtonsToggleVisibility)
{
	PlaybackControls controls;
	const QList<QPushButton *> buttons = NavigationButtons(&controls);
	ASSERT_EQ(buttons.size(), 6);

	// Hidden by default (constructor passes false)
	EXPECT_TRUE(buttons.at(4)->isHidden());
	EXPECT_TRUE(buttons.at(5)->isHidden());

	controls.SetAudioVideoDragButtonsVisible(true);
	EXPECT_FALSE(buttons.at(4)->isHidden());
	EXPECT_FALSE(buttons.at(5)->isHidden());
}
