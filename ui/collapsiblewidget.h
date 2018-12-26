#ifndef COLLAPSIBLEWIDGET_H
#define COLLAPSIBLEWIDGET_H

#include <QWidget>
class QLabel;
class QCheckBox;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
class QFrame;
class CheckboxEx;

class CollapsibleWidgetHeader : public QWidget {
    Q_OBJECT
public:
    CollapsibleWidgetHeader(QWidget* parent = 0);
    bool selected;
protected:
    void mousePressEvent(QMouseEvent* event);
	void paintEvent(QPaintEvent *event);
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
	bool is_expanded();

	CheckboxEx* enabled_check;
    bool selected;
	QWidget* contents;
	CollapsibleWidgetHeader* title_bar;
private:
	QLabel* header;
	QVBoxLayout* layout;
	QPushButton* collapse_button;
	QFrame* line;
    QHBoxLayout* title_bar_layout;
    void set_button_icon(bool open);

signals:
    void deselect_others(QWidget*);
	void visibleChanged();

private slots:
	void on_enabled_change(bool b);
	void on_visible_change();

public slots:
    void header_click(bool s, bool deselect);
};

#endif // COLLAPSIBLEWIDGET_H
