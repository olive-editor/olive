#ifndef PROJECTTOOLBAR_H
#define PROJECTTOOLBAR_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>

class ProjectToolbar : public QWidget
{
  Q_OBJECT
public:
  ProjectToolbar(QWidget* parent);
protected:
  void changeEvent(QEvent *) override;
signals:
  void NewClicked();
  void OpenClicked();
  void SaveClicked();

  void UndoClicked();
  void RedoClicked();

  void SearchChanged(const QString&);

  void TreeViewClicked();
  void IconViewClicked();
  void ListViewClicked();
private:
  void Retranslate();

  QPushButton* new_button_;
  QPushButton* open_button_;
  QPushButton* save_button_;
  QPushButton* undo_button_;
  QPushButton* redo_button_;

  QLineEdit* search_field_;

  QPushButton* tree_button_;
  QPushButton* list_button_;
  QPushButton* icon_button_;
};

#endif // PROJECTTOOLBAR_H
