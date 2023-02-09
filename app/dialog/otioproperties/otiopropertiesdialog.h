
#ifndef OTIOPROPERTIESDIALOG_H
#define OTIOPROPERTIESDIALOG_H

#include <QDialog>
#include <QTreeWidget>

#include "common/define.h"
#include "opentimelineio/timeline.h"
#include "node/project/sequence/sequence.h"
#include "node/project/project.h"

namespace olive {

  /**
   * @brief Dialog to load setting for OTIO sequences.
   *
   * Takes a list of Sequences and allows the setting of options for each.
   */
  class OTIOPropertiesDialog : public QDialog {
    Q_OBJECT
   public:
    OTIOPropertiesDialog(const QList<Sequence*>& sequences, Project* active_project, QWidget* parent = nullptr);

   private:
    QTreeWidget* table_;

    const QList<Sequence*> sequences_;

   private slots:
    /**
     * @brief Brings up the Sequence settings dialog.
     */
    void SetupSequence();
  };

} //namespace olive

#endif  // OTIOPROPERTIESDIALOG_H
