#include "trackviewitem.h"

#include <QDebug>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QtMath>

TrackViewItem::TrackViewItem(Qt::Alignment alignment, QWidget *parent) :
  QWidget(parent),
  alignment_(alignment)
{
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setSpacing(0);
  layout->setMargin(0);

  label_ = new QLabel(tr("Track"));
  layout->addWidget(label_);

  mute_button_ = CreateMSLButton(tr("M"), Qt::red);
  layout->addWidget(mute_button_);

  solo_button_ = CreateMSLButton(tr("S"), Qt::yellow);
  layout->addWidget(solo_button_);

  lock_button_ = CreateMSLButton(tr("L"), Qt::gray);
  layout->addWidget(lock_button_);

  setMinimumHeight(mute_button_->height());
}

QPushButton *TrackViewItem::CreateMSLButton(const QString& text, const QColor& checked_color) const
{
  QPushButton* button = new QPushButton(text);
  button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  button->setCheckable(true);
  button->setStyleSheet(QStringLiteral("QPushButton::checked { background: %1; }").arg(checked_color.name()));

  int size = button->sizeHint().height();
  size = qRound(size * 0.75);
  button->setFixedSize(size, size);

  return button;
}
