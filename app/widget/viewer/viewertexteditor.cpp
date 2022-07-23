/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2022 Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#include "viewertexteditor.h"

#include <QAbstractTextDocumentLayout>
#include <QColorDialog>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QPainter>
#include <QScrollBar>
#include <QTextBlock>

#include "common/qtutils.h"
#include "ui/icons/icons.h"

namespace olive {

#define super QTextEdit

ViewerTextEditor::ViewerTextEditor(double scale, QWidget *parent) :
  super(parent),
  transparent_clone_(nullptr),
  block_update_toolbar_signal_(false),
  listen_to_focus_events_(false),
  forced_default_(false)
{
  // Ensure default text color is white
  QPalette p = palette();
  p.setColor(QPalette::Text, Qt::white);
  setPalette(p);

  document()->setDefaultStyleSheet(QStringLiteral("body { color: white; }"));

  viewport()->setAutoFillBackground(false);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  connect(horizontalScrollBar(), &QScrollBar::rangeChanged, this, &ViewerTextEditor::LockScrollBarMaximumToZero);
  connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, &ViewerTextEditor::LockScrollBarMaximumToZero);

  // Force DPI to the same one that we're using in the actual render
  dpi_force_ = QImage(1, 1, QImage::Format_RGBA8888_Premultiplied);

  const int dpm = 3780 * scale;
  dpi_force_.setDotsPerMeterX(dpm);
  dpi_force_.setDotsPerMeterY(dpm);
  document()->documentLayout()->setPaintDevice(&dpi_force_);

  connect(this, &QTextEdit::currentCharFormatChanged, this, &ViewerTextEditor::FormatChanged);
  connect(document(), &QTextDocument::contentsChanged, this, &ViewerTextEditor::DocumentChanged, Qt::QueuedConnection);

  setAcceptRichText(false);
}

void ViewerTextEditor::ConnectToolBar(ViewerTextEditorToolBar *toolbar)
{
  connect(toolbar, &ViewerTextEditorToolBar::FamilyChanged, this, &ViewerTextEditor::SetFamily);
  connect(toolbar, &ViewerTextEditorToolBar::SizeChanged, this, &ViewerTextEditor::setFontPointSize);
  connect(toolbar, &ViewerTextEditorToolBar::StyleChanged, this, &ViewerTextEditor::SetStyle);
  connect(toolbar, &ViewerTextEditorToolBar::UnderlineChanged, this, &ViewerTextEditor::setFontUnderline);
  connect(toolbar, &ViewerTextEditorToolBar::StrikethroughChanged, this, &ViewerTextEditor::SetFontStrikethrough);
  connect(toolbar, &ViewerTextEditorToolBar::ColorChanged, this, &ViewerTextEditor::setTextColor);
  connect(toolbar, &ViewerTextEditorToolBar::SmallCapsChanged, this, &ViewerTextEditor::SetSmallCaps);
  connect(toolbar, &ViewerTextEditorToolBar::StretchChanged, this, &ViewerTextEditor::SetFontStretch);
  connect(toolbar, &ViewerTextEditorToolBar::KerningChanged, this, &ViewerTextEditor::SetFontKerning);
  connect(toolbar, &ViewerTextEditorToolBar::LineHeightChanged, this, &ViewerTextEditor::SetLineHeight);
  connect(toolbar, &ViewerTextEditorToolBar::AlignmentChanged, this, [this](Qt::Alignment a){
    this->setAlignment(a);

    // Ensure no buttons are checked that shouldn't be
    static_cast<ViewerTextEditorToolBar*>(sender())->SetAlignment(a);
  });

  UpdateToolBar(toolbar, this->currentCharFormat(), this->textCursor().blockFormat(), this->alignment());

  toolbars_.append(toolbar);
}

void ViewerTextEditor::paintEvent(QPaintEvent *e)
{
  QPainter p(this->viewport());

  QAbstractTextDocumentLayout::PaintContext ctx;

  QRect r = e->rect();
  if (r.isValid())
    p.setClipRect(r, Qt::IntersectClip);
  ctx.clip = r;

  ctx.cursorPosition = this->textCursor().position();

  if (this->textCursor().hasSelection()) {
    QAbstractTextDocumentLayout::Selection selection;
    selection.cursor = this->textCursor();

    QPalette::ColorGroup cg = this->hasFocus() ? QPalette::Active : QPalette::Inactive;
    QBrush b = ctx.palette.brush(cg, QPalette::Highlight);
    QColor bc = b.color();
    bc.setAlpha(128);
    b.setColor(bc);
    selection.format.setBackground(b);
    QStyleOption opt;
    opt.initFrom(this);
    if (this->style()->styleHint(QStyle::SH_RichText_FullWidthSelection, &opt, this)) {
      selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    }
    ctx.selections.append(selection);
  }

  if (transparent_clone_) {
    transparent_clone_->documentLayout()->draw(&p, ctx);
  }
}

void ViewerTextEditor::UpdateToolBar(ViewerTextEditorToolBar *toolbar, const QTextCharFormat &f, const QTextBlockFormat &b, Qt::Alignment alignment)
{
  QFontDatabase fd;

  QString family = f.fontFamily();
  if (family.isEmpty()) {
    family = qApp->font().family();
  }

  QString style = f.fontStyleName().toString();
  QStringList styles = fd.styles(family);
  if (!styles.isEmpty() && (style.isEmpty() || !styles.contains(style))) {
    // There seems to be no better way to find the "regular" style outside of this heuristic.
    // Feel free to add more if a font isn't working right.
    style = QStringLiteral("Regular");
    if (!styles.contains(style)) {
      style = QStringLiteral("Normal");
    }
    if (!styles.contains(style)) {
      style = QStringLiteral("R");
    }
    if (!styles.contains(style)) {
      style = styles.first();
    }
  }

  toolbar->SetFontFamily(family);
  toolbar->SetFontSize(f.fontPointSize());
  toolbar->SetStyle(style);
  toolbar->SetUnderline(f.fontUnderline());
  toolbar->SetStrikethrough(f.fontStrikeOut());
  toolbar->SetAlignment(alignment);
  toolbar->SetColor(f.foreground().color());
  toolbar->SetSmallCaps(f.fontCapitalization() == QFont::SmallCaps);
  toolbar->SetStretch(f.fontStretch() == 0 ? 100 : f.fontStretch());
  toolbar->SetKerning(f.fontLetterSpacing() == 0.0 ? 100 : f.fontLetterSpacing());
  toolbar->SetLineHeight(b.lineHeight() == 0.0 ? 100 : b.lineHeight());
}

void ViewerTextEditor::FormatChanged(const QTextCharFormat &f)
{
  if (!block_update_toolbar_signal_) {
    foreach (ViewerTextEditorToolBar *toolbar, toolbars_) {
      UpdateToolBar(toolbar, f, textCursor().blockFormat(), this->alignment());
    }
  }

  if (!(document()->blockCount() == 1 && document()->firstBlock().text().isEmpty())) {
    default_fmt_ = f;
  }
}

void ViewerTextEditor::SetFamily(const QString &s)
{
  ViewerTextEditorToolBar *toolbar = static_cast<ViewerTextEditorToolBar *>(sender());

  QTextCharFormat f;
  f.setFontFamily(s);
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
  f.setFontFamilies({s});
#endif

  ApplyStyle(&f, s, toolbar->GetFontStyleName());

  MergeCharFormat(f);
}

void ViewerTextEditor::SetStyle(const QString &s)
{
  ViewerTextEditorToolBar *toolbar = static_cast<ViewerTextEditorToolBar *>(sender());

  QTextCharFormat f;

  ApplyStyle(&f, toolbar->GetFontFamily(), s);

  MergeCharFormat(f);
}

void ViewerTextEditor::SetFontStrikethrough(bool e)
{
  QTextCharFormat f;
  f.setFontStrikeOut(e);
  MergeCharFormat(f);
}

void ViewerTextEditor::SetSmallCaps(bool e)
{
  QTextCharFormat f;
  f.setFontCapitalization(e ? QFont::SmallCaps : QFont::MixedCase);
  MergeCharFormat(f);
}

void ViewerTextEditor::SetFontStretch(int i)
{
  QTextCharFormat f;
  f.setFontStretch(i);
  MergeCharFormat(f);
}

void ViewerTextEditor::SetFontKerning(qreal i)
{
  QTextCharFormat f;
  f.setFontLetterSpacing(i);
  MergeCharFormat(f);
}

void ViewerTextEditor::MergeCharFormat(const QTextCharFormat &fmt)
{
  // mergeCurrentCharFormat throws a currentCharFormatChanged signal that updates the toolbar,
  // this can be undesirable if the user is currently typing a font
  block_update_toolbar_signal_ = true;
  mergeCurrentCharFormat(fmt);
  //default_fmt_ = this->currentCharFormat();
  block_update_toolbar_signal_ = false;
}

void ViewerTextEditor::ApplyStyle(QTextCharFormat *format, const QString &family, const QString &style)
{
  // NOTE: Windows appears to require setting weight and italic manually, while macOS and Linux are
  //       perfectly fine with just the style name
  QFontDatabase fd;
  format->setFontWeight(fd.weight(family, style));
  format->setFontItalic(fd.italic(family, style));

  format->setFontStyleName(style);
}

void ViewerTextEditor::SetLineHeight(qreal i)
{
  QTextBlockFormat f = this->textCursor().blockFormat();
  f.setLineHeight(i, QTextBlockFormat::ProportionalHeight);
  this->textCursor().setBlockFormat(f);
}

void ViewerTextEditor::LockScrollBarMaximumToZero()
{
  static_cast<QScrollBar*>(sender())->setMaximum(0);
}

void ViewerTextEditor::DocumentChanged()
{
  if (document()->blockCount() == 1 && document()->firstBlock().text().isEmpty()) {
    if (!forced_default_) {
      QTextCursor c(document()->firstBlock());
      c.setBlockCharFormat(default_fmt_);
      forced_default_ = true;
    }
  } else {
    forced_default_ = false;
  }

  // HACK: We want to show the text cursor and selections without necessarily rendering the text,
  //       because the text is already being rendered underneath the gizmo (and rendering twice will
  //       alter the overall look of the text while editing). This is something that Qt does not
  //       support by default, and while it could be solved by subclassing QAbstractTextDocumentLayout,
  //       this seems like overkill since 99% of QTextDocumentLayout's (the subclass used by default)
  //       functionality and would need to be fully implemented ourselves. So instead, we clone the
  //       document, set all of its colors to rgba(0,0,0,0) to make them transparent, and draw that
  //       document instead. The result is cursor and selections being rendered without the text.
  //       While this isn't the fastest code, nor is it the cleanest solution, it definitely works.
  delete transparent_clone_;
  transparent_clone_ = document()->clone(this);
  transparent_clone_->documentLayout()->setPaintDevice(&dpi_force_);

  QTextCursor cursor(transparent_clone_);
  cursor.select(QTextCursor::Document);

  QTextCharFormat fmt;
  fmt.setForeground(QColor(0, 0, 0, 0));
  cursor.mergeCharFormat(fmt);
}

ViewerTextEditorToolBar::ViewerTextEditorToolBar(QWidget *parent) :
  QWidget(parent),
  painted_(false),
  drag_enabled_(false)
{
  QVBoxLayout *outer_layout = new QVBoxLayout(this);
  outer_layout->setSpacing(0);

  QHBoxLayout *basic_layout = new QHBoxLayout();
  outer_layout->addLayout(basic_layout);

  int advanced_slider_width = QtUtils::QFontMetricsWidth(fontMetrics(), QStringLiteral("9999.9%"));

  font_combo_ = new QFontComboBox();
  connect(font_combo_, &QFontComboBox::currentTextChanged, this, &ViewerTextEditorToolBar::UpdateFontStyleListAndEmitFamilyChanged);
  basic_layout->addWidget(font_combo_);

  font_sz_slider_ = new FloatSlider();
  font_sz_slider_->SetMinimum(0.1);
  font_sz_slider_->SetMaximum(9999.9);
  font_sz_slider_->SetDecimalPlaces(1);
  font_sz_slider_->SetAlignment(Qt::AlignCenter);
  font_sz_slider_->setFixedWidth(advanced_slider_width);
  connect(font_sz_slider_, &FloatSlider::ValueChanged, this, &ViewerTextEditorToolBar::SizeChanged);
  font_sz_slider_->SetLadderElementCount(2);
  basic_layout->addWidget(font_sz_slider_);

  style_combo_ = new QComboBox();
  connect(style_combo_, &QComboBox::currentTextChanged, this, &ViewerTextEditorToolBar::StyleChanged);
  basic_layout->addWidget(style_combo_);

  underline_btn_ = new QPushButton();
  connect(underline_btn_, &QPushButton::clicked, this, &ViewerTextEditorToolBar::UnderlineChanged);
  underline_btn_->setCheckable(true);
  underline_btn_->setIcon(icon::TextUnderline);
  basic_layout->addWidget(underline_btn_);

  strikethrough_btn_ = new QPushButton();
  connect(strikethrough_btn_, &QPushButton::clicked, this, &ViewerTextEditorToolBar::StrikethroughChanged);
  strikethrough_btn_->setCheckable(true);
  strikethrough_btn_->setIcon(icon::TextStrikethrough);
  basic_layout->addWidget(strikethrough_btn_);

  basic_layout->addWidget(QtUtils::CreateVerticalLine());

  align_left_btn_ = new QPushButton();
  align_left_btn_->setCheckable(true);
  align_left_btn_->setIcon(icon::TextAlignLeft);
  connect(align_left_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignLeft);});
  basic_layout->addWidget(align_left_btn_);

  align_center_btn_ = new QPushButton();
  align_center_btn_->setCheckable(true);
  align_center_btn_->setIcon(icon::TextAlignCenter);
  connect(align_center_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignCenter);});
  basic_layout->addWidget(align_center_btn_);

  align_right_btn_ = new QPushButton();
  align_right_btn_->setCheckable(true);
  align_right_btn_->setIcon(icon::TextAlignRight);
  connect(align_right_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignRight);});
  basic_layout->addWidget(align_right_btn_);

  align_justify_btn_ = new QPushButton();
  align_justify_btn_->setCheckable(true);
  align_justify_btn_->setIcon(icon::TextAlignJustify);
  connect(align_justify_btn_, &QPushButton::clicked, this, [this]{emit AlignmentChanged(Qt::AlignJustify);});
  basic_layout->addWidget(align_justify_btn_);

  basic_layout->addWidget(QtUtils::CreateVerticalLine());

  color_btn_ = new QPushButton();
  color_btn_->setAutoFillBackground(true);
  connect(color_btn_, &QPushButton::clicked, this, [this]{
    QColor c = color_btn_->property("color").value<QColor>();

    QColorDialog cd(c, this);
    if (cd.exec() == QDialog::Accepted) {
      c = cd.selectedColor();
      SetColor(c);
      emit ColorChanged(c);
    }
  });
  basic_layout->addWidget(color_btn_);

  basic_layout->addStretch();

  QHBoxLayout *advanced_layout = new QHBoxLayout();
  outer_layout->addLayout(advanced_layout);

  advanced_layout->addWidget(new QLabel(tr("Stretch: "))); // FIXME: Procure icon

  stretch_slider_ = new IntegerSlider();
  stretch_slider_->SetMinimum(0);
  stretch_slider_->SetDefaultValue(100);
  stretch_slider_->setFixedWidth(advanced_slider_width);
  stretch_slider_->SetFormat(tr("%1%"));
  connect(stretch_slider_, &IntegerSlider::ValueChanged, this, &ViewerTextEditorToolBar::StretchChanged);
  advanced_layout->addWidget(stretch_slider_);

  advanced_layout->addWidget(new QLabel(tr("Kerning: "))); // FIXME: Procure icon

  kerning_slider_ = new FloatSlider();
  kerning_slider_->SetMinimum(0);
  kerning_slider_->SetDefaultValue(100);
  kerning_slider_->SetDecimalPlaces(1);
  kerning_slider_->setFixedWidth(advanced_slider_width);
  kerning_slider_->SetFormat(tr("%1%"));
  connect(kerning_slider_, &FloatSlider::ValueChanged, this, &ViewerTextEditorToolBar::KerningChanged);
  advanced_layout->addWidget(kerning_slider_);

  advanced_layout->addWidget(new QLabel(tr("Line Height: "))); // FIXME: Procure icon

  line_height_slider_ = new FloatSlider();
  line_height_slider_->SetMinimum(0);
  line_height_slider_->SetDefaultValue(100);
  line_height_slider_->SetDecimalPlaces(1);
  line_height_slider_->setFixedWidth(advanced_slider_width);
  line_height_slider_->SetFormat(tr("%1%"));
  connect(line_height_slider_, &FloatSlider::ValueChanged, this, &ViewerTextEditorToolBar::LineHeightChanged);
  advanced_layout->addWidget(line_height_slider_);

  small_caps_btn_ = new QPushButton();
  small_caps_btn_->setIcon(icon::TextSmallCaps);
  small_caps_btn_->setCheckable(true);
  connect(small_caps_btn_, &QPushButton::clicked, this, &ViewerTextEditorToolBar::SmallCapsChanged);
  advanced_layout->addWidget(small_caps_btn_);

  advanced_layout->addStretch();

  setAutoFillBackground(true);

  resize(sizeHint());
}

void ViewerTextEditorToolBar::SetAlignment(Qt::Alignment a)
{
  align_left_btn_->setChecked(a == Qt::AlignLeft);
  align_center_btn_->setChecked(a == Qt::AlignCenter);
  align_right_btn_->setChecked(a == Qt::AlignRight);
  align_justify_btn_->setChecked(a == Qt::AlignJustify);
}

void ViewerTextEditorToolBar::SetColor(const QColor &c)
{
  color_btn_->setProperty("color", c);
  color_btn_->setStyleSheet(QStringLiteral("QPushButton { background: %1; }").arg(c.name()));
}

void ViewerTextEditorToolBar::closeEvent(QCloseEvent *event)
{
  event->ignore();
}

void ViewerTextEditorToolBar::paintEvent(QPaintEvent *event)
{
  if (!painted_) {
    emit FirstPaint();
    painted_ = true;
  }
  QWidget::paintEvent(event);
}

void ViewerTextEditorToolBar::UpdateFontStyleList(const QString &family)
{
  QString temp = style_combo_->currentText();

  style_combo_->blockSignals(true);
  style_combo_->clear();
  QStringList l = QFontDatabase().styles(family);
  foreach (const QString &style, l) {
    style_combo_->addItem(style);
  }
  style_combo_->setCurrentText(temp);
  style_combo_->blockSignals(false);
}

void ViewerTextEditorToolBar::UpdateFontStyleListAndEmitFamilyChanged(const QString &family)
{
  // Ensures correct ordering of commands
  UpdateFontStyleList(family);
  emit FamilyChanged(family);
}

void ViewerTextEditorToolBar::mousePressEvent(QMouseEvent *event)
{
  QWidget::mousePressEvent(event);

  if (event->button() == Qt::LeftButton && drag_enabled_) {
    drag_anchor_ = event->pos();
  }
}

void ViewerTextEditorToolBar::mouseMoveEvent(QMouseEvent *event)
{
  QWidget::mouseMoveEvent(event);

  if ((event->buttons() & Qt::LeftButton) && drag_enabled_) {
    this->move(mapToParent(QPoint(event->pos() - drag_anchor_)));
  }
}

void ViewerTextEditorToolBar::mouseReleaseEvent(QMouseEvent *event)
{
  QWidget::mouseReleaseEvent(event);
}

}
