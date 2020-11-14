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

#ifndef OTIOPROPERTIESDIALOG_H
#define OTIOPROPERTIESDIALOG_H

#include <QDialog>
#include <QTreeWidget>

#include "common/define.h"
#include "opentimelineio/timeline.h"
#include "project/item/sequence/sequence.h"
#include "project/project.h"

OLIVE_NAMESPACE_ENTER

/**
 * @brief Dialog to load setting for OTIO sequences.
 *
 * Takes a list of Sequences and allows the setting of options for each.
 */
class OTIOPropertiesDialog : public QDialog 
{
  Q_OBJECT
public:
  OTIOPropertiesDialog(QList<SequencePtr> sequences, ProjectPtr active_project, QWidget* parent = nullptr);

private:
  QTreeWidget* table_;

  QList<SequencePtr> sequences_;

private slots:
  /**
   * @brief Brings up the Sequence settings dialog.
   */
  void SetupSequence();
};


OLIVE_NAMESPACE_EXIT

#endif // OTIOPROPERTIESDIALOG_H
