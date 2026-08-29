#include <gtest/gtest.h>

#include <memory>

#include <QMatrix4x4>
#include <QSignalSpy>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>

#include "node/keyframe.h"
#include "node/math/math/math.h"
#include "node/color/colormanager/colormanager.h"
#include "node/project.h"
#include "node/value.h"
#include "olive/core/render/audioparams.h"
#include "olive/core/render/samplebuffer.h"
#include "olive/core/render/sampleformat.h"

TEST(NodeValueExtended, VectorAccessors)
{
	olive::NodeValue v2(olive::NodeValue::kVec2, QVector2D(1.5f, -2.5f));
	EXPECT_FLOAT_EQ(v2.toVec2().x(), 1.5f);
	EXPECT_FLOAT_EQ(v2.toVec2().y(), -2.5f);

	olive::NodeValue v3(olive::NodeValue::kVec3, QVector3D(1.0f, 2.0f, 3.0f));
	EXPECT_FLOAT_EQ(v3.toVec3().x(), 1.0f);
	EXPECT_FLOAT_EQ(v3.toVec3().y(), 2.0f);
	EXPECT_FLOAT_EQ(v3.toVec3().z(), 3.0f);

	olive::NodeValue v4(olive::NodeValue::kVec4,
						QVector4D(1.0f, 2.0f, 3.0f, 4.0f));
	EXPECT_FLOAT_EQ(v4.toVec4().x(), 1.0f);
	EXPECT_FLOAT_EQ(v4.toVec4().y(), 2.0f);
	EXPECT_FLOAT_EQ(v4.toVec4().z(), 3.0f);
	EXPECT_FLOAT_EQ(v4.toVec4().w(), 4.0f);
}

TEST(NodeValueExtended, ColorMatrixBezierAccessors)
{
	olive::NodeValue color(olive::NodeValue::kColor,
						   olive::core::Color(0.1f, 0.2f, 0.3f, 0.4f));
	EXPECT_FLOAT_EQ(color.toColor().red(), 0.1f);
	EXPECT_FLOAT_EQ(color.toColor().green(), 0.2f);
	EXPECT_FLOAT_EQ(color.toColor().blue(), 0.3f);
	EXPECT_FLOAT_EQ(color.toColor().alpha(), 0.4f);

	QMatrix4x4 matrix;
	matrix.scale(2.0f, 3.0f, 4.0f);
	olive::NodeValue mat(olive::NodeValue::kMatrix, matrix);
	EXPECT_FLOAT_EQ(mat.toMatrix()(0, 0), 2.0f);
	EXPECT_FLOAT_EQ(mat.toMatrix()(1, 1), 3.0f);
	EXPECT_FLOAT_EQ(mat.toMatrix()(2, 2), 4.0f);

	olive::NodeValue bezier(olive::NodeValue::kBezier,
							olive::core::Bezier(1.0, 2.0, 3.0, 4.0, 5.0, 6.0));
	EXPECT_DOUBLE_EQ(bezier.toBezier().x(), 1.0);
	EXPECT_DOUBLE_EQ(bezier.toBezier().y(), 2.0);
	EXPECT_DOUBLE_EQ(bezier.toBezier().cp1_x(), 3.0);
	EXPECT_DOUBLE_EQ(bezier.toBezier().cp1_y(), 4.0);
	EXPECT_DOUBLE_EQ(bezier.toBezier().cp2_x(), 5.0);
	EXPECT_DOUBLE_EQ(bezier.toBezier().cp2_y(), 6.0);
}

TEST(NodeValueExtended, ScalarAccessors)
{
	olive::NodeValue boolean(olive::NodeValue::kBoolean, true);
	EXPECT_TRUE(boolean.toBool());

	olive::NodeValue floating(olive::NodeValue::kFloat, 2.75);
	EXPECT_DOUBLE_EQ(floating.toDouble(), 2.75);

	olive::NodeValue text(olive::NodeValue::kText, QStringLiteral("oak"));
	EXPECT_EQ(text.toString(), QStringLiteral("oak"));

	olive::NodeValue rational_value(olive::NodeValue::kRational,
									olive::core::rational(3, 4));
	EXPECT_EQ(rational_value.toRational(), olive::core::rational(3, 4));

	olive::NodeValue audio(
		olive::NodeValue::kAudioParams,
		olive::core::AudioParams(48000, olive::core::kChannelLayoutStereo,
								 olive::core::SampleFormat::F32P));
	EXPECT_EQ(audio.toAudioParams().sample_rate(), 48000);
}

TEST(NodeValueExtended, SamplesAccessorRoundTripsBuffer)
{
	olive::core::AudioParams params(48000, olive::core::kChannelLayoutMono,
									olive::core::SampleFormat::F32P);
	olive::core::SampleBuffer buffer(params, size_t(4));
	for (int i = 0; i < 4; i++) {
		buffer.data(0)[i] = 0.25f * float(i + 1);
	}

	olive::NodeValue value(olive::NodeValue::kSamples, buffer);
	olive::core::SampleBuffer out = value.toSamples();
	ASSERT_EQ(out.sample_count(), size_t(4));
	EXPECT_FLOAT_EQ(out.data(0)[0], 0.25f);
	EXPECT_FLOAT_EQ(out.data(0)[1], 0.5f);
	EXPECT_FLOAT_EQ(out.data(0)[2], 0.75f);
	EXPECT_FLOAT_EQ(out.data(0)[3], 1.0f);
}

TEST(NodeValueExtended, MismatchedTypeAccessorsReturnDefaults)
{
	olive::NodeValue text(olive::NodeValue::kText, QStringLiteral("hello"));

	// Accessors do not validate the stored type; failed QVariant conversions
	// produce default-constructed values
	EXPECT_EQ(text.toTexture(), nullptr);
	EXPECT_FALSE(text.toSamples().is_allocated());
	EXPECT_TRUE(text.toVec4().isNull());
	EXPECT_TRUE(text.toMatrix().isIdentity());

	const olive::core::Color c = text.toColor();
	EXPECT_FLOAT_EQ(c.red(), 0.0f);
	EXPECT_FLOAT_EQ(c.green(), 0.0f);
	EXPECT_FLOAT_EQ(c.blue(), 0.0f);
	EXPECT_FLOAT_EQ(c.alpha(), 0.0f);
}

TEST(NodeValueExtended, SourceArrayFlagAndEquality)
{
	olive::MathNode node; // any Node works; only the pointer value is observed
	olive::NodeValue value(olive::NodeValue::kFloat, 1.5, &node, true,
						   QStringLiteral("tag"));
	EXPECT_EQ(value.source(), static_cast<const olive::Node *>(&node));
	EXPECT_TRUE(value.array());
	EXPECT_EQ(value.tag(), QStringLiteral("tag"));
	EXPECT_EQ(value.type(), olive::NodeValue::kFloat);
	EXPECT_TRUE(value);

	// A default-constructed value carries no data
	olive::NodeValue empty;
	EXPECT_EQ(empty.type(), olive::NodeValue::kNone);
	EXPECT_EQ(empty.source(), nullptr);
	EXPECT_FALSE(empty.array());
	EXPECT_TRUE(empty.data().isNull());
	EXPECT_FALSE(empty);

	// Equality compares type, tag, and data; the source pointer and the array
	// flag are ignored
	olive::NodeValue same(olive::NodeValue::kFloat, 1.5, nullptr, false,
						  QStringLiteral("tag"));
	EXPECT_TRUE(value == same);

	olive::NodeValue different_tag(olive::NodeValue::kFloat, 1.5, &node, true,
								   QStringLiteral("other"));
	EXPECT_FALSE(value == different_tag);

	olive::NodeValue different_data(olive::NodeValue::kFloat, 2.5, &node, true,
									QStringLiteral("tag"));
	EXPECT_FALSE(value == different_data);

	olive::NodeValue different_type(olive::NodeValue::kInt, int64_t(1), &node,
									true, QStringLiteral("tag"));
	EXPECT_FALSE(value == different_type);
}

TEST(NodeValueExtended, CanConvertReflectsStoredData)
{
	olive::NodeValue integer(olive::NodeValue::kInt, int64_t(7));
	EXPECT_TRUE(integer.canConvert<int64_t>());

	olive::NodeValue text(olive::NodeValue::kText, QStringLiteral("hello"));
	EXPECT_TRUE(text.canConvert<QString>());

	olive::NodeValue vec(olive::NodeValue::kVec2, QVector2D(1.0f, 2.0f));
	EXPECT_TRUE(vec.canConvert<QVector2D>());
	EXPECT_FALSE(vec.canConvert<olive::core::Color>());
}

TEST(NodeValueExtended, ColorStringRoundTrip)
{
	const olive::core::Color c(0.25f, 0.5f, 0.75f, 1.0f);
	QString encoded = olive::NodeValue::ValueToString(
		olive::NodeValue::kColor, QVariant::fromValue(c), false);
	QVariant decoded = olive::NodeValue::StringToValue(
		olive::NodeValue::kColor, encoded, false);
	const olive::core::Color out = decoded.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(out.red(), c.red());
	EXPECT_FLOAT_EQ(out.green(), c.green());
	EXPECT_FLOAT_EQ(out.blue(), c.blue());
	EXPECT_FLOAT_EQ(out.alpha(), c.alpha());
}

TEST(NodeValueExtended, BezierStringRoundTrip)
{
	const olive::core::Bezier b(1.0, 2.0, 3.0, 4.0, 5.0, 6.0);
	QString encoded = olive::NodeValue::ValueToString(
		olive::NodeValue::kBezier, QVariant::fromValue(b), false);
	QVariant decoded = olive::NodeValue::StringToValue(
		olive::NodeValue::kBezier, encoded, false);
	const olive::core::Bezier out = decoded.value<olive::core::Bezier>();
	EXPECT_DOUBLE_EQ(out.x(), b.x());
	EXPECT_DOUBLE_EQ(out.y(), b.y());
	EXPECT_DOUBLE_EQ(out.cp1_x(), b.cp1_x());
	EXPECT_DOUBLE_EQ(out.cp1_y(), b.cp1_y());
	EXPECT_DOUBLE_EQ(out.cp2_x(), b.cp2_x());
	EXPECT_DOUBLE_EQ(out.cp2_y(), b.cp2_y());
}

TEST(NodeValueExtended, RationalStringRoundTrip)
{
	const olive::core::rational r(1, 24);
	QString encoded = olive::NodeValue::ValueToString(
		olive::NodeValue::kRational, QVariant::fromValue(r), false);
	EXPECT_EQ(encoded, QStringLiteral("1/24"));
	QVariant decoded = olive::NodeValue::StringToValue(
		olive::NodeValue::kRational, encoded, false);
	EXPECT_EQ(decoded.value<olive::core::rational>(), r);

	// The rational path applies to key track values too
	EXPECT_EQ(olive::NodeValue::ValueToString(olive::NodeValue::kRational,
											  QVariant::fromValue(r), true),
			  QStringLiteral("1/24"));
}

TEST(NodeValueExtended, IntStringRoundTrip)
{
	const int64_t big = INT64_C(9223372036854775807);
	QString encoded = olive::NodeValue::ValueToString(
		olive::NodeValue::kInt, QVariant::fromValue(big), false);
	EXPECT_EQ(encoded, QStringLiteral("9223372036854775807"));
	QVariant decoded = olive::NodeValue::StringToValue(
		olive::NodeValue::kInt, encoded, false);
	EXPECT_EQ(decoded.value<int64_t>(), big);

	const int64_t small = -big - 1;
	encoded = olive::NodeValue::ValueToString(
		olive::NodeValue::kInt, QVariant::fromValue(small), false);
	decoded = olive::NodeValue::StringToValue(olive::NodeValue::kInt, encoded,
											  false);
	EXPECT_EQ(decoded.value<int64_t>(), small);
}

TEST(NodeValueExtended, BufferAndNoneTypesSerializeToEmptyString)
{
	// Textures, samples, and empty values have no XML representation
	EXPECT_TRUE(olive::NodeValue::ValueToString(
					olive::NodeValue::kTexture,
					QVariant::fromValue(olive::TexturePtr()), false)
					.isEmpty());
	EXPECT_TRUE(olive::NodeValue::ValueToString(
					olive::NodeValue::kSamples,
					QVariant::fromValue(olive::core::SampleBuffer()), false)
					.isEmpty());
	EXPECT_TRUE(olive::NodeValue::ValueToString(olive::NodeValue::kNone,
												QVariant(), false)
					.isEmpty());
}

TEST(NodeValueExtended, KeyTrackFlagFallsBackToPlainString)
{
	// With the key-track flag set, values without a dedicated serialization
	// fall back to plain string conversion
	EXPECT_EQ(olive::NodeValue::ValueToString(olive::NodeValue::kText,
											  QStringLiteral("hello"), true),
			  QStringLiteral("hello"));
	EXPECT_EQ(olive::NodeValue::ValueToString(olive::NodeValue::kFloat, 2.5,
											  true),
			  QStringLiteral("2.5"));

	// StringToValue() likewise leaves key-track values as raw strings
	QVariant decoded = olive::NodeValue::StringToValue(
		olive::NodeValue::kFloat, QStringLiteral("2.5"), true);
	EXPECT_EQ(decoded.toString(), QStringLiteral("2.5"));
}

TEST(NodeValueExtended, ShortVectorStringIsZeroPadded)
{
	QVariant decoded = olive::NodeValue::StringToValue(
		olive::NodeValue::kVec3, QStringLiteral("5:7"), false);
	const QVector3D vec = decoded.value<QVector3D>();
	EXPECT_FLOAT_EQ(vec.x(), 5.0f);
	EXPECT_FLOAT_EQ(vec.y(), 7.0f);
	EXPECT_FLOAT_EQ(vec.z(), 0.0f);

	// Even an empty string yields a zero vector rather than crashing
	decoded = olive::NodeValue::StringToValue(olive::NodeValue::kVec2,
											  QString(), false);
	const QVector2D vec2 = decoded.value<QVector2D>();
	EXPECT_FLOAT_EQ(vec2.x(), 0.0f);
	EXPECT_FLOAT_EQ(vec2.y(), 0.0f);
}

TEST(NodeValueExtended, KeyframeTrackCounts)
{
	EXPECT_EQ(olive::NodeValue::get_number_of_keyframe_tracks(
				  olive::NodeValue::kVec2),
			  2);
	EXPECT_EQ(olive::NodeValue::get_number_of_keyframe_tracks(
				  olive::NodeValue::kVec3),
			  3);
	EXPECT_EQ(olive::NodeValue::get_number_of_keyframe_tracks(
				  olive::NodeValue::kVec4),
			  4);
	EXPECT_EQ(olive::NodeValue::get_number_of_keyframe_tracks(
				  olive::NodeValue::kColor),
			  4);
	EXPECT_EQ(olive::NodeValue::get_number_of_keyframe_tracks(
				  olive::NodeValue::kBezier),
			  6);

	// All scalar types live on a single track
	EXPECT_EQ(olive::NodeValue::get_number_of_keyframe_tracks(
				  olive::NodeValue::kFloat),
			  1);
	EXPECT_EQ(olive::NodeValue::get_number_of_keyframe_tracks(
				  olive::NodeValue::kInt),
			  1);
	EXPECT_EQ(olive::NodeValue::get_number_of_keyframe_tracks(
				  olive::NodeValue::kText),
			  1);
	EXPECT_EQ(olive::NodeValue::get_number_of_keyframe_tracks(
				  olive::NodeValue::kRational),
			  1);
	EXPECT_EQ(olive::NodeValue::get_number_of_keyframe_tracks(
				  olive::NodeValue::kNone),
			  1);
}

TEST(NodeValueExtended, SplitVectorIntoTrackValues)
{
	olive::NodeValue value(olive::NodeValue::kVec3,
						   QVector3D(1.0f, 2.0f, 3.0f));
	const olive::SplitValue split = value.to_split_value();
	ASSERT_EQ(split.size(), 3);
	EXPECT_FLOAT_EQ(split.at(0).toFloat(), 1.0f);
	EXPECT_FLOAT_EQ(split.at(1).toFloat(), 2.0f);
	EXPECT_FLOAT_EQ(split.at(2).toFloat(), 3.0f);

	// to_split_value() matches the underlying static helper
	const QVector<QVariant> manual =
		olive::NodeValue::split_normal_value_into_track_values(
			olive::NodeValue::kVec3,
			QVariant::fromValue(QVector3D(1.0f, 2.0f, 3.0f)));
	ASSERT_EQ(manual.size(), 3);
	EXPECT_FLOAT_EQ(manual.at(2).toFloat(), 3.0f);
}

TEST(NodeValueExtended, SplitColorAndBezierIntoTrackValues)
{
	olive::NodeValue color(olive::NodeValue::kColor,
						   olive::core::Color(0.1f, 0.2f, 0.3f, 0.4f));
	olive::SplitValue split = color.to_split_value();
	ASSERT_EQ(split.size(), 4);
	EXPECT_FLOAT_EQ(split.at(0).toFloat(), 0.1f);
	EXPECT_FLOAT_EQ(split.at(1).toFloat(), 0.2f);
	EXPECT_FLOAT_EQ(split.at(2).toFloat(), 0.3f);
	EXPECT_FLOAT_EQ(split.at(3).toFloat(), 0.4f);

	olive::NodeValue bezier(olive::NodeValue::kBezier,
							olive::core::Bezier(1.0, 2.0, 3.0, 4.0, 5.0, 6.0));
	split = bezier.to_split_value();
	ASSERT_EQ(split.size(), 6);
	EXPECT_DOUBLE_EQ(split.at(0).toDouble(), 1.0);
	EXPECT_DOUBLE_EQ(split.at(1).toDouble(), 2.0);
	EXPECT_DOUBLE_EQ(split.at(2).toDouble(), 3.0);
	EXPECT_DOUBLE_EQ(split.at(3).toDouble(), 4.0);
	EXPECT_DOUBLE_EQ(split.at(4).toDouble(), 5.0);
	EXPECT_DOUBLE_EQ(split.at(5).toDouble(), 6.0);
}

TEST(NodeValueExtended, SplitScalarStaysSingleValue)
{
	olive::NodeValue value(olive::NodeValue::kFloat, 4.75);
	const olive::SplitValue split = value.to_split_value();
	ASSERT_EQ(split.size(), 1);
	EXPECT_DOUBLE_EQ(split.at(0).toDouble(), 4.75);
}

TEST(NodeValueExtended, CombineTrackValuesRebuildsValue)
{
	// Round trip through split/combine restores the original value
	const olive::core::Color c(0.25f, 0.5f, 0.75f, 1.0f);
	olive::SplitValue split =
		olive::NodeValue(olive::NodeValue::kColor, c).to_split_value();
	QVariant combined = olive::NodeValue::combine_track_values_into_normal_value(
		olive::NodeValue::kColor, split);
	const olive::core::Color color_out = combined.value<olive::core::Color>();
	EXPECT_FLOAT_EQ(color_out.red(), c.red());
	EXPECT_FLOAT_EQ(color_out.green(), c.green());
	EXPECT_FLOAT_EQ(color_out.blue(), c.blue());
	EXPECT_FLOAT_EQ(color_out.alpha(), c.alpha());

	const QVector2D vec(1.5f, -2.5f);
	split = olive::NodeValue(olive::NodeValue::kVec2, vec).to_split_value();
	combined = olive::NodeValue::combine_track_values_into_normal_value(
		olive::NodeValue::kVec2, split);
	const QVector2D vec_out = combined.value<QVector2D>();
	EXPECT_FLOAT_EQ(vec_out.x(), vec.x());
	EXPECT_FLOAT_EQ(vec_out.y(), vec.y());

	// Scalar types return the first (only) track value
	QVariant scalar = olive::NodeValue::combine_track_values_into_normal_value(
		olive::NodeValue::kFloat, { 4.5 });
	EXPECT_DOUBLE_EQ(scalar.toDouble(), 4.5);

	// An empty split combines to a null variant
	EXPECT_TRUE(olive::NodeValue::combine_track_values_into_normal_value(
					olive::NodeValue::kVec2, {})
					.isNull());
}

TEST(NodeValueExtended, PrettyDataTypeNames)
{
	for (int i = olive::NodeValue::kNone; i < olive::NodeValue::kDataTypeCount;
		 i++) {
		const auto type = static_cast<olive::NodeValue::Type>(i);
		EXPECT_FALSE(olive::NodeValue::GetPrettyDataTypeName(type).isEmpty())
			<< "type " << i;
	}
	EXPECT_EQ(olive::NodeValue::GetPrettyDataTypeName(
				  olive::NodeValue::kDataTypeCount),
			  QStringLiteral("Unknown"));

	// kStrCombo and kPushButton have dedicated pretty names like every other
	// type
	EXPECT_EQ(
		olive::NodeValue::GetPrettyDataTypeName(olive::NodeValue::kStrCombo),
		QStringLiteral("String Combo"));
	EXPECT_EQ(
		olive::NodeValue::GetPrettyDataTypeName(olive::NodeValue::kPushButton),
		QStringLiteral("Push Button"));
}

TEST(NodeValueExtended, TypeClassificationRemainingCases)
{
	EXPECT_TRUE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kVec2));
	EXPECT_TRUE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kVec3));
	EXPECT_TRUE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kVec4));
	EXPECT_TRUE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kBezier));
	EXPECT_TRUE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kRational));
	EXPECT_FALSE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kNone));
	EXPECT_FALSE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kText));
	EXPECT_FALSE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kTexture));
	EXPECT_FALSE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kSamples));
	EXPECT_FALSE(
		olive::NodeValue::type_can_be_interpolated(olive::NodeValue::kBoolean));

	EXPECT_TRUE(olive::NodeValue::type_is_numeric(olive::NodeValue::kRational));
	EXPECT_FALSE(olive::NodeValue::type_is_numeric(olive::NodeValue::kVec2));
	EXPECT_FALSE(olive::NodeValue::type_is_numeric(olive::NodeValue::kColor));
	EXPECT_FALSE(olive::NodeValue::type_is_numeric(olive::NodeValue::kBoolean));
	EXPECT_FALSE(olive::NodeValue::type_is_numeric(olive::NodeValue::kNone));

	EXPECT_TRUE(olive::NodeValue::type_is_vector(olive::NodeValue::kVec4));
	EXPECT_FALSE(olive::NodeValue::type_is_vector(olive::NodeValue::kColor));
	EXPECT_FALSE(olive::NodeValue::type_is_vector(olive::NodeValue::kText));

	EXPECT_FALSE(olive::NodeValue::type_is_buffer(olive::NodeValue::kNone));
	EXPECT_FALSE(olive::NodeValue::type_is_buffer(olive::NodeValue::kFloat));
}

TEST(NodeValueExtended, AllRealTypesHaveDataTypeNames)
{
	EXPECT_EQ(olive::NodeValue::GetDataTypeName(olive::NodeValue::kStrCombo),
			  QStringLiteral("strcombo"));
	EXPECT_EQ(olive::NodeValue::GetDataTypeName(olive::NodeValue::kPushButton),
			  QStringLiteral("pushbutton"));
	EXPECT_TRUE(
		olive::NodeValue::GetDataTypeName(olive::NodeValue::kDataTypeCount)
			.isEmpty());

	EXPECT_EQ(olive::NodeValue::GetDataTypeFromName(
				  QStringLiteral("not-a-type")),
			  olive::NodeValue::kNone);

	// An empty name matches no type
	EXPECT_EQ(olive::NodeValue::GetDataTypeFromName(QString()),
			  olive::NodeValue::kNone);

	// The newly named types round-trip
	EXPECT_EQ(
		olive::NodeValue::GetDataTypeFromName(QStringLiteral("strcombo")),
		olive::NodeValue::kStrCombo);
	EXPECT_EQ(
		olive::NodeValue::GetDataTypeFromName(QStringLiteral("pushbutton")),
		olive::NodeValue::kPushButton);
}

TEST(NodeValueExtended, ArrayValuesRoundTrip)
{
	olive::NodeValueArray array;
	array[0] = olive::NodeValue(olive::NodeValue::kInt, int64_t(4));
	array[5] = olive::NodeValue(olive::NodeValue::kText,
								QStringLiteral("five"));

	olive::NodeValue value(olive::NodeValue::kInt, array, nullptr, true);
	EXPECT_TRUE(value.array());

	const olive::NodeValueArray round_trip = value.toArray();
	ASSERT_EQ(round_trip.size(), size_t(2));
	EXPECT_EQ(round_trip.at(0).toInt(), 4);
	EXPECT_EQ(round_trip.at(5).toString(), QStringLiteral("five"));
}

TEST(NodeValueTableExtended, GetReturnsNewestMatchingValue)
{
	olive::NodeValueTable table;
	table.Push(olive::NodeValue(olive::NodeValue::kFloat, 1.0));
	table.Push(olive::NodeValue(olive::NodeValue::kFloat, 2.0));

	// Get() scans from the back, so the newest value wins and nothing is
	// removed
	EXPECT_DOUBLE_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), 2.0);
	EXPECT_EQ(table.Count(), 2);
}

TEST(NodeValueTableExtended, GetWithTagSelectsMatchingValue)
{
	olive::NodeValueTable table;
	table.Push(olive::NodeValue(olive::NodeValue::kFloat, 1.0, nullptr,
								QStringLiteral("a")));
	table.Push(olive::NodeValue(olive::NodeValue::kFloat, 2.0, nullptr,
								QStringLiteral("b")));
	table.Push(olive::NodeValue(olive::NodeValue::kFloat, 3.0));

	EXPECT_DOUBLE_EQ(
		table.Get(olive::NodeValue::kFloat, QStringLiteral("a")).toDouble(),
		1.0);
	EXPECT_DOUBLE_EQ(
		table.Get(olive::NodeValue::kFloat, QStringLiteral("b")).toDouble(),
		2.0);

	// Without a tag the newest value wins
	EXPECT_DOUBLE_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), 3.0);
	EXPECT_EQ(table.GetValueIndex({ olive::NodeValue::kFloat },
								  QStringLiteral("b")),
			  1);

	// An unknown tag yields an empty value; the fallback to the oldest value
	// of the type only applies when no tag is requested
	olive::NodeValue missing =
		table.Get(olive::NodeValue::kFloat, QStringLiteral("missing"));
	EXPECT_EQ(missing.type(), olive::NodeValue::kNone);
	EXPECT_EQ(table.GetValueIndex({ olive::NodeValue::kFloat },
								  QStringLiteral("missing")),
			  -1);
}

TEST(NodeValueTableExtended, GetWithMultipleTypes)
{
	olive::NodeValueTable table;
	table.Push(
		olive::NodeValue(olive::NodeValue::kText, QStringLiteral("s")));
	table.Push(olive::NodeValue(olive::NodeValue::kFloat, 2.5));

	olive::NodeValue newest =
		table.Get({ olive::NodeValue::kVec2, olive::NodeValue::kFloat });
	EXPECT_DOUBLE_EQ(newest.toDouble(), 2.5);

	olive::NodeValue text =
		table.Get({ olive::NodeValue::kVec2, olive::NodeValue::kText });
	EXPECT_EQ(text.toString(), QStringLiteral("s"));

	// A type that was never pushed produces an empty (kNone) value
	olive::NodeValue missing = table.Get(olive::NodeValue::kColor);
	EXPECT_EQ(missing.type(), olive::NodeValue::kNone);
	EXPECT_FALSE(missing);
}

TEST(NodeValueTableExtended, PrependAddsValueToFront)
{
	olive::NodeValueTable table;
	table.Push(olive::NodeValue(olive::NodeValue::kFloat, 1.0));
	table.Prepend(olive::NodeValue(olive::NodeValue::kFloat, 2.0));
	table.Prepend(olive::NodeValue::kText, QStringLiteral("t"), nullptr,
				  QStringLiteral("tag"));

	ASSERT_EQ(table.Count(), 3);
	EXPECT_EQ(table.at(0).type(), olive::NodeValue::kText);
	EXPECT_EQ(table.at(0).tag(), QStringLiteral("tag"));
	EXPECT_DOUBLE_EQ(table.at(1).toDouble(), 2.0);
	EXPECT_DOUBLE_EQ(table.at(2).toDouble(), 1.0);

	// Get() scans from the back, so prepended values are the lowest priority
	EXPECT_DOUBLE_EQ(table.Get(olive::NodeValue::kFloat).toDouble(), 1.0);
}

TEST(NodeValueTableExtended, TakeWithTagAndMissingType)
{
	olive::NodeValueTable table;
	table.Push(olive::NodeValue(olive::NodeValue::kText, QStringLiteral("a"),
								nullptr, QStringLiteral("x")));
	table.Push(olive::NodeValue(olive::NodeValue::kText, QStringLiteral("b"),
								nullptr, QStringLiteral("y")));

	olive::NodeValue taken =
		table.Take(olive::NodeValue::kText, QStringLiteral("x"));
	EXPECT_EQ(taken.toString(), QStringLiteral("a"));
	EXPECT_EQ(table.Count(), 1);

	// Taking a type that is not present returns an empty value and leaves the
	// table unchanged
	olive::NodeValue absent = table.Take(olive::NodeValue::kColor);
	EXPECT_EQ(absent.type(), olive::NodeValue::kNone);
	EXPECT_EQ(table.Count(), 1);

	// Taking with an unmatched tag returns an empty value and leaves the table
	// unchanged; the oldest value of the type is not used as a fallback
	olive::NodeValue fallback =
		table.Take(olive::NodeValue::kText, QStringLiteral("missing"));
	EXPECT_EQ(fallback.type(), olive::NodeValue::kNone);
	ASSERT_EQ(table.Count(), 1);
	EXPECT_EQ(table.at(0).toString(), QStringLiteral("b"));
}

TEST(NodeValueTableExtended, TakeWithMultipleTypes)
{
	olive::NodeValueTable table;
	table.Push(
		olive::NodeValue(olive::NodeValue::kText, QStringLiteral("s")));
	table.Push(olive::NodeValue(olive::NodeValue::kFloat, 1.5));

	olive::NodeValue taken =
		table.Take({ olive::NodeValue::kVec2, olive::NodeValue::kFloat });
	EXPECT_DOUBLE_EQ(taken.toDouble(), 1.5);
	ASSERT_EQ(table.Count(), 1);
	EXPECT_EQ(table.at(0).type(), olive::NodeValue::kText);
}

TEST(NodeValueTableExtended, TakeAtRemovesByIndex)
{
	olive::NodeValueTable table;
	table.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(1)));
	table.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(2)));
	table.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(3)));

	olive::NodeValue taken = table.TakeAt(1);
	EXPECT_EQ(taken.toInt(), 2);
	ASSERT_EQ(table.Count(), 2);
	EXPECT_EQ(table.at(0).toInt(), 1);
	EXPECT_EQ(table.at(1).toInt(), 3);
}

TEST(NodeValueTableExtended, RemoveDeletesNewestEqualValue)
{
	olive::NodeValueTable table;
	table.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(1)));
	table.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(2)));
	table.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(1)));

	// Remove() scans from the back and drops the newest equal value
	table.Remove(olive::NodeValue(olive::NodeValue::kInt, int64_t(1)));
	ASSERT_EQ(table.Count(), 2);
	EXPECT_EQ(table.at(0).toInt(), 1);
	EXPECT_EQ(table.at(1).toInt(), 2);

	// Removing a value that is not present is a no-op
	table.Remove(olive::NodeValue(olive::NodeValue::kInt, int64_t(99)));
	EXPECT_EQ(table.Count(), 2);
}

TEST(NodeValueTableExtended, HasUsesExactTypeMatch)
{
	olive::NodeValueTable table;
	table.Push(olive::NodeValue(olive::NodeValue::kFloat, 1.0));
	EXPECT_TRUE(table.Has(olive::NodeValue::kFloat));
	EXPECT_FALSE(table.Has(olive::NodeValue::kInt));

	// Type is a sequential enum, so no other type aliases kFloat
	EXPECT_FALSE(table.Has(olive::NodeValue::kRational));
	EXPECT_FALSE(table.Has(olive::NodeValue::kText));

	// A table holding a kNone value reports it too
	olive::NodeValueTable none_table;
	none_table.Push(olive::NodeValue());
	EXPECT_TRUE(none_table.Has(olive::NodeValue::kNone));
}

TEST(NodeValueTableExtended, PushTableAppendsAllValues)
{
	olive::NodeValueTable first;
	first.Push(olive::NodeValue(olive::NodeValue::kFloat, 1.0));
	first.Push(
		olive::NodeValue(olive::NodeValue::kText, QStringLiteral("a")));

	olive::NodeValueTable second;
	second.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(7)));

	first.Push(second);
	ASSERT_EQ(first.Count(), 3);
	EXPECT_EQ(first.at(2).toInt(), 7);
}

TEST(NodeValueTableExtended, MergeSlipstreamsTables)
{
	// A single table is returned as-is
	olive::NodeValueTable single;
	single.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(9)));
	olive::NodeValueTable merged_single =
		olive::NodeValueTable::Merge({ single });
	ASSERT_EQ(merged_single.Count(), 1);
	EXPECT_EQ(merged_single.at(0).toInt(), 9);

	// Merging no tables yields an empty table
	EXPECT_TRUE(olive::NodeValueTable::Merge({}).isEmpty());

	// Rows are slipstreamed together from the back of each input table
	olive::NodeValueTable a;
	a.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(1)));
	a.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(3)));
	olive::NodeValueTable b;
	b.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(2)));
	b.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(4)));

	olive::NodeValueTable merged = olive::NodeValueTable::Merge({ a, b });
	ASSERT_EQ(merged.Count(), 4);
	EXPECT_EQ(merged.at(0).toInt(), 2);
	EXPECT_EQ(merged.at(1).toInt(), 1);
	EXPECT_EQ(merged.at(2).toInt(), 4);
	EXPECT_EQ(merged.at(3).toInt(), 3);

	// A longer table's excess rows end up at the front
	olive::NodeValueTable c;
	c.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(5)));
	c.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(6)));
	c.Push(olive::NodeValue(olive::NodeValue::kInt, int64_t(7)));

	olive::NodeValueTable merged_long = olive::NodeValueTable::Merge({ a, c });
	ASSERT_EQ(merged_long.Count(), 5);
	EXPECT_EQ(merged_long.at(0).toInt(), 5);
	EXPECT_EQ(merged_long.at(1).toInt(), 6);
	EXPECT_EQ(merged_long.at(2).toInt(), 1);
	EXPECT_EQ(merged_long.at(3).toInt(), 7);
	EXPECT_EQ(merged_long.at(4).toInt(), 3);
}

TEST(NodeKeyframeExtended, FullConstructorInitializesFields)
{
	olive::NodeKeyframe key(olive::core::rational(1, 24), 42.0,
							olive::NodeKeyframe::kBezier, 2, 3,
							QStringLiteral("input_name"));
	EXPECT_EQ(key.time(), olive::core::rational(1, 24));
	EXPECT_DOUBLE_EQ(key.value().toDouble(), 42.0);
	EXPECT_EQ(key.type(), olive::NodeKeyframe::kBezier);
	EXPECT_EQ(key.track(), 2);
	EXPECT_EQ(key.element(), 3);
	EXPECT_EQ(key.input(), QStringLiteral("input_name"));
	EXPECT_TRUE(key.bezier_control_in().isNull());
	EXPECT_TRUE(key.bezier_control_out().isNull());
	EXPECT_EQ(key.previous(), nullptr);
	EXPECT_EQ(key.next(), nullptr);
	EXPECT_EQ(key.parent(), nullptr);

	EXPECT_EQ(olive::NodeKeyframe::kDefaultType, olive::NodeKeyframe::kLinear);
}

TEST(NodeKeyframeExtended, CopyDuplicatesAllFields)
{
	olive::NodeKeyframe key(olive::core::rational(1, 24), 3.5,
							olive::NodeKeyframe::kBezier, 1, 2,
							QStringLiteral("in"));
	key.set_bezier_control_in(QPointF(0.1, 0.2));
	key.set_bezier_control_out(QPointF(0.3, 0.4));

	std::unique_ptr<olive::NodeKeyframe> copy(key.copy());
	EXPECT_EQ(copy->time(), key.time());
	EXPECT_DOUBLE_EQ(copy->value().toDouble(), 3.5);
	EXPECT_EQ(copy->type(), olive::NodeKeyframe::kBezier);
	EXPECT_EQ(copy->track(), 1);
	EXPECT_EQ(copy->element(), 2);
	EXPECT_EQ(copy->input(), QStringLiteral("in"));
	EXPECT_EQ(copy->bezier_control_in(), QPointF(0.1, 0.2));
	EXPECT_EQ(copy->bezier_control_out(), QPointF(0.3, 0.4));
	EXPECT_EQ(copy->parent(), nullptr);

	// copy(element) overrides the element
	std::unique_ptr<olive::NodeKeyframe> moved(key.copy(7));
	EXPECT_EQ(moved->element(), 7);

	// The copy is independent of the original
	key.set_value(9.0);
	key.set_bezier_control_in(QPointF(1.0, 1.0));
	EXPECT_DOUBLE_EQ(copy->value().toDouble(), 3.5);
	EXPECT_EQ(copy->bezier_control_in(), QPointF(0.1, 0.2));
}

TEST(NodeKeyframeExtended, SettersEmitSignals)
{
	// QSignalSpy resolves signal argument types at runtime; the app normally
	// registers these in Core::Start(), which the test harness does not call
	qRegisterMetaType<olive::core::rational>();
	qRegisterMetaType<olive::NodeKeyframe::Type>();

	olive::NodeKeyframe key;

	QSignalSpy time_spy(&key, &olive::NodeKeyframe::TimeChanged);
	key.set_time(olive::core::rational(1, 2));
	ASSERT_EQ(time_spy.count(), 1);
	EXPECT_EQ(time_spy.first().at(0).value<olive::core::rational>(),
			  olive::core::rational(1, 2));

	QSignalSpy value_spy(&key, &olive::NodeKeyframe::ValueChanged);
	key.set_value(3.5);
	ASSERT_EQ(value_spy.count(), 1);
	EXPECT_DOUBLE_EQ(value_spy.first().at(0).toDouble(), 3.5);

	QSignalSpy type_spy(&key, &olive::NodeKeyframe::TypeChanged);
	key.set_type(olive::NodeKeyframe::kHold);
	ASSERT_EQ(type_spy.count(), 1);
	EXPECT_EQ(type_spy.first().at(0).value<olive::NodeKeyframe::Type>(),
			  olive::NodeKeyframe::kHold);

	// Setting the same type again does not re-emit
	key.set_type(olive::NodeKeyframe::kHold);
	EXPECT_EQ(type_spy.count(), 1);

	QSignalSpy in_spy(&key, &olive::NodeKeyframe::BezierControlInChanged);
	key.set_bezier_control_in(QPointF(0.25, -0.5));
	ASSERT_EQ(in_spy.count(), 1);
	EXPECT_EQ(in_spy.first().at(0).toPointF(), QPointF(0.25, -0.5));

	QSignalSpy out_spy(&key, &olive::NodeKeyframe::BezierControlOutChanged);
	key.set_bezier_control_out(QPointF(-0.25, 0.5));
	ASSERT_EQ(out_spy.count(), 1);
	EXPECT_EQ(out_spy.first().at(0).toPointF(), QPointF(-0.25, 0.5));
}

TEST(NodeKeyframeExtended, SetTypeToBezierInitializesHandles)
{
	// Without neighbors the handles default to one second either way
	olive::NodeKeyframe lone;
	lone.set_time(olive::core::rational(2));
	lone.set_type(olive::NodeKeyframe::kBezier);
	EXPECT_DOUBLE_EQ(lone.bezier_control_in().x(), -1.0);
	EXPECT_DOUBLE_EQ(lone.bezier_control_in().y(), 0.0);
	EXPECT_DOUBLE_EQ(lone.bezier_control_out().x(), 1.0);
	EXPECT_DOUBLE_EQ(lone.bezier_control_out().y(), 0.0);

	// With neighbors the handles default to halfway to each neighbor's time
	olive::NodeKeyframe previous;
	previous.set_time(olive::core::rational(-4));
	olive::NodeKeyframe next;
	next.set_time(olive::core::rational(8));
	olive::NodeKeyframe key;
	key.set_time(olive::core::rational(2));
	key.set_previous(&previous);
	key.set_next(&next);
	key.set_type(olive::NodeKeyframe::kBezier);
	EXPECT_DOUBLE_EQ(key.bezier_control_in().x(), -3.0);
	EXPECT_DOUBLE_EQ(key.bezier_control_in().y(), 0.0);
	EXPECT_DOUBLE_EQ(key.bezier_control_out().x(), 3.0);
	EXPECT_DOUBLE_EQ(key.bezier_control_out().y(), 0.0);

	// Handles that are already set are preserved
	olive::NodeKeyframe preset;
	preset.set_bezier_control_in(QPointF(-0.25, 0.5));
	preset.set_bezier_control_out(QPointF(0.75, -0.5));
	preset.set_type(olive::NodeKeyframe::kBezier);
	EXPECT_EQ(preset.bezier_control_in(), QPointF(-0.25, 0.5));
	EXPECT_EQ(preset.bezier_control_out(), QPointF(0.75, -0.5));
}

TEST(NodeKeyframeExtended, SetTypeNoBezierAdjLeavesHandlesUntouched)
{
	olive::NodeKeyframe key;
	key.set_bezier_control_in(QPointF(0.5, 0.5));
	key.set_bezier_control_out(QPointF(-0.5, -0.5));
	key.set_type_no_bezier_adj(olive::NodeKeyframe::kBezier);
	EXPECT_EQ(key.type(), olive::NodeKeyframe::kBezier);
	EXPECT_EQ(key.bezier_control_in(), QPointF(0.5, 0.5));
	EXPECT_EQ(key.bezier_control_out(), QPointF(-0.5, -0.5));

	// Handles stay null when none were set
	olive::NodeKeyframe other;
	other.set_type_no_bezier_adj(olive::NodeKeyframe::kBezier);
	EXPECT_TRUE(other.bezier_control_in().isNull());
	EXPECT_TRUE(other.bezier_control_out().isNull());
}

TEST(NodeKeyframeExtended, BezierControlAccessorsByHandleType)
{
	olive::NodeKeyframe key;
	key.set_bezier_control(olive::NodeKeyframe::kInHandle,
						   QPointF(-0.5, 0.25));
	key.set_bezier_control(olive::NodeKeyframe::kOutHandle,
						   QPointF(0.5, -0.25));

	EXPECT_EQ(key.bezier_control_in(), QPointF(-0.5, 0.25));
	EXPECT_EQ(key.bezier_control_out(), QPointF(0.5, -0.25));
	EXPECT_EQ(key.bezier_control(olive::NodeKeyframe::kInHandle),
			  key.bezier_control_in());
	EXPECT_EQ(key.bezier_control(olive::NodeKeyframe::kOutHandle),
			  key.bezier_control_out());

	EXPECT_EQ(olive::NodeKeyframe::get_opposing_bezier_type(
				  olive::NodeKeyframe::kInHandle),
			  olive::NodeKeyframe::kOutHandle);
	EXPECT_EQ(olive::NodeKeyframe::get_opposing_bezier_type(
				  olive::NodeKeyframe::kOutHandle),
			  olive::NodeKeyframe::kInHandle);
}

TEST(NodeKeyframeExtended, ValidBezierControlsClampToNeighbors)
{
	olive::NodeKeyframe key;
	key.set_time(olive::core::rational(2));
	key.set_bezier_control_in(QPointF(-5.0, 0.5));
	key.set_bezier_control_out(QPointF(5.0, -0.25));

	// Without neighbors the handles pass through unchanged
	EXPECT_EQ(key.valid_bezier_control_in(), QPointF(-5.0, 0.5));
	EXPECT_EQ(key.valid_bezier_control_out(), QPointF(5.0, -0.25));

	olive::NodeKeyframe previous;
	previous.set_time(olive::core::rational(1));
	olive::NodeKeyframe next;
	next.set_time(olive::core::rational(3));
	key.set_previous(&previous);
	key.set_next(&next);

	EXPECT_EQ(key.previous(), &previous);
	EXPECT_EQ(key.next(), &next);

	// The clamped handles may not cross the neighboring keyframe's time
	EXPECT_EQ(key.valid_bezier_control_in(), QPointF(-1.0, 0.5));
	EXPECT_EQ(key.valid_bezier_control_out(), QPointF(1.0, -0.25));
}

TEST(NodeKeyframeExtended, KeyTrackRefReflectsInputTrackElement)
{
	olive::NodeKeyframe key(olive::core::rational(1, 24), 2.0,
							olive::NodeKeyframe::kLinear, 2, 3,
							QStringLiteral("my_input"));

	const olive::NodeKeyframeTrackReference ref = key.key_track_ref();
	EXPECT_EQ(ref.track(), 2);
	EXPECT_EQ(ref.input().input(), QStringLiteral("my_input"));
	EXPECT_EQ(ref.input().element(), 3);
	EXPECT_EQ(ref.input().node(), nullptr);
	EXPECT_FALSE(ref.IsValid());
}

TEST(NodeKeyframeExtended, HasSiblingAtTimeDetectsOtherKeyframes)
{
	olive::ColorManager::SetUpDefaultConfig();
	olive::Project project;
	project.Initialize();

	auto *math = new olive::MathNode();
	math->setParent(&project);

	// Keyframe lookups only apply to tracks with keyframing enabled
	math->SetInputIsKeyframing(olive::MathNode::kParamAIn, true);

	auto *first = new olive::NodeKeyframe(
		olive::core::rational(0), 1.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamAIn, math);
	auto *second = new olive::NodeKeyframe(
		olive::core::rational(1), 2.0, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamAIn, math);

	// Parenting inserts the keyframes in time order and links the track
	EXPECT_EQ(first->next(), second);
	EXPECT_EQ(second->previous(), first);
	EXPECT_EQ(first->previous(), nullptr);
	EXPECT_EQ(second->next(), nullptr);

	// A sibling exists wherever another keyframe holds the time
	EXPECT_TRUE(second->has_sibling_at_time(olive::core::rational(0)));
	EXPECT_FALSE(second->has_sibling_at_time(olive::core::rational(1)));
	EXPECT_FALSE(first->has_sibling_at_time(olive::core::rational(2)));

	// Inserting out of order keeps the track sorted and relinks neighbors
	auto *middle = new olive::NodeKeyframe(
		olive::core::rational(1, 2), 1.5, olive::NodeKeyframe::kLinear, 0, -1,
		olive::MathNode::kParamAIn, math);
	EXPECT_EQ(first->next(), middle);
	EXPECT_EQ(middle->previous(), first);
	EXPECT_EQ(middle->next(), second);
	EXPECT_EQ(second->previous(), middle);
	EXPECT_TRUE(middle->has_sibling_at_time(olive::core::rational(0)));
}
