#include "preferencesdialog.h"

#include "io/config.h"

#include <QMenuBar>
#include <QAction>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QDialogButtonBox>
#include <QTreeWidget>
#include <QVector>

KeySequenceEditor::KeySequenceEditor(QWidget* parent, QAction* a)
	: QKeySequenceEdit(parent), action(a) {
	setKeySequence(action->shortcut());
	//connect(this, SIGNAL(editingFinished()), this, SLOT(set_action_shortcut()));
}

void KeySequenceEditor::set_action_shortcut() {
	action->setShortcut(keySequence());
}

PreferencesDialog::PreferencesDialog(QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle("Preferences");
	setup_ui();

	accurateSeekButton->setChecked(!config.fast_seeking);
	fastSeekButton->setChecked(config.fast_seeking);
	recordingComboBox->setCurrentIndex(config.recording_mode - 1);
	imgSeqFormatEdit->setText(config.img_seq_formats);
}

PreferencesDialog::~PreferencesDialog() {}

void PreferencesDialog::setup_kbd_shortcut_worker(QMenu* menu, QTreeWidgetItem* parent) {
	QList<QAction*> actions = menu->actions();
	for (int i=0;i<actions.size();i++) {
		QAction* a = actions.at(i);

		if (!a->isSeparator()) {
			QTreeWidgetItem* item = new QTreeWidgetItem();
			item->setText(0, a->text().replace("&", ""));

			parent->addChild(item);

			if (a->menu() != NULL) {
				setup_kbd_shortcut_worker(a->menu(), item);
			} else {
				key_shortcut_items.append(item);
				key_shortcut_actions.append(a);
			}
		}
	}
}

void PreferencesDialog::setup_kbd_shortcuts(QMenuBar* menubar) {
	QList<QAction*> menus = menubar->actions();

	for (int i=0;i<menus.size();i++) {
		QMenu* menu = menus.at(i)->menu();

		QTreeWidgetItem* item = new QTreeWidgetItem();
		item->setText(0, menu->title().replace("&", ""));

		keyboard_tree->addTopLevelItem(item);

		setup_kbd_shortcut_worker(menu, item);
	}

	for (int i=0;i<key_shortcut_items.size();i++) {
		KeySequenceEditor* editor = new KeySequenceEditor(keyboard_tree, key_shortcut_actions.at(i));
		keyboard_tree->setItemWidget(key_shortcut_items.at(i), 1, editor);
		key_shortcut_fields.append(editor);
	}
}

void PreferencesDialog::save() {
	config.recording_mode = recordingComboBox->currentIndex() + 1;
	config.img_seq_formats = imgSeqFormatEdit->text();
	config.fast_seeking = fastSeekButton->isChecked();

	// save keyboard shortcuts
	for (int i=0;i<key_shortcut_fields.size();i++) {
		key_shortcut_fields.at(i)->set_action_shortcut();
	}

	accept();
}

void PreferencesDialog::setup_ui() {
	QVBoxLayout* verticalLayout = new QVBoxLayout(this);
	QTabWidget* tabWidget = new QTabWidget(this);

	QTabWidget* tab = new QTabWidget();
	QGridLayout* gridLayout_2 = new QGridLayout(tab);

	gridLayout_2->addWidget(new QLabel("Image sequence formats:"), 0, 0, 1, 1);

	imgSeqFormatEdit = new QLineEdit(tab);

	gridLayout_2->addWidget(imgSeqFormatEdit, 0, 1, 1, 1);

	gridLayout_2->addWidget(new QLabel("Audio Recording:"), 1, 0, 1, 1);

	recordingComboBox = new QComboBox(tab);
	recordingComboBox->addItem("Mono");
	recordingComboBox->addItem("Stereo");

	gridLayout_2->addWidget(recordingComboBox, 1, 1, 1, 1);

	tabWidget->addTab(tab, "General");
	QWidget* tab_2 = new QWidget();
	tabWidget->addTab(tab_2, "Behavior");
	QWidget* tab_4 = new QWidget();
	QVBoxLayout* verticalLayout_2 = new QVBoxLayout(tab_4);
	QGroupBox* groupBox = new QGroupBox(tab_4);
	groupBox->setTitle("Seeking");
	QVBoxLayout* verticalLayout_3 = new QVBoxLayout(groupBox);
	accurateSeekButton = new QRadioButton(groupBox);
	accurateSeekButton->setText("Accurate Seeking\nAlways show the correct frame (visual may pause briefly as correct frame is retrieved)");

	verticalLayout_3->addWidget(accurateSeekButton);

	fastSeekButton = new QRadioButton(groupBox);
	fastSeekButton->setText("Fast Seeking\nSeek quickly (may briefly show inaccurate frames when seeking - doesn't affect playback/export)");

	verticalLayout_3->addWidget(fastSeekButton);

	verticalLayout_2->addWidget(groupBox);

	tabWidget->addTab(tab_4, "Playback");
	QWidget* tab_3 = new QWidget();
	QHBoxLayout* horizontalLayout = new QHBoxLayout(tab_3);
	horizontalLayout->setContentsMargins(0, 0, 0, 0);

	keyboard_tree = new QTreeWidget();
	QTreeWidgetItem* tree_header = keyboard_tree->headerItem();
	tree_header->setText(0, "Action");
	tree_header->setText(1, "Shortcut");
	horizontalLayout->addWidget(keyboard_tree);

	tabWidget->addTab(tab_3, "Keyboard");

	verticalLayout->addWidget(tabWidget);

	QDialogButtonBox* buttonBox = new QDialogButtonBox(this);
	buttonBox->setOrientation(Qt::Horizontal);
	buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

	verticalLayout->addWidget(buttonBox);

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(save()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	tabWidget->setCurrentIndex(2);
}
