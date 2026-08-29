#include <gtest/gtest.h>

#include <QLabel>

#include "common/decibel.h"
#include "widget/slider/base/decimalsliderbase.h"
#include "widget/slider/floatslider.h"
#include "widget/slider/integerslider.h"

namespace
{

// Exposes the protected mapping/parsing entry points so they can be tested
// without simulating mouse drags
class ExposedFloatSlider : public olive::FloatSlider {
public:
	QString ValueToStringPublic(const QVariant &v) const
	{
		return ValueToString(v);
	}

	QVariant StringToValuePublic(const QString &s, bool *ok) const
	{
		return StringToValue(s, ok);
	}

	QVariant AdjustDragPublic(const QVariant &start, const double &drag) const
	{
		return AdjustDragDistanceInternal(start, drag);
	}
};

class ExposedIntegerSlider : public olive::IntegerSlider {
public:
	QString ValueToStringPublic(const QVariant &v) const
	{
		return ValueToString(v);
	}

	QVariant StringToValuePublic(const QString &s, bool *ok) const
	{
		return StringToValue(s, ok);
	}

	QVariant AdjustDragPublic(const QVariant &start, const double &drag) const
	{
		return AdjustDragDistanceInternal(start, drag);
	}
};

} // namespace

TEST(WidgetSlider, FloatToStringFormatsAndTrims)
{
	using olive::DecimalSliderBase;

	EXPECT_EQ(DecimalSliderBase::FloatToString(1.5, 2, false),
			  QStringLiteral("1.50"));
	EXPECT_EQ(DecimalSliderBase::FloatToString(1.5, 2, true),
			  QStringLiteral("1.5"));

	// Trimming always leaves at least one decimal digit
	EXPECT_EQ(DecimalSliderBase::FloatToString(2.0, 2, true),
			  QStringLiteral("2.0"));
	EXPECT_EQ(DecimalSliderBase::FloatToString(0.0, 3, true),
			  QStringLiteral("0.0"));

	EXPECT_EQ(DecimalSliderBase::FloatToString(-3.5, 1, false),
			  QStringLiteral("-3.5"));
	EXPECT_EQ(DecimalSliderBase::FloatToString(1.234, 2, false),
			  QStringLiteral("1.23"));
}

TEST(WidgetSlider, FloatSetValueClampsToRange)
{
	olive::FloatSlider s;
	EXPECT_DOUBLE_EQ(s.GetValue(), 0.0);

	s.SetValue(1.25);
	EXPECT_DOUBLE_EQ(s.GetValue(), 1.25);

	s.SetMinimum(0.0);
	s.SetMaximum(2.0);

	s.SetValue(-5.0);
	EXPECT_DOUBLE_EQ(s.GetValue(), 0.0);

	s.SetValue(10.0);
	EXPECT_DOUBLE_EQ(s.GetValue(), 2.0);
}

TEST(WidgetSlider, FloatRangeChangeClampsExistingValue)
{
	olive::FloatSlider s;

	s.SetValue(-3.0);
	s.SetMinimum(0.0);
	EXPECT_DOUBLE_EQ(s.GetValue(), 0.0);

	s.SetValue(5.0);
	s.SetMaximum(1.0);
	EXPECT_DOUBLE_EQ(s.GetValue(), 1.0);
}

TEST(WidgetSlider, FloatDisplayTransformRoundTrips)
{
	EXPECT_DOUBLE_EQ(
		olive::FloatSlider::TransformValueToDisplay(0.5, olive::FloatSlider::kPercentage),
		50.0);
	EXPECT_DOUBLE_EQ(
		olive::FloatSlider::TransformDisplayToValue(50.0, olive::FloatSlider::kPercentage),
		0.5);

	EXPECT_DOUBLE_EQ(
		olive::FloatSlider::TransformValueToDisplay(1.0, olive::FloatSlider::kDecibel),
		0.0);
	EXPECT_NEAR(
		olive::FloatSlider::TransformValueToDisplay(0.5, olive::FloatSlider::kDecibel),
		-6.0206, 0.001);
	EXPECT_NEAR(
		olive::FloatSlider::TransformDisplayToValue(
			olive::FloatSlider::TransformValueToDisplay(0.75, olive::FloatSlider::kDecibel),
			olive::FloatSlider::kDecibel),
		0.75, 1e-12);

	EXPECT_DOUBLE_EQ(
		olive::FloatSlider::TransformValueToDisplay(3.5, olive::FloatSlider::kNormal),
		3.5);
	EXPECT_DOUBLE_EQ(
		olive::FloatSlider::TransformDisplayToValue(3.5, olive::FloatSlider::kNormal),
		3.5);
}

TEST(WidgetSlider, FloatStaticValueToStringRespectsDisplayType)
{
	// Zero volume in decibel mode displays as an infinity symbol (U+221E)
	EXPECT_EQ(olive::FloatSlider::ValueToString(0.0, olive::FloatSlider::kDecibel, 2,
												false),
			  QString(QChar(0x221E)));

	EXPECT_EQ(olive::FloatSlider::ValueToString(0.5, olive::FloatSlider::kPercentage,
												1, false),
			  QStringLiteral("50.0"));
	EXPECT_EQ(olive::FloatSlider::ValueToString(1.234, olive::FloatSlider::kNormal,
												2, false),
			  QStringLiteral("1.23"));
}

TEST(WidgetSlider, FloatLabelShowsFormattedValue)
{
	olive::FloatSlider s;
	QLabel *label = s.findChild<QLabel *>();
	ASSERT_NE(label, nullptr);

	s.SetValue(0.5);
	EXPECT_EQ(label->text(), QStringLiteral("0.50"));

	s.SetDisplayType(olive::FloatSlider::kPercentage);
	EXPECT_EQ(label->text(), QStringLiteral("50.00%"));

	s.SetFormat(QStringLiteral("%1 px"));
	EXPECT_EQ(label->text(), QStringLiteral("50.00 px"));

	s.ClearFormat();
	s.SetDisplayType(olive::FloatSlider::kNormal);
	EXPECT_EQ(label->text(), QStringLiteral("0.50"));
}

TEST(WidgetSlider, FloatOffsetAppliesToDisplayAndParse)
{
	ExposedFloatSlider s;
	s.SetOffset(10.0);

	EXPECT_EQ(s.ValueToStringPublic(2.0), QStringLiteral("12.00"));

	bool ok = false;
	QVariant v = s.StringToValuePublic(QStringLiteral("12.5"), &ok);
	EXPECT_TRUE(ok);
	EXPECT_DOUBLE_EQ(v.toDouble(), 2.5);
}

TEST(WidgetSlider, FloatStringToValueRejectsGarbage)
{
	ExposedFloatSlider s;

	bool ok = true;
	s.StringToValuePublic(QStringLiteral("not a number"), &ok);
	EXPECT_FALSE(ok);
}

TEST(WidgetSlider, FloatStringToValueRespectsDisplayType)
{
	ExposedFloatSlider s;
	s.SetDisplayType(olive::FloatSlider::kPercentage);

	bool ok = false;
	QVariant v = s.StringToValuePublic(QStringLiteral("50"), &ok);
	EXPECT_TRUE(ok);
	EXPECT_DOUBLE_EQ(v.toDouble(), 0.5);
}

TEST(WidgetSlider, FloatDragDistanceRespectsDisplayType)
{
	ExposedFloatSlider s;

	// Normal: plain addition
	EXPECT_DOUBLE_EQ(s.AdjustDragPublic(1.0, 2.5).toDouble(), 3.5);

	// Percentage: drag is scaled by 1/100
	s.SetDisplayType(olive::FloatSlider::kPercentage);
	EXPECT_DOUBLE_EQ(s.AdjustDragPublic(0.5, 10.0).toDouble(), 0.6);

	// Decibel: drag happens in dB space
	s.SetDisplayType(olive::FloatSlider::kDecibel);
	const double expected =
		olive::Decibel::toLinear(olive::Decibel::fromLinear(1.0) + 6.0);
	EXPECT_DOUBLE_EQ(s.AdjustDragPublic(1.0, 6.0).toDouble(), expected);
}

TEST(WidgetSlider, TristateShowsDashesUntilValueSet)
{
	olive::FloatSlider s;
	QLabel *label = s.findChild<QLabel *>();
	ASSERT_NE(label, nullptr);

	s.SetValue(1.0);
	s.SetTristate();
	EXPECT_TRUE(s.IsTristate());
	EXPECT_EQ(label->text(), QStringLiteral("---"));

	// Setting a value clears the tristate display
	s.SetValue(2.0);
	EXPECT_FALSE(s.IsTristate());
	EXPECT_EQ(label->text(), QStringLiteral("2.00"));
}

TEST(WidgetSlider, LabelSubstitutionOverridesText)
{
	olive::FloatSlider s;
	QLabel *label = s.findChild<QLabel *>();
	ASSERT_NE(label, nullptr);

	s.SetValue(0.0);
	s.InsertLabelSubstitution(0.0, QStringLiteral("Zero"));
	EXPECT_EQ(label->text(), QStringLiteral("Zero"));

	s.SetValue(1.0);
	EXPECT_EQ(label->text(), QStringLiteral("1.00"));
}

TEST(WidgetSlider, IntegerSetValueClampsToRange)
{
	olive::IntegerSlider s;
	EXPECT_EQ(s.GetValue(), 0);

	s.SetMinimum(0);
	s.SetMaximum(10);

	s.SetValue(-3);
	EXPECT_EQ(s.GetValue(), 0);

	s.SetValue(42);
	EXPECT_EQ(s.GetValue(), 10);

	s.SetValue(7);
	EXPECT_EQ(s.GetValue(), 7);
}

TEST(WidgetSlider, IntegerStringToValueRounds)
{
	ExposedIntegerSlider s;

	bool ok = false;
	EXPECT_EQ(s.StringToValuePublic(QStringLiteral("3.6"), &ok).toLongLong(), 4);
	EXPECT_TRUE(ok);
	EXPECT_EQ(s.StringToValuePublic(QStringLiteral("-2.4"), &ok).toLongLong(), -2);
	EXPECT_TRUE(ok);

	ok = true;
	s.StringToValuePublic(QStringLiteral("junk"), &ok);
	EXPECT_FALSE(ok);
}

TEST(WidgetSlider, IntegerOffsetAppliesToDisplayAndParse)
{
	ExposedIntegerSlider s;
	s.SetOffset(10);

	EXPECT_EQ(s.ValueToStringPublic(2), QStringLiteral("12"));

	bool ok = false;
	EXPECT_EQ(s.StringToValuePublic(QStringLiteral("12"), &ok).toLongLong(), 2);
	EXPECT_TRUE(ok);
}

TEST(WidgetSlider, IntegerDragRoundsToWhole)
{
	ExposedIntegerSlider s;

	EXPECT_EQ(s.AdjustDragPublic(2, 1.4).toLongLong(), 3);
	EXPECT_EQ(s.AdjustDragPublic(2, -1.4).toLongLong(), 1);
}
