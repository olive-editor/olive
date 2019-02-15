#ifndef MENUHELPER_H
#define MENUHELPER_H

#include <QObject>
#include <QMenu>

class MenuHelper : public QObject {
    Q_OBJECT
public:
    /**
     * @brief Creates a menu of new items that can be created
     *
     * Adds the full set of creatable items to a QMenu (e.g. new project,
     * new sequence, new folder, etc.)
     *
     * @param parent
     *
     * The menu to add items to.
     */
    void make_new_menu(QMenu* parent);

    /**
     * @brief Creates a menu of options for working with in/out points
     *
     * Adds a set of options for working with sequence/footage in/out points,
     * e.g. setting in/out points, clearing in/out points, etc.
     *
     * @param parent
     *
     * The menu to add items to.
     */
    void make_inout_menu(QMenu* parent);

    /**
     * @brief Creates a menu of clip functions
     *
     * Adds a set of clip functions including:
     * * Add Default Transition
     * * Link/Unlink
     * * Enable/Disable
     * * Nest
     *
     * @param parent
     *
     * The menu to add items to.
     */
    void make_clip_functions_menu(QMenu* parent);

    /**
     * @brief Creates standard edit menu (cut, copy, paste, etc.)
     *
     * @param parent
     *
     * The menu to add items to.
     */
    void make_edit_functions_menu(QMenu* parent);

    void set_bool_action_checked(QAction* a);
    void set_int_action_checked(QAction* a, const int& i);
    void set_button_action_checked(QAction* a);

public slots:

    void toggle_bool_action();

    void set_titlesafe_from_menu();

    void set_autoscroll();
    void menu_click_button();
    void set_timecode_view();

private slots:


};

namespace Olive {
    /**
     * @brief A global MenuHelper object to assist menu creation throughout Olive.
     */
    extern MenuHelper MenuHelper;
}

#endif // MENUHELPER_H
