/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019 Olive Team

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

#include "timelineviewblockitem.h"

#include <QBrush>
#include <QCoreApplication>
#include <QDir>
#include <QFloat16>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

#include "audio/sumsamples.h"
#include "common/qtutils.h"
#include "config/config.h"
#include "node/block/transition/transition.h"

TimelineViewBlockItem::TimelineViewBlockItem(Block *block, QGraphicsItem* parent) :
  TimelineViewRect(parent),
  block_(block)
{
  setBrush(Qt::white);
  setCursor(Qt::DragMoveCursor);
  setFlag(QGraphicsItem::ItemIsSelectable,
          block_->type() == Block::kClip
          || block_->type() == Block::kGap
          || block_->type() == Block::kTransition);

  UpdateRect();
}

Block *TimelineViewBlockItem::block() const
{
  return block_;
}

void TimelineViewBlockItem::UpdateRect()
{
  double item_left = TimeToScene(block_->in());
  double item_width = TimeToScene(block_->length());

  // -1 on width and height so we don't overlap any adjacent clips
  setRect(0, y_, item_width - 1, height_);
  setPos(item_left, 0.0);

  setToolTip(QCoreApplication::translate("TimelineViewBlockItem",
                                         "%1\n\nIn: %2\nOut: %3\nMedia In: %4").arg(block_->Name(),
                                                                                    QString::number(block_->in().toDouble()),
                                                                                    QString::number(block_->out().toDouble()),
                                                                                    QString::number(block_->media_in().toDouble())));
}

void TimelineViewBlockItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  switch (block_->type()) {
  case Block::kClip:
  {
    QLinearGradient grad;
    grad.setStart(0, rect().top());
    grad.setFinalStop(0, rect().bottom());
    grad.setColorAt(0.0, QColor(160, 160, 240));
    grad.setColorAt(1.0, QColor(128, 128, 192));
    painter->fillRect(rect(), grad);

    if (option->state & QStyle::State_Selected) {
      painter->fillRect(rect(), QColor(0, 0, 0, 64));
    }

    // Draw waveform if one is available
    QFile wave_file(QDir(QDir(Config::Current()["DiskCachePath"].toString()).filePath("waveform")).filePath(QString::number(reinterpret_cast<quintptr>(block_))));
    if (wave_file.open(QFile::ReadOnly)){
      painter->setPen(QColor(64, 64, 64));

      QByteArray w = wave_file.readAll();

      // FIXME: Hardcoded channel count
      int channels = 2;

      const SampleSummer::Sum* samples = reinterpret_cast<const SampleSummer::Sum*>(w.constData());
      int nb_samples = w.size() / sizeof(SampleSummer::Sum);

      int sample_index, next_sample_index = 0;

      QVector<SampleSummer::Sum> summary;
      int summary_index = -1;

      int channel_height = rect().height() / channels;
      int channel_half_height = channel_height / 2;

      for (int i=0;i<rect().width();i++) {
        sample_index = next_sample_index;

        if (sample_index == nb_samples) {
          break;
        }

        next_sample_index = qMin(nb_samples,
                                 qFloor(static_cast<double>(SampleSummer::kSumSampleRate) * static_cast<double>(i+1) / this->GetScale()) * channels);

        if (summary_index != sample_index) {
          summary = SampleSummer::ReSumSamples(&samples[sample_index],
                                               qMax(channels, next_sample_index - sample_index),
                                               channels);
          summary_index = sample_index;
        }

        for (int j=0;j<summary.size();j++) {
          int channel_mid = channel_height * j + channel_half_height;

          painter->drawLine(i,
                            channel_mid + qRound(summary.at(j).min * channel_half_height),
                            i,
                            channel_mid + qRound(summary.at(j).max * channel_half_height));
        }
      }

      wave_file.close();
    }

    painter->setPen(Qt::white);
    painter->drawLine(rect().topLeft(), QPointF(rect().right(), rect().top()));
    painter->drawLine(rect().topLeft(), QPointF(rect().left(), rect().bottom() - 1));

    painter->setPen(Qt::white);
    painter->drawText(rect(), static_cast<int>(Qt::AlignLeft | Qt::AlignTop), block_->block_name());

    // Linked clips are underlined
    if (block_->HasLinks()) {
      QFontMetrics fm = painter->fontMetrics();
      int text_width = qMin(qRound(rect().width()), QFontMetricsWidth(fm, block_->block_name()));

      QPointF underline_start = rect().topLeft() + QPointF(0, fm.height());
      QPointF underline_end = underline_start + QPointF(text_width, 0);

      painter->drawLine(underline_start, underline_end);
    }

    painter->setPen(QColor(64, 64, 64));
    painter->drawLine(QPointF(rect().left(), rect().bottom() - 1), QPointF(rect().right(), rect().bottom() - 1));
    painter->drawLine(QPointF(rect().right(), rect().bottom() - 1), QPointF(rect().right(), rect().top()));
    break;
  }
  case Block::kGap:
    if (option->state & QStyle::State_Selected) {
      // FIXME: Make this palette or CSS
      painter->fillRect(rect(), QColor(255, 255, 255, 128));
    }
    break;
  case Block::kTransition:
  {
    QLinearGradient grad;
    grad.setStart(0, rect().top());
    grad.setFinalStop(0, rect().bottom());
    grad.setColorAt(0.0, QColor(192, 160, 224));
    grad.setColorAt(1.0, QColor(160, 128, 192));
    painter->setBrush(grad);
    painter->setPen(QPen(QColor(96, 80, 112), 1));
    painter->drawRect(rect());

    if (option->state & QStyle::State_Selected) {
      painter->fillRect(rect(), QColor(0, 0, 0, 64));
    }

    // Draw lines antialiased
    painter->setRenderHint(QPainter::Antialiasing);

    TransitionBlock* t = static_cast<TransitionBlock*>(block_);

    if (t->connected_out_block() && t->connected_in_block()) {

      // Draw line between out offset and in offset
      qreal crossover_line = rect().left();
      crossover_line += TimeToScene(t->out_offset());
      painter->drawLine(qRound(crossover_line),
                        qRound(rect().top()),
                        qRound(crossover_line),
                        qRound(rect().bottom()));

      // Draw lines to mid point
      QPointF mid_point(crossover_line, rect().center().y());
      painter->drawLine(rect().topLeft(), mid_point);
      painter->drawLine(rect().bottomLeft(), mid_point);
      painter->drawLine(rect().topRight(), mid_point);
      painter->drawLine(rect().bottomRight(), mid_point);

    } else if (t->connected_out_block()) {

      // Transition fades something out, we'll draw a line
      painter->drawLine(rect().topLeft(), rect().bottomRight());

    } else if (t->connected_in_block()) {

      // Transition fades something in, we'll draw a line
      painter->drawLine(rect().bottomLeft(), rect().topRight());

    }
    break;
  }
  }
}
