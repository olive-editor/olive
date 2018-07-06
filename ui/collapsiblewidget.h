#ifndef COLLAPSIBLEWIDGET_H
#define COLLAPSIBLEWIDGET_H

#include <QWidget>
class QLabel;
class QCheckBox;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
class QFrame;

class CollapsibleWidgetHeader : public QWidget {
    Q_OBJECT
public:
    CollapsibleWidgetHeader(QWidget* parent = 0);
    bool selected;
protected:
    void mousePressEvent(QMouseEvent* event);
signals:
    void select(bool, bool);
};

class CollapsibleWidget : public QWidget
{
	Q_OBJECT
public:
	CollapsibleWidget(QWidget* parent = 0);
	void setContents(QWidget* c);
	void setText(const QString &);
    bool is_focused();

    QCheckBox* enabled_check;
    bool selected;
private:
	QLabel* header;
    CollapsibleWidgetHeader* title_bar;
	QVBoxLayout* layout;
	QPushButton* collapse_button;
	QWidget* contents;
	QFrame* line;

signals:
    void deselect_others(QWidget*);

private slots:
	void on_enabled_change(bool b);
	void on_visible_change();

public slots:
    void header_click(bool s, bool deselect);
};

#endif // COLLAPSIBLEWIDGET_H
