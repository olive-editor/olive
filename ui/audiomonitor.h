/* 
 * Olive. Olive is a free non-linear video editor for Windows, macOS, and Linux.
 * Copyright (C) 2018  {{ organization }}
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef AUDIOMONITOR_H
#define AUDIOMONITOR_H

#include <QWidget>

class AudioMonitor : public QWidget
{
    Q_OBJECT
public:
    explicit AudioMonitor(QWidget *parent = 0);
    QVector<qint16> sample_cache;
    long sample_cache_offset;
    void reset();

protected:
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);

signals:

public slots:

private:
    QLinearGradient gradient;
//    QVector<bool> peaks;
};

#endif // AUDIOMONITOR_H
