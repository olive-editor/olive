#ifndef COLLAPSIBLEWIDGET_H
#define COLLAPSIBLEWIDGET_H

#include <QWidget>
class QLabel;
class QCheckBox;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
class QFrame;

class CollapsibleWidget : public QWidget
{
	Q_OBJECT
public:
	CollapsibleWidget(QWidget* parent = 0);
	void setContents(QWidget* c);
	void setText(const QString &);
private:
	QLabel* header;
	QCheckBox* enabled_check;
	QHBoxLayout* title_bar;
	QVBoxLayout* layout;
	QPushButton* collapse_button;
	QWidget* contents;
	QFrame* line;

private slots:
	void on_enabled_change(bool b);
	void on_visible_change();
};

#endif // COLLAPSIBLEWIDGET_H
