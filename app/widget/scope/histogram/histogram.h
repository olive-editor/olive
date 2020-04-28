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

#ifndef HISTOGRAMSCOPE_H
#define HISTOGRAMSCOPE_H

#include <QMutex>
#include <QThread>
#include <QWaitCondition>

#include "codec/frame.h"
#include "render/colorprocessor.h"
#include "widget/manageddisplay/manageddisplay.h"

OLIVE_NAMESPACE_ENTER

class HistogramScopeWorker : public QThread
{
  Q_OBJECT
public:
  HistogramScopeWorker();

  // Thread-safe
  void QueueNext(const Frame& f, ColorProcessorPtr processor, int width);

  // Thread-safe
  void Cancel();

protected:
  virtual void run() override;

signals:
  void Finished(QVector<double> red, QVector<double> green, QVector<double> blue);

private:
  QAtomicInt cancelled_;

  QMutex next_lock_;
  QWaitCondition next_wait_;
  Frame next_;
  int next_width_;
  ColorProcessorPtr next_processor_;

};

class HistogramScope : public ManagedDisplayWidget
{
  Q_OBJECT
public:
  HistogramScope(QWidget* parent = nullptr);

  virtual ~HistogramScope() override;

public slots:
  void SetBuffer(Frame* frame);

protected:
  virtual void paintGL() override;

  virtual void resizeEvent(QResizeEvent* e) override;

  virtual void ColorProcessorChangedEvent() override;

  virtual void showEvent(QShowEvent* e) override;

private:
  void StartUpdate();

  Frame* buffer_;

  QVector<double> red_val_;

  QVector<double> green_val_;

  QVector<double> blue_val_;

  HistogramScopeWorker worker_;

private slots:
  void FinishedProcessing(QVector<double> red, QVector<double> green, QVector<double> blue);

};

OLIVE_NAMESPACE_EXIT

#endif // HISTOGRAMSCOPE_H
