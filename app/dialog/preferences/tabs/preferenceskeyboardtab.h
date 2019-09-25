#ifndef PREFERENCESKEYBOARDTAB_H
#define PREFERENCESKEYBOARDTAB_H

#include <QMenuBar>
#include <QTreeWidget>

#include "preferencestab.h"
#include "../keysequenceeditor.h"

class PreferencesKeyboardTab : public PreferencesTab
{
  Q_OBJECT
public:
  PreferencesKeyboardTab(QMenuBar* menubar);

  virtual void Accept() override;

private slots:
  /**
   * @brief Show a file dialog to load an external shortcut preset from file
   */
  void load_shortcut_file();

  /**
   * @brief Show a file dialog to save an external shortcut preset from file
   */
  void save_shortcut_file();

  /**
   * @brief Reset all selected shortcuts in keyboard_tree to their defaults
   */
  void reset_default_shortcut();

  /**
   * @brief Reset all shortcuts indiscriminately to their defaults
   *
   * This is safe to call directly as it'll ask the user if they wish to do so before it resets.
   */
  void reset_all_shortcuts();

  /**
   * @brief Shows/hides shortcut entries according to a shortcut query.
   *
   * This function can be directly connected to QLineEdit::textChanged() for simplicity.
   *
   * @param s
   *
   * The search query to compare shortcut names to.
   *
   * @param parent
   *
   * This is used as the function calls itself recursively to traverse the menu item hierarchy. This should be left as
   * nullptr when called externally.
   *
   * @return
   *
   * Value used as function calls itself recursively to determine if a menu parent has any children that are not hidden.
   * If so, TRUE is returned so the parent is shown too (even if it doesn't match the search query). If not, FALSE is
   * returned so the parent is hidden.
   */
  bool refine_shortcut_list(const QString &s, QTreeWidgetItem* parent = nullptr);

private:
  /**
   * @brief Populate keyboard shortcut panel with keyboard shortcuts from the menu bar
   *
   * @param menu
   *
   * A reference to the main application's menu bar. Usually MainWindow::menuBar().
   */
  void setup_kbd_shortcuts(QMenuBar* menu);

  /**
   * @brief Internal function called by setup_kbd_shortcuts() to traverse down the menu bar's hierarchy and populate the
   * shortcut panel.
   *
   * This function will call itself recursively as it finds submenus belong to the menu provided. It will also create
   * QTreeWidgetItems as children of the parent item provided, either using them as parents themselves for submenus
   * or attaching a KeySequenceEditor to them for shortcut editing.
   *
   * @param menu
   *
   * The current menu to traverse down.
   *
   * @param parent
   *
   * The parent item to add QTreeWidgetItems to.
   */
  void setup_kbd_shortcut_worker(QMenu* menu, QTreeWidgetItem* parent);

  /**
   * @brief UI widget for editing keyboard shortcuts
   */
  QTreeWidget* keyboard_tree_;

  /**
   * @brief List of keyboard shortcut actions that can be triggered (links with key_shortcut_items and
   * key_shortcut_fields)
   */
  QVector<QAction*> key_shortcut_actions_;

  /**
   * @brief List of keyboard shortcut items in keyboard_tree corresponding to existing actions (links with
   * key_shortcut_actions and key_shortcut_fields)
   */
  QVector<QTreeWidgetItem*> key_shortcut_items_;

  /**
   * @brief List of keyboard shortcut editing fields in keyboard_tree corresponding to existing actions (links with
   * key_shortcut_actions and key_shortcut_fields)
   */
  QVector<KeySequenceEditor*> key_shortcut_fields_;
};

#endif // PREFERENCESKEYBOARDTAB_H
