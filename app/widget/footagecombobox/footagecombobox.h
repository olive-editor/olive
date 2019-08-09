#ifndef FOOTAGECOMBOBOX_H
#define FOOTAGECOMBOBOX_H

#include <QComboBox>
#include <QMenu>

#include "project/item/footage/footage.h"
#include "project/project.h"

class FootageComboBox : public QComboBox
{
  Q_OBJECT
public:
  FootageComboBox(QWidget* parent = nullptr);

  virtual void showPopup() override;

  void SetRoot(const Folder *p);

  void SetOnlyShowReadyFootage(bool e);

  Footage* SelectedFootage();

public slots:
  void SetFootage(Footage* f);

signals:
  void FootageChanged(Footage* f);

private:
  void TraverseFolder(const Folder *f, QMenu* m);

  const Folder* root_;

  Footage* footage_;

  bool only_show_ready_footage_;
};

#endif // FOOTAGECOMBOBOX_H
