#include <gtest/gtest.h>

#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QRadioButton>
#include <QSignalSpy>
#include <QStyleOptionSlider>
#include <QTemporaryDir>
#include <QTest>
#include <QWheelEvent>

#include "config/config.h"
#include "core.h"
#include "node/globals.h"
#include "node/math/math/math.h"
#include "node/project.h"
#include "node/traverser.h"
#include "ui/colorcoding.h"
#include "ui/icons/icons.h"
#include "widget/bezier/bezierwidget.h"
#include "widget/clickablelabel/clickablelabel.h"
#include "widget/collapsebutton/collapsebutton.h"
#include "widget/colorbutton/colorbutton.h"
#include "widget/colorlabelmenu/colorlabelmenu.h"
#include "widget/colorwheel/colorgradientwidget.h"
#include "widget/colorwheel/colorpreviewbox.h"
#include "widget/colorwheel/colorspacechooser.h"
#include "widget/colorwheel/colorswatchchooser.h"
#include "widget/colorwheel/colorvalueswidget.h"
#include "widget/colorwheel/colorwheelwidget.h"
#include "widget/filefield/filefield.h"
#include "widget/focusablelineedit/focusablelineedit.h"
#include "widget/handmovableview/handmovableview.h"
#include "widget/menu/menu.h"
#include "widget/nodevaluetree/nodevaluetree.h"
#include "widget/path/pathwidget.h"
#include "widget/pixelsampler/pixelsampler.h"
#include "widget/resizablescrollbar/resizablescrollbar.h"
#include "widget/slider/stringslider.h"
#include "widget/toolbar/toolbar.h"
#include "widget/toolbar/toolbarbutton.h"

namespace
{

// Widgets that connect to Core::instance() at construction require the
// application singleton, but not a MainWindow
void EnsureCore()
{
	if (!olive::Core::instance()) {
		new olive::Core(olive::Core::CoreParams()); // intentionally leaked
	}
}

// Exposes the protected slider rect calculation so tests can aim mouse events
// at the resize handles deterministically
class ProbeScrollBar : public olive::ResizableScrollBar {
public:
	explicit ProbeScrollBar(Qt::Orientation orientation)
		: olive::ResizableScrollBar(orientation)
	{
	}

	QRect SliderRect()
	{
		QStyleOptionSlider opt;
		initStyleOption(&opt);
		return style()->subControlRect(QStyle::CC_ScrollBar, &opt,
									   QStyle::SC_ScrollBarSlider, this);
	}
};

// Exposes the protected hand-drag state machine entry points
class ProbeHandView : public olive::HandMovableView {
public:
	bool PubHandPress(QMouseEvent *e)
	{
		return HandPress(e);
	}
	bool PubHandMove(QMouseEvent *e)
	{
		return HandMove(e);
	}
	bool PubHandRelease(QMouseEvent *e)
	{
		return HandRelease(e);
	}
	void PubSetDefaultDragMode(DragMode mode)
	{
		SetDefaultDragMode(mode);
	}
	const DragMode &PubGetDefaultDragMode() const
	{
		return GetDefaultDragMode();
	}
};

// ToolbarButton has no Q_OBJECT, so findChildren<ToolbarButton*> doesn't
// compile; every button in a Toolbar is a ToolbarButton, so fetch QPushButtons
// and static_cast
QList<olive::ToolbarButton *> ToolbarButtons(olive::Toolbar *bar)
{
	QList<olive::ToolbarButton *> out;
	for (QPushButton *b : bar->findChildren<QPushButton *>()) {
		out.append(static_cast<olive::ToolbarButton *>(b));
	}
	return out;
}

// Minimal node that pushes a float and an integer row so NodeValueTree has
// more than one value to choose from
class TwoValueNode : public olive::Node {
public:
	TwoValueNode() = default;

	NODE_DEFAULT_FUNCTIONS(TwoValueNode)

	virtual QString Name() const override
	{
		return QStringLiteral("Test Two Value");
	}

	virtual QString id() const override
	{
		return QStringLiteral("org.oak.test.twovalue");
	}

	virtual QVector<CategoryID> Category() const override
	{
		return { kCategoryMath };
	}

	virtual void Value(const olive::NodeValueRow &value,
					   const olive::NodeGlobals &globals,
					   olive::NodeValueTable *table) const override
	{
		Q_UNUSED(value)
		Q_UNUSED(globals)

		table->Push(olive::NodeValue::kFloat, QVariant(1.5), this);
		table->Push(olive::NodeValue::kInt, QVariant(2), this);
	}
};

} // namespace

TEST(WidgetMenu, InsertAlphabeticallySortsActions)
{
	olive::Menu menu;
	menu.InsertAlphabetically(QStringLiteral("Charlie"));
	menu.InsertAlphabetically(QStringLiteral("Alpha"));
	menu.InsertAlphabetically(QStringLiteral("Bravo"));

	ASSERT_EQ(menu.actions().size(), 3);
	EXPECT_EQ(menu.actions().at(0)->text(), QStringLiteral("Alpha"));
	EXPECT_EQ(menu.actions().at(1)->text(), QStringLiteral("Bravo"));
	EXPECT_EQ(menu.actions().at(2)->text(), QStringLiteral("Charlie"));

	// Submenus slot in by their title too
	auto *sub = new olive::Menu(&menu);
	sub->setTitle(QStringLiteral("Aardvark"));
	menu.InsertAlphabetically(sub);

	ASSERT_EQ(menu.actions().size(), 4);
	EXPECT_EQ(menu.actions().at(0)->text(), QStringLiteral("Aardvark"));
	EXPECT_EQ(menu.actions().at(0)->menu(), sub);
}

TEST(WidgetMenu, AddActionWithDataChecksMatchingValue)
{
	olive::Menu menu;
	QAction *match = menu.AddActionWithData(QStringLiteral("Five"), 5, 5);
	QAction *other = menu.AddActionWithData(QStringLiteral("Six"), 6, 5);

	EXPECT_TRUE(match->isCheckable());
	EXPECT_TRUE(match->isChecked());
	EXPECT_EQ(match->data().toInt(), 5);

	EXPECT_TRUE(other->isCheckable());
	EXPECT_FALSE(other->isChecked());
	EXPECT_EQ(other->data().toInt(), 6);
}

TEST(WidgetMenu, ConformItemStoresIdAndKeyDefault)
{
	QAction a;
	olive::Menu::ConformItem(&a, QStringLiteral("myaction"),
							 QKeySequence(QStringLiteral("Ctrl+K")));

	EXPECT_EQ(a.property("id").toString(), QStringLiteral("myaction"));
	EXPECT_EQ(a.shortcut(), QKeySequence(QStringLiteral("Ctrl+K")));
	EXPECT_EQ(a.property("keydefault").value<QKeySequence>(),
			  QKeySequence(QStringLiteral("Ctrl+K")));
	EXPECT_EQ(a.shortcutContext(), Qt::ApplicationShortcut);

	// Without a key, no keydefault is stored
	QAction b;
	olive::Menu::ConformItem(&b, QStringLiteral("plain"));
	EXPECT_EQ(b.property("id").toString(), QStringLiteral("plain"));
	EXPECT_FALSE(b.property("keydefault").isValid());
}

TEST(WidgetColorLabelMenu, ItemsCarryIndexAndEmitSelection)
{
	olive::ColorLabelMenu menu;
	const int color_count = int(olive::ColorCoding::standard_colors().size());
	ASSERT_GE(color_count, 3);
	ASSERT_EQ(menu.actions().size(), color_count);

	for (int i = 0; i < color_count; i++) {
		QAction *a = menu.actions().at(i);
		EXPECT_EQ(a->data().toInt(), i);
		EXPECT_EQ(a->text(), olive::ColorCoding::GetColorName(i));
		EXPECT_EQ(a->property("id").toString(),
				  QStringLiteral("colorlabel%1").arg(i));
	}

	QSignalSpy spy(&menu, &olive::ColorLabelMenu::ColorSelected);
	menu.actions().at(2)->trigger();
	ASSERT_EQ(spy.count(), 1);
	EXPECT_EQ(spy.first().first().toInt(), 2);
}

TEST(WidgetFileField, SetFilenameReadbackDoesNotSignal)
{
	olive::FileField field;
	QSignalSpy spy(&field, &olive::FileField::FilenameChanged);

	field.SetFilename(QStringLiteral("/some/file.txt"));
	EXPECT_EQ(field.GetFilename(), QStringLiteral("/some/file.txt"));

	// Programmatic changes don't count as user edits
	EXPECT_EQ(spy.count(), 0);
}

TEST(WidgetFileField, TypingEmitsFilenameChanged)
{
	olive::FileField field;
	QLineEdit *edit = field.findChild<QLineEdit *>();
	ASSERT_NE(edit, nullptr);

	QSignalSpy spy(&field, &olive::FileField::FilenameChanged);
	QTest::keyClicks(edit, QStringLiteral("a"));

	ASSERT_EQ(spy.count(), 1);
	EXPECT_EQ(spy.first().first().toString(), QStringLiteral("a"));
	EXPECT_EQ(field.GetFilename(), QStringLiteral("a"));
}

TEST(WidgetFileField, InvalidPathMarkedRed)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const QString existing =
		QDir(dir.path()).filePath(QStringLiteral("f.txt"));
	ASSERT_TRUE(QFile(existing).open(QIODevice::WriteOnly));

	olive::FileField field;
	QLineEdit *edit = field.findChild<QLineEdit *>();
	ASSERT_NE(edit, nullptr);

	edit->setText(existing);
	EXPECT_TRUE(edit->styleSheet().isEmpty());

	edit->setText(QStringLiteral("/definitely/not/here.xyz"));
	EXPECT_TRUE(edit->styleSheet().contains(QStringLiteral("red")));

	// An empty field is neutral again
	edit->setText(QString());
	EXPECT_TRUE(edit->styleSheet().isEmpty());
}

TEST(WidgetPathWidget, ReadbackAndDirectoryValidation)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	olive::PathWidget w(dir.path());
	EXPECT_EQ(w.text(), dir.path());

	QLineEdit *edit = w.findChild<QLineEdit *>();
	ASSERT_NE(edit, nullptr);

	edit->setText(QStringLiteral("/definitely/not/a/dir"));
	EXPECT_TRUE(edit->styleSheet().contains(QStringLiteral("red")));

	edit->setText(dir.path());
	EXPECT_TRUE(edit->styleSheet().isEmpty());
}

TEST(WidgetCollapseButton, ToggleSwitchesIcon)
{
	// Icons are normally loaded by the app style; pull them in explicitly
	olive::icon::LoadAll(QStringLiteral(":/style/olive-dark"));

	olive::CollapseButton btn;
	EXPECT_TRUE(btn.isCheckable());
	EXPECT_TRUE(btn.isChecked());
	EXPECT_FALSE(btn.icon().isNull());

	const qint64 expanded_key = btn.icon().cacheKey();
	btn.setChecked(false);
	EXPECT_FALSE(btn.icon().isNull());
	EXPECT_NE(btn.icon().cacheKey(), expanded_key);
}

TEST(WidgetClickableLabel, ClickAndDoubleClickSignals)
{
	olive::ClickableLabel label(QStringLiteral("Click me"));
	label.resize(120, 40);
	label.show();
	EXPECT_TRUE(QTest::qWaitForWindowExposed(&label));

	// mouseReleaseEvent requires the cursor to be over the widget; if the
	// offscreen platform can't track the cursor there's nothing to assert
	QTest::mouseMove(&label, QPoint(10, 10));
	if (!label.underMouse()) {
		GTEST_SKIP() << "Platform does not track cursor position";
	}

	QSignalSpy clicked_spy(&label, &olive::ClickableLabel::MouseClicked);
	QSignalSpy dbl_spy(&label, &olive::ClickableLabel::MouseDoubleClicked);

	QTest::mouseClick(&label, Qt::LeftButton);
	EXPECT_EQ(clicked_spy.count(), 1);
	EXPECT_EQ(dbl_spy.count(), 0);

	QTest::mouseDClick(&label, Qt::LeftButton);
	EXPECT_GE(dbl_spy.count(), 1);
}

TEST(WidgetFocusableLineEdit, EnterConfirmsEscapeCancels)
{
	olive::FocusableLineEdit edit;
	QSignalSpy confirmed(&edit, &olive::FocusableLineEdit::Confirmed);
	QSignalSpy cancelled(&edit, &olive::FocusableLineEdit::Cancelled);

	QTest::keyClick(&edit, Qt::Key_Return);
	EXPECT_EQ(confirmed.count(), 1);
	EXPECT_EQ(cancelled.count(), 0);

	QTest::keyClick(&edit, Qt::Key_Enter);
	EXPECT_EQ(confirmed.count(), 2);

	QTest::keyClick(&edit, Qt::Key_Escape);
	EXPECT_EQ(cancelled.count(), 1);

	// Ordinary keys pass through to QLineEdit
	QTest::keyClick(&edit, Qt::Key_A);
	EXPECT_EQ(edit.text(), QStringLiteral("a"));
	EXPECT_EQ(confirmed.count(), 2);
	EXPECT_EQ(cancelled.count(), 1);
}

TEST(WidgetPixelSampler, LabelShowsColorComponents)
{
	olive::PixelSamplerWidget w;
	QLabel *label = w.findChild<QLabel *>();
	ASSERT_NE(label, nullptr);

	w.SetValues(olive::Color(1.0, 0.5, 0.0, 1.0));
	const QString text = label->text();
	EXPECT_TRUE(text.contains(QStringLiteral("R: 1 (255)")));
	EXPECT_TRUE(text.contains(QStringLiteral("G: 0.5 (127)")));
	EXPECT_TRUE(text.contains(QStringLiteral("B: 0 (0)")));
	EXPECT_TRUE(text.contains(QStringLiteral("A: 1 (255)")));
}

TEST(WidgetPixelSampler, ManagedSamplerForwardsValues)
{
	olive::ManagedPixelSamplerWidget w;
	const auto samplers = w.findChildren<olive::PixelSamplerWidget *>();
	ASSERT_EQ(samplers.size(), 2);

	// First child is the display view, second the reference view
	w.SetValues(olive::Color(1.0, 0.0, 0.0, 1.0), olive::Color(0.0, 1.0, 0.0, 1.0));

	EXPECT_TRUE(samplers.at(0)->findChild<QLabel *>()->text().contains(
		QStringLiteral("G: 1 (255)")));
	EXPECT_TRUE(samplers.at(1)->findChild<QLabel *>()->text().contains(
		QStringLiteral("R: 1 (255)")));
}

TEST(WidgetBezierWidget, ValueRoundTripsThroughSliders)
{
	olive::BezierWidget w;

	olive::Bezier b(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
	w.SetValue(b);

	olive::Bezier out = w.GetValue();
	EXPECT_DOUBLE_EQ(out.x(), 1.0);
	EXPECT_DOUBLE_EQ(out.y(), 2.0);
	EXPECT_DOUBLE_EQ(out.cp1_x(), 3.0);
	EXPECT_DOUBLE_EQ(out.cp1_y(), 4.0);
	EXPECT_DOUBLE_EQ(out.cp2_x(), 5.0);
	EXPECT_DOUBLE_EQ(out.cp2_y(), 6.0);

	EXPECT_DOUBLE_EQ(w.x_slider()->GetValue(), 1.0);
	EXPECT_DOUBLE_EQ(w.y_slider()->GetValue(), 2.0);
	EXPECT_DOUBLE_EQ(w.cp1_x_slider()->GetValue(), 3.0);
	EXPECT_DOUBLE_EQ(w.cp2_y_slider()->GetValue(), 6.0);
}

TEST(WidgetResizableScrollBar, DefaultsMatchInit)
{
	olive::ResizableScrollBar bar;
	EXPECT_EQ(bar.singleStep(), 20);
	EXPECT_EQ(bar.maximum(), 0);
	EXPECT_TRUE(bar.hasMouseTracking());

	olive::ResizableScrollBar hbar(Qt::Horizontal);
	EXPECT_EQ(hbar.orientation(), Qt::Horizontal);
}

TEST(WidgetResizableScrollBar, HandleDragEmitsResizeSignals)
{
	ProbeScrollBar bar(Qt::Horizontal);
	bar.resize(300, 20);
	bar.setRange(0, 1000);
	bar.setPageStep(200);
	bar.setValue(500);
	bar.show();
	EXPECT_TRUE(QTest::qWaitForWindowExposed(&bar));

	const QRect slider = bar.SliderRect();
	ASSERT_GT(slider.width(), 30)
		<< "slider too small to hold two handles and a middle";

	QSignalSpy began(&bar, &olive::ResizableScrollBar::ResizeBegan);
	QSignalSpy moved(&bar, &olive::ResizableScrollBar::ResizeMoved);
	QSignalSpy ended(&bar, &olive::ResizableScrollBar::ResizeEnded);

	// Hover the top (left) handle, then drag it 25px to the right
	const QPoint handle_pos(slider.left() + 1, slider.center().y());
	QTest::mouseMove(&bar, handle_pos);
	QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, handle_pos);
	ASSERT_EQ(began.count(), 1);
	EXPECT_EQ(began.first().at(0).toInt(), slider.width());
	EXPECT_TRUE(began.first().at(1).toBool());

	QTest::mouseMove(&bar, handle_pos + QPoint(25, 0));
	ASSERT_EQ(moved.count(), 1);
	EXPECT_EQ(moved.first().first().toInt(), 25);

	QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier,
						handle_pos + QPoint(25, 0));
	EXPECT_EQ(ended.count(), 1);

	// The bottom (right) handle reports top_handle=false
	const QPoint bottom_handle(slider.right() - 1, slider.center().y());
	QTest::mouseMove(&bar, bottom_handle);
	QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, bottom_handle);
	ASSERT_EQ(began.count(), 2);
	EXPECT_FALSE(began.at(1).at(1).toBool());
	QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier, bottom_handle);
	EXPECT_EQ(ended.count(), 2);

	// Pressing the middle of the slider behaves like a normal scrollbar
	const QPoint middle = slider.center();
	QTest::mouseMove(&bar, middle);
	QTest::mousePress(&bar, Qt::LeftButton, Qt::NoModifier, middle);
	EXPECT_EQ(began.count(), 2);
	QTest::mouseRelease(&bar, Qt::LeftButton, Qt::NoModifier, middle);
	EXPECT_EQ(ended.count(), 2);
}

TEST(WidgetHandMovableView, ToolSwitchChangesDragMode)
{
	EnsureCore();

	ProbeHandView view;
	view.PubSetDefaultDragMode(QGraphicsView::RubberBandDrag);
	EXPECT_EQ(view.dragMode(), QGraphicsView::RubberBandDrag);
	EXPECT_EQ(view.PubGetDefaultDragMode(), QGraphicsView::RubberBandDrag);

	olive::Core::instance()->SetTool(olive::Tool::kHand);
	EXPECT_EQ(view.dragMode(), QGraphicsView::ScrollHandDrag);
	EXPECT_FALSE(view.isInteractive());

	// Restore the previous tool state for other tests
	olive::Core::instance()->SetTool(olive::Tool::kPointer);
	EXPECT_EQ(view.dragMode(), QGraphicsView::RubberBandDrag);
	EXPECT_TRUE(view.isInteractive());
}

TEST(WidgetHandMovableView, MiddleButtonHandDragStateMachine)
{
	EnsureCore();

	ProbeHandView view;
	view.resize(200, 100);
	view.PubSetDefaultDragMode(QGraphicsView::NoDrag);

	// Left button is not a hand drag
	QMouseEvent left_press(QEvent::MouseButtonPress, QPointF(10, 10),
						   QPointF(10, 10), QPointF(10, 10), Qt::LeftButton,
						   Qt::LeftButton, Qt::NoModifier);
	EXPECT_FALSE(view.PubHandPress(&left_press));
	EXPECT_TRUE(view.isInteractive());

	// Middle button starts a hand drag
	QMouseEvent mid_press(QEvent::MouseButtonPress, QPointF(10, 10),
						  QPointF(10, 10), QPointF(10, 10), Qt::MiddleButton,
						  Qt::MiddleButton, Qt::NoModifier);
	EXPECT_TRUE(view.PubHandPress(&mid_press));
	EXPECT_EQ(view.dragMode(), QGraphicsView::ScrollHandDrag);
	EXPECT_FALSE(view.isInteractive());

	QMouseEvent move(QEvent::MouseMove, QPointF(30, 20), QPointF(30, 20),
					 QPointF(30, 20), Qt::NoButton, Qt::MiddleButton,
					 Qt::NoModifier);
	EXPECT_TRUE(view.PubHandMove(&move));

	// Release restores the pre-drag state
	QMouseEvent release(QEvent::MouseButtonRelease, QPointF(30, 20),
						QPointF(30, 20), QPointF(30, 20), Qt::MiddleButton,
						Qt::NoButton, Qt::NoModifier);
	EXPECT_TRUE(view.PubHandRelease(&release));
	EXPECT_TRUE(view.isInteractive());
	EXPECT_EQ(view.dragMode(), QGraphicsView::NoDrag);

	// Without an active hand drag, move/release are ignored
	EXPECT_FALSE(view.PubHandMove(&move));
	EXPECT_FALSE(view.PubHandRelease(&release));
}

TEST(WidgetHandMovableView, WheelZoomHelpers)
{
	const QVariant old_scroll_zooms =
		olive::Config::Current()[QStringLiteral("ScrollZooms")];

	QWheelEvent plain(QPointF(5, 5), QPointF(5, 5), QPoint(), QPoint(0, 120),
					  Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
	QWheelEvent ctrl(QPointF(5, 5), QPointF(5, 5), QPoint(), QPoint(0, 120),
					 Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, false);

	// With ScrollZooms off, only Ctrl+wheel zooms
	olive::Config::Current()[QStringLiteral("ScrollZooms")] = false;
	EXPECT_TRUE(olive::HandMovableView::WheelEventIsAZoomEvent(&ctrl));
	EXPECT_FALSE(olive::HandMovableView::WheelEventIsAZoomEvent(&plain));

	// With ScrollZooms on, plain wheel zooms and Ctrl+wheel does not
	olive::Config::Current()[QStringLiteral("ScrollZooms")] = true;
	EXPECT_TRUE(olive::HandMovableView::WheelEventIsAZoomEvent(&plain));
	EXPECT_FALSE(olive::HandMovableView::WheelEventIsAZoomEvent(&ctrl));

	// 120 wheel units -> 1.12x; inverted devices flip the sign
	EXPECT_NEAR(olive::HandMovableView::GetScrollZoomMultiplier(&plain), 1.12,
				1e-9);
	QWheelEvent inverted(QPointF(5, 5), QPointF(5, 5), QPoint(),
						 QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
						 Qt::NoScrollPhase, true);
	EXPECT_NEAR(olive::HandMovableView::GetScrollZoomMultiplier(&inverted),
				0.88, 1e-9);

	olive::Config::Current()[QStringLiteral("ScrollZooms")] =
		old_scroll_zooms;
}

TEST(WidgetToolbarButton, StoresToolAndIsCheckable)
{
	olive::ToolbarButton btn(nullptr, olive::Tool::kSlip);
	EXPECT_EQ(btn.tool(), olive::Tool::kSlip);
	EXPECT_TRUE(btn.isCheckable());
}

TEST(WidgetToolbar, SetToolChecksMatchingButtonOnly)
{
	olive::Toolbar bar(nullptr);
	const auto buttons = ToolbarButtons(&bar);

	// 13 tool buttons + 1 snapping toggle
	EXPECT_EQ(buttons.size(), 14);

	bar.SetTool(olive::Tool::kRazor);
	for (olive::ToolbarButton *b : buttons) {
		if (b->tool() == olive::Tool::kNone) {
			continue;
		}
		EXPECT_EQ(b->isChecked(), b->tool() == olive::Tool::kRazor)
			<< int(b->tool());
	}
}

TEST(WidgetToolbar, ClickingButtonEmitsToolChanged)
{
	olive::Toolbar bar(nullptr);

	QVector<olive::Tool::Item> received;
	QObject::connect(&bar, &olive::Toolbar::ToolChanged,
					 [&received](const olive::Tool::Item &t) {
						 received.append(t);
					 });

	olive::ToolbarButton *pointer = nullptr;
	for (olive::ToolbarButton *b : ToolbarButtons(&bar)) {
		if (b->tool() == olive::Tool::kPointer) {
			pointer = b;
			break;
		}
	}
	ASSERT_NE(pointer, nullptr);
	pointer->click();

	ASSERT_EQ(received.size(), 1);
	EXPECT_EQ(received.first(), olive::Tool::kPointer);
}

TEST(WidgetToolbar, SnappingToggleReflectsAndEmits)
{
	olive::Toolbar bar(nullptr);
	QSignalSpy spy(&bar, &olive::Toolbar::SnappingChanged);

	olive::ToolbarButton *snap = nullptr;
	for (olive::ToolbarButton *b : ToolbarButtons(&bar)) {
		if (b->tool() == olive::Tool::kNone) {
			snap = b;
			break;
		}
	}
	ASSERT_NE(snap, nullptr);

	bar.SetSnapping(false);
	EXPECT_FALSE(snap->isChecked());
	bar.SetSnapping(true);
	EXPECT_TRUE(snap->isChecked());

	snap->click();
	ASSERT_EQ(spy.count(), 1);
	EXPECT_EQ(spy.first().first().toBool(), false);
}

TEST(WidgetColorButton, SetColorRoundTrips)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;

	olive::ColorButton btn(project.color_manager());
	EXPECT_FLOAT_EQ(btn.GetColor().red(), 1.0f);
	EXPECT_FLOAT_EQ(btn.GetColor().green(), 1.0f);
	EXPECT_FLOAT_EQ(btn.GetColor().blue(), 1.0f);

	btn.SetColor(olive::ManagedColor(0.25, 0.5, 0.75, 1.0));

	const olive::ManagedColor &out = btn.GetColor();
	EXPECT_FLOAT_EQ(out.red(), 0.25f);
	EXPECT_FLOAT_EQ(out.green(), 0.5f);
	EXPECT_FLOAT_EQ(out.blue(), 0.75f);
	EXPECT_FLOAT_EQ(out.alpha(), 1.0f);

	// An unset colorspace is conformed to the manager default
	EXPECT_FALSE(out.color_input().isEmpty());
}

TEST(WidgetColorValuesTab, FloatModeRoundTripsColor)
{
	olive::ColorValuesTab tab(false);
	tab.SetColor(olive::Color(0.25, 0.5, 0.75));

	EXPECT_NEAR(tab.GetRed(), 0.25, 1e-6);
	EXPECT_NEAR(tab.GetGreen(), 0.5, 1e-6);
	EXPECT_NEAR(tab.GetBlue(), 0.75, 1e-6);

	olive::Color out = tab.GetColor();
	EXPECT_NEAR(out.red(), 0.25, 1e-6);
	EXPECT_NEAR(out.green(), 0.5, 1e-6);
	EXPECT_NEAR(out.blue(), 0.75, 1e-6);

	// The web field shows the rgb() form in float mode
	auto *hex = tab.findChild<olive::StringSlider *>();
	ASSERT_NE(hex, nullptr);
	EXPECT_EQ(hex->GetValue(), QStringLiteral("rgb(0.25, 0.5, 0.75)"));
}

TEST(WidgetColorValuesTab, LegacyToggleRescalesSliders)
{
	const QVariant old_legacy =
		olive::Config::Current()[QStringLiteral("UseLegacyColorInInputTab")];
	olive::Config::Current()[QStringLiteral("UseLegacyColorInInputTab")] = false;

	{
		olive::ColorValuesTab tab(true);
		tab.SetRed(1.0);
		EXPECT_NEAR(tab.GetRed(), 1.0, 1e-6);

		QCheckBox *legacy = tab.findChild<QCheckBox *>();
		ASSERT_NE(legacy, nullptr);
		EXPECT_FALSE(legacy->isChecked());

		// Switching to legacy keeps the effective color but shows 0-255
		legacy->click();
		EXPECT_NEAR(tab.GetRed(), 1.0, 1e-6);

		auto *hex = tab.findChild<olive::StringSlider *>();
		ASSERT_NE(hex, nullptr);
		EXPECT_EQ(hex->GetValue(), QStringLiteral("FF0000"));

		// And back
		legacy->click();
		EXPECT_NEAR(tab.GetRed(), 1.0, 1e-6);
		EXPECT_EQ(hex->GetValue(), QStringLiteral("rgb(1.0, 0.0, 0.0)"));
	}

	olive::Config::Current()[QStringLiteral("UseLegacyColorInInputTab")] =
		old_legacy;
}

TEST(WidgetColorSwatchChooser, ClickingSwatchEmitsItsColor)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;

	olive::ColorSwatchChooser chooser(project.color_manager());
	const auto buttons = chooser.findChildren<olive::ColorButton *>();
	EXPECT_EQ(buttons.size(), 32);

	QVector<olive::Color> received;
	QObject::connect(&chooser, &olive::ColorSwatchChooser::ColorClicked,
					 [&received](const olive::ManagedColor &c) {
						 received.append(c);
					 });

	buttons.first()->click();
	ASSERT_EQ(received.size(), 1);

	// The emitted color is exactly the clicked button's color
	const olive::ManagedColor &expected = buttons.first()->GetColor();
	EXPECT_FLOAT_EQ(received.first().red(), expected.red());
	EXPECT_FLOAT_EQ(received.first().green(), expected.green());
	EXPECT_FLOAT_EQ(received.first().blue(), expected.blue());
}

TEST(WidgetColorSpaceChooser, InputRoundTripsAndEmits)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;

	const QStringList spaces =
		project.color_manager()->ListAvailableColorspaces();
	ASSERT_GE(spaces.size(), 2);

	// Input-only mode, as used by the export dialog
	olive::ColorSpaceChooser chooser(project.color_manager(), true, false);
	EXPECT_FALSE(chooser.input().isEmpty());

	QSignalSpy spy(&chooser,
				   &olive::ColorSpaceChooser::InputColorSpaceChanged);

	// Pick whichever colorspace isn't currently selected
	QString target;
	for (const QString &s : spaces) {
		if (s != chooser.input()) {
			target = s;
			break;
		}
	}
	ASSERT_FALSE(target.isEmpty());

	chooser.set_input(target);
	EXPECT_EQ(chooser.input(), target);
	ASSERT_EQ(spy.count(), 1);
	EXPECT_EQ(spy.first().first().toString(), target);
}

TEST(WidgetColorSpaceChooser, FullModePopulatesDisplayFields)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;

	olive::ColorSpaceChooser chooser(project.color_manager());
	EXPECT_FALSE(chooser.input().isEmpty());
	EXPECT_FALSE(chooser.output().display().isEmpty());
	EXPECT_FALSE(chooser.output().view().isEmpty());
}

TEST(WidgetColorPreviewBox, RendersManagedColor)
{
	olive::ColorPreviewBox box;
	box.resize(20, 20);
	box.SetColor(olive::Color(1.0, 0.0, 0.0, 1.0));

	QImage img(box.size(), QImage::Format_ARGB32);
	img.fill(Qt::transparent);
	box.render(&img);

	const QColor px = img.pixelColor(img.rect().center());
	EXPECT_GT(px.red(), 200);
	EXPECT_LT(px.green(), 60);
	EXPECT_LT(px.blue(), 60);
}

TEST(WidgetColorGradient, ClickPositionsMapToValueRange)
{
	olive::ColorGradientWidget grad(Qt::Horizontal);
	grad.resize(100, 20);
	grad.SetSelectedColor(olive::Color(1.0, 0.0, 0.0));

	QVector<olive::Color> received;
	QObject::connect(&grad, &olive::ColorGradientWidget::SelectedColorChanged,
					 [&received](const olive::Color &c) { received.append(c); });

	// Left edge is the full-value end of the gradient
	float hue, sat, val;
	QTest::mouseClick(&grad, Qt::LeftButton, Qt::NoModifier, QPoint(0, 10));
	ASSERT_EQ(received.size(), 1);
	EXPECT_FLOAT_EQ(grad.GetSelectedColor().red(), received.first().red());
	received.first().toHsv(&hue, &sat, &val);
	EXPECT_NEAR(val, 1.0, 1e-4);

	// Right edge approaches the zero-value end
	QTest::mouseClick(&grad, Qt::LeftButton, Qt::NoModifier, QPoint(99, 10));
	ASSERT_EQ(received.size(), 2);
	received.at(1).toHsv(&hue, &sat, &val);
	EXPECT_NEAR(val, 0.01, 0.02);
}

TEST(WidgetColorWheel, ResizeEmitsDiameter)
{
	olive::ColorWheelWidget wheel;
	wheel.show();
	EXPECT_TRUE(QTest::qWaitForWindowExposed(&wheel));

	QSignalSpy spy(&wheel, &olive::ColorWheelWidget::DiameterChanged);

	wheel.resize(200, 100);
	ASSERT_GE(spy.count(), 1);
	EXPECT_EQ(spy.last().first().toInt(), 100);

	wheel.resize(80, 120);
	EXPECT_EQ(spy.last().first().toInt(), 80);
}

TEST(WidgetColorWheel, SelectedColorRoundTrips)
{
	olive::ColorWheelWidget wheel;
	wheel.resize(100, 100);

	wheel.SetSelectedColor(olive::Color(0.2, 0.4, 0.6));
	EXPECT_FLOAT_EQ(wheel.GetSelectedColor().red(), 0.2f);
	EXPECT_FLOAT_EQ(wheel.GetSelectedColor().green(), 0.4f);
	EXPECT_FLOAT_EQ(wheel.GetSelectedColor().blue(), 0.6f);
}

TEST(WidgetNodeValueTree, PopulatesRowsAndSetsValueHint)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *source = new TwoValueNode();
	source->setParent(&project);

	auto *consumer = new olive::MathNode();
	consumer->setParent(&project);

	olive::Node::ConnectEdge(source,
							 olive::NodeInput(consumer, olive::MathNode::kParamAIn));

	olive::NodeValueTree tree;
	tree.SetNode(olive::NodeInput(consumer, olive::MathNode::kParamAIn),
				 olive::rational(0));

	// One row per pushed value
	ASSERT_EQ(tree.topLevelItemCount(), 2);

	// The row matching the input's float type is pre-selected
	int checked_row = -1;
	int float_row = -1;
	for (int i = 0; i < 2; i++) {
		if (tree.topLevelItem(i)->text(1) == QStringLiteral("Float")) {
			float_row = i;
		}
		auto *radio =
			qobject_cast<QRadioButton *>(tree.itemWidget(tree.topLevelItem(i), 0));
		ASSERT_NE(radio, nullptr);
		if (radio->isChecked()) {
			checked_row = i;
		}
	}
	EXPECT_EQ(checked_row, float_row);
	EXPECT_EQ(consumer->GetValueHintForInput(olive::MathNode::kParamAIn).index(),
			  -1);

	// Clicking the other row writes its value hint back to the node
	const int other_row = 1 - checked_row;
	auto *other_radio = qobject_cast<QRadioButton *>(
		tree.itemWidget(tree.topLevelItem(other_row), 0));
	ASSERT_NE(other_radio, nullptr);
	other_radio->click();

	const olive::Node::ValueHint hint =
		consumer->GetValueHintForInput(olive::MathNode::kParamAIn);
	EXPECT_EQ(hint.index(), 1 - other_row); // table.Count() - 1 - row
	EXPECT_TRUE(hint.types().contains(olive::NodeValue::kInt));
}
