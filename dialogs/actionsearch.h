#ifndef ACTIONSEARCH_H
#define ACTIONSEARCH_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>

class QListWidget;
class QMenu;

class ActionSearchList : public QListWidget {
	Q_OBJECT
protected:
	void mouseDoubleClickEvent(QMouseEvent *event);
signals:
	void dbl_click();
};

class ActionSearch : public QDialog
{
	Q_OBJECT
public:
	ActionSearch(QWidget* parent = 0);
private slots:
	void search_update(const QString& s, const QString &p = 0, QMenu *parent = NULL);
	void perform_action();
	void move_selection_up();
	void move_selection_down();
private:
	ActionSearchList* list_widget;
};

class ActionSearchEntry : public QLineEdit {
	Q_OBJECT
protected:
	void keyPressEvent(QKeyEvent * event);
signals:
	void moveSelectionUp();
	void moveSelectionDown();
};

#endif // ACTIONSEARCH_H
