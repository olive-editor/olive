/***

    Olive - Non-Linear Video Editor
    Copyright (C) 2019  Olive Team

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

#ifndef ADVANCEDVIDEODIALOG_H
#define ADVANCEDVIDEODIALOG_H

#include <QDialog>

#include "io/exportthread.h"

class QComboBox;

class AdvancedVideoDialog : public QDialog {
    Q_OBJECT
public:
    AdvancedVideoDialog(QWidget* parent,
                        int encoding_codec,
                        VideoCodecParams& iparams);

public slots:
    virtual void accept() override;
private:
    VideoCodecParams& params;

    QComboBox* pix_fmt_combo;
};

#endif // ADVANCEDVIDEODIALOG_H
