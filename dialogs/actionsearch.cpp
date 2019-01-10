#include "actionsearch.h"

#include <QVBoxLayout>
#include <QKeyEvent>
#include <QMenuBar>
#include <QLabel>

#include "mainwindow.h"

ActionSearch::ActionSearch(QWidget *parent) :
	QDialog(parent)
{
	setObjectName("ASDiag");
	setStyleSheet("#ASDiag{border: 2px solid #808080;}");

	resize(parent->width()/3, parent->height()/3);

	setWindowFlags(Qt::Popup);

	QVBoxLayout* layout = new QVBoxLayout();

	ActionSearchEntry* entry_field = new ActionSearchEntry();
	QFont entry_field_font = entry_field->font();
	entry_field_font.setPointSize(entry_field_font.pointSize()*1.2);
	entry_field->setFont(entry_field_font);
	entry_field->setPlaceholderText("Search for action...");
	connect(entry_field, SIGNAL(textChanged(const QString&)), this, SLOT(search_update(const QString &)));
	connect(entry_field, SIGNAL(returnPressed()), this, SLOT(perform_action()));
	connect(entry_field, SIGNAL(moveSelectionUp()), this, SLOT(move_selection_up()));
	connect(entry_field, SIGNAL(moveSelectionDown()), this, SLOT(move_selection_down()));
	layout->addWidget(entry_field);

	list_widget = new ActionSearchList();
	QFont list_widget_font = list_widget->font();
	list_widget_font.setPointSize(list_widget_font.pointSize()*1.2);
	list_widget->setFont(list_widget_font);
	layout->addWidget(list_widget);
	connect(list_widget, SIGNAL(dbl_click()), this, SLOT(perform_action()));

	setLayout(layout);

	entry_field->setFocus();
}

void ActionSearch::search_update(const QString &s, const QString &p, QMenu *parent) {
	if (parent == NULL) {
		list_widget->clear();
		QList<QAction*> menus = mainWindow->menuBar()->actions();
		for (int i=0;i<menus.size();i++) {
			QMenu* menu = menus.at(i)->menu();
			search_update(s, p, menu);
		}
		if (list_widget->count() > 0)
			list_widget->item(0)->setSelected(true);
	} else {
		QString menu_text;
		if (!p.isEmpty()) menu_text += p + " > ";
		menu_text += parent->title().replace("&", "");
		QList<QAction*> actions = parent->actions();
		for (int i=0;i<actions.size();i++) {
			QAction* a = actions.at(i);
			if (!a->isSeparator()) {
				if (a->menu() != NULL) {
					search_update(s, menu_text, a->menu());
				} else {
					QString comp = a->text().replace("&", "");
					if (comp.contains(s, Qt::CaseInsensitive)) {
						QListWidgetItem* item = new QListWidgetItem(comp + "\n(" + menu_text + ")");
						item->setData(Qt::UserRole+1, reinterpret_cast<quintptr>(a));
						list_widget->addItem(item);
					}
				}
			}
		}
	}
}

void ActionSearch::perform_action() {
	QList<QListWidgetItem*> selected_items = list_widget->selectedItems();
	if (list_widget->count() > 0 && selected_items.size() > 0) {
		QListWidgetItem* item = selected_items.at(0);
		QAction* a = reinterpret_cast<QAction*>(item->data(Qt::UserRole+1).value<quintptr>());
		a->trigger();
	}
	accept();
}

void ActionSearch::move_selection_up() {
	int lim = list_widget->count();
	for (int i=1;i<lim;i++) {
        if (list_widget->item(i)->isSelected()) {
			list_widget->item(i-1)->setSelected(true);
            list_widget->scrollToItem(list_widget->item(i-1));
			break;
		}
	}
}

void ActionSearch::move_selection_down() {
	int lim = list_widget->count()-1;
	for (int i=0;i<lim;i++) {
        if (list_widget->item(i)->isSelected()) {
			list_widget->item(i+1)->setSelected(true);
            list_widget->scrollToItem(list_widget->item(i+1));
			break;
		}
	}
}

void ActionSearchEntry::keyPressEvent(QKeyEvent * event) {
	switch (event->key()) {
	case Qt::Key_Up:
		emit moveSelectionUp();
		break;
	case Qt::Key_Down:
		emit moveSelectionDown();
		break;
	default:
		QLineEdit::keyPressEvent(event);
	}
}

void ActionSearchList::mouseDoubleClickEvent(QMouseEvent *event) {
	emit dbl_click();
}
