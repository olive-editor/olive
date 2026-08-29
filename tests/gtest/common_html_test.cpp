#include <gtest/gtest.h>

#include <QTextCursor>
#include <QTextDocument>

#include "common/html.h"

namespace
{

QTextDocument *MakeDoc(const QString &plain_text)
{
	auto *doc = new QTextDocument();
	QTextCursor c(doc);
	c.insertText(plain_text);
	return doc;
}

// Extract the single fragment of a single-block document for format checks.
QTextFragment OnlyFragment(QTextDocument *doc)
{
	QTextBlock block = doc->begin();
	auto it = block.begin();
	EXPECT_NE(it, block.end());
	return it.fragment();
}

} // namespace

TEST(CommonHtml, DocToHtmlWrapsTextInParagraph)
{
	std::unique_ptr<QTextDocument> doc(MakeDoc(QStringLiteral("hello")));

	const QString html = olive::Html::DocToHtml(doc.get());

	EXPECT_TRUE(html.contains(QStringLiteral("<p")));
	EXPECT_TRUE(html.contains(QStringLiteral("hello")));
	EXPECT_TRUE(html.contains(QStringLiteral("</p>")));
}

TEST(CommonHtml, DocToHtmlEscapesSpecialCharacters)
{
	std::unique_ptr<QTextDocument> doc(MakeDoc(QStringLiteral("a<b>&\"c\"")));

	const QString html = olive::Html::DocToHtml(doc.get());

	EXPECT_FALSE(html.contains(QStringLiteral("a<b>")));
	EXPECT_TRUE(html.contains(QStringLiteral("&lt;")));
	EXPECT_TRUE(html.contains(QStringLiteral("&amp;")));
}

TEST(CommonHtml, AlignmentIsWrittenAsAttribute)
{
	QTextDocument doc;
	QTextCursor c(&doc);
	QTextBlockFormat fmt;
	fmt.setAlignment(Qt::AlignRight);
	c.setBlockFormat(fmt);
	c.insertText(QStringLiteral("right"));

	const QString html = olive::Html::DocToHtml(&doc);

	EXPECT_TRUE(html.contains(QStringLiteral("align=\"right\"")));
}

TEST(CommonHtml, CenterAlignmentIsWrittenAsAttribute)
{
	QTextDocument doc;
	QTextCursor c(&doc);
	QTextBlockFormat fmt;
	fmt.setAlignment(Qt::AlignHCenter);
	c.setBlockFormat(fmt);
	c.insertText(QStringLiteral("center"));

	const QString html = olive::Html::DocToHtml(&doc);

	EXPECT_TRUE(html.contains(QStringLiteral("align=\"center\"")));
}

TEST(CommonHtml, LeftAlignmentWritesNoAlignAttribute)
{
	QTextDocument doc;
	QTextCursor c(&doc);
	QTextBlockFormat fmt;
	fmt.setAlignment(Qt::AlignLeft);
	c.setBlockFormat(fmt);
	c.insertText(QStringLiteral("left"));

	const QString html = olive::Html::DocToHtml(&doc);

	EXPECT_FALSE(html.contains(QStringLiteral("align=")));
}

TEST(CommonHtml, CharFormatRoundTrip)
{
	QTextDocument doc;
	QTextCursor c(&doc);
	QTextCharFormat fmt;
	fmt.setFontWeight(QFont::Bold);
	fmt.setFontItalic(true);
	fmt.setFontUnderline(true);
	fmt.setFontStrikeOut(true);
	fmt.setFontOverline(true);
	fmt.setFontPointSize(24.0);
	fmt.setForeground(QColor(255, 0, 0));
	c.insertText(QStringLiteral("styled"), fmt);

	const QString html = olive::Html::DocToHtml(&doc);

	QTextDocument parsed;
	olive::Html::HtmlToDoc(&parsed, html);

	ASSERT_EQ(parsed.begin().text(), QStringLiteral("styled"));
	const QTextFragment frag = OnlyFragment(&parsed);
	const QTextCharFormat &out = frag.charFormat();
	EXPECT_EQ(out.fontWeight(), QFont::Bold);
	EXPECT_TRUE(out.fontItalic());
	EXPECT_TRUE(out.fontUnderline());
	EXPECT_TRUE(out.fontStrikeOut());
	EXPECT_TRUE(out.fontOverline());
	EXPECT_DOUBLE_EQ(out.fontPointSize(), 24.0);
	EXPECT_EQ(out.foreground().color(), QColor(255, 0, 0));
}

TEST(CommonHtml, ColorWithAlphaRoundTripsAsRgba)
{
	QTextDocument doc;
	QTextCursor c(&doc);
	QTextCharFormat fmt;
	QColor semi(10, 20, 30, 128);
	fmt.setForeground(semi);
	c.insertText(QStringLiteral("x"), fmt);

	const QString html = olive::Html::DocToHtml(&doc);
	EXPECT_TRUE(html.contains(QStringLiteral("rgba(")));

	QTextDocument parsed;
	olive::Html::HtmlToDoc(&parsed, html);

	const QTextFragment frag = OnlyFragment(&parsed);
	const QColor out = frag.charFormat().foreground().color();
	EXPECT_EQ(out.red(), semi.red());
	EXPECT_EQ(out.green(), semi.green());
	EXPECT_EQ(out.blue(), semi.blue());
	EXPECT_NEAR(out.alphaF(), semi.alphaF(), 0.01);
}

TEST(CommonHtml, BlockAlignmentRoundTrips)
{
	QTextDocument doc;
	QTextCursor c(&doc);
	QTextBlockFormat bfmt;
	bfmt.setAlignment(Qt::AlignHCenter);
	c.setBlockFormat(bfmt);
	c.insertText(QStringLiteral("centered"));

	QTextDocument parsed;
	olive::Html::HtmlToDoc(&parsed, olive::Html::DocToHtml(&doc));

	EXPECT_TRUE(parsed.begin().blockFormat().alignment() & Qt::AlignHCenter);
}

TEST(CommonHtml, MultipleBlocksSurviveRoundTrip)
{
	QTextDocument doc;
	QTextCursor c(&doc);
	c.insertText(QStringLiteral("first"));
	c.insertBlock();
	c.insertText(QStringLiteral("second"));

	QTextDocument parsed;
	olive::Html::HtmlToDoc(&parsed, olive::Html::DocToHtml(&doc));

	EXPECT_EQ(parsed.blockCount(), 2);
	EXPECT_EQ(parsed.begin().text(), QStringLiteral("first"));
	EXPECT_EQ(parsed.begin().next().text(), QStringLiteral("second"));
}

TEST(CommonHtml, LineSeparatorBecomesBr)
{
	QTextDocument doc;
	QTextCursor c(&doc);
	c.insertText(QStringLiteral("line1"));
	c.insertText(QString(QChar(QChar::LineSeparator)) +
				 QStringLiteral("line2"));

	const QString html = olive::Html::DocToHtml(&doc);

	EXPECT_TRUE(html.contains(QStringLiteral("<br/>")));
}

TEST(CommonHtml, HtmlToDocReplacesExistingContent)
{
	QTextDocument doc;
	QTextCursor c(&doc);
	c.insertText(QStringLiteral("old content that should disappear"));

	olive::Html::HtmlToDoc(&doc, QStringLiteral("<p>new</p>"));

	EXPECT_EQ(doc.toPlainText().trimmed(), QStringLiteral("new"));
}

TEST(CommonHtml, HtmlToDocHandlesInvalidMarkupWithoutCrash)
{
	QTextDocument doc;

	// Malformed markup must not crash; error is only logged.
	olive::Html::HtmlToDoc(&doc, QStringLiteral("<p>unclosed"));

	SUCCEED();
}

TEST(CommonHtml, EmptyHtmlProducesEmptyDocument)
{
	QTextDocument doc;
	olive::Html::HtmlToDoc(&doc, QString());

	EXPECT_LE(doc.blockCount(), 1);
	EXPECT_TRUE(doc.toPlainText().trimmed().isEmpty());
}

TEST(CommonHtml, LetterSpacingAndStretchRoundTrip)
{
	QTextDocument doc;
	QTextCursor c(&doc);
	QTextCharFormat fmt;
	fmt.setFontLetterSpacing(150.0);
	fmt.setFontStretch(125);
	c.insertText(QStringLiteral("spaced"), fmt);

	QTextDocument parsed;
	olive::Html::HtmlToDoc(&parsed, olive::Html::DocToHtml(&doc));

	const QTextFragment frag = OnlyFragment(&parsed);
	EXPECT_DOUBLE_EQ(frag.charFormat().fontLetterSpacing(), 150.0);
	EXPECT_EQ(frag.charFormat().fontStretch(), 125);
}

TEST(CommonHtml, NestedInlineTagsMergeFormats)
{
	QTextDocument doc;
	olive::Html::HtmlToDoc(
		&doc, QStringLiteral(
				  "<p><span style=\"font-weight: 600;\"><span "
				  "style=\"font-style: italic;\">both</span></span></p>"));

	const QTextFragment frag = OnlyFragment(&doc);
	EXPECT_TRUE(frag.charFormat().fontItalic());
	EXPECT_EQ(frag.charFormat().fontWeight(), 600 / 8);
}
