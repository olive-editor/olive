/***

  Olive - Non-Linear Video Editor
  Copyright (C) 2019  Olive Team

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

***/

#ifndef LABELSLIDER_H
#define LABELSLIDER_H

#include <QLabel>
#include <QUndoCommand>

/**
 * @brief The LabelSlider class
 *
 * A UI element that shows a number and can be dragged to increase/decrease its value or clicked to enter a specific
 * one.
 */
class LabelSlider : public QLabel
{
  Q_OBJECT
public:
  LabelSlider(QWidget* parent = nullptr);

  /**
   * @brief Set the display frame rate
   *
   * If the `display_type` is set to LABELSLIDER_FRAMENUMBER, this function sets how many frames per second the
   * timecode will be in.
   *
   * @param d
   */
  void set_frame_rate(double d);

  enum DisplayType {
    LABELSLIDER_NORMAL,
    LABELSLIDER_FRAMENUMBER,
    LABELSLIDER_PERCENT,
    LABELSLIDER_DECIBEL
  };

  /**
   * @brief Sets the way to display the value
   *
   * * `LABELSLIDER_NORMAL` - Shows the value as a normal number
   * * `LABELSLIDER_FRAMENUMBER` - Shows the number as a timecode according to `config.timecode_view`. By default,
   * will render hh:mm:ss:ff
   * * `LABELSLIDER_PERCENT` - Shows the number as a percentage. 1.0 becomes "100%", 0.5 becomes 50%, etc.
   * * `LABELSLIDER_DECIBLE` - Shows the number as a decibel. 1.0 becomes "0 dB", 2.0 becames roughly "6 dB", 0.5
   * becomes roughly "-6 dB", etc.
   *
   * @param type
   *
   * The display type to set to.
   */
  void set_display_type(const DisplayType& type);

  /**
   * @brief Set the value
   * @param v
   *
   * Value to set to.
   *
   * @param userSet
   *
   * **TRUE** if this was called through a user action, **FALSE** if this was called through some other way. The
   * only difference is **TRUE** will emit a signal (valueChanged()) indicating that the value has changed, and
   * **FALSE** will not.
   */
  void set_value(double v, bool userSet);

  /**
   * @brief Set the default value
   *
   * If a default value is set, alt+clicking the LabelSlider will return to the default value.
   *
   * @param v
   *
   * Value to set as default
   */
  void set_default_value(double v);

  /**
   * @brief Set the minimum value
   *
   * If a minimum value is set, the value will never go below it. If the user manually sets a value lower than
   * the minimum, it will automatically snap to the minimum.
   *
   * @param v
   *
   * Value to set as minimum
   */
  void set_minimum_value(double v);

  /**
   * @brief Set the maximum value
   *
   * If a maximum value is set, the value will never go above it. If the user manually sets a value higher than
   * the maximum, it will automatically snap to the maximum.
   *
   * @param v
   *
   * Value to set as maximum
   */
  void set_maximum_value(double v);

  /**
   * @brief Returns the internal value as a double
   * @return The internal value. This will not respect the `display_type`, i.e. 100% will return as 1.0, 12dB will
   * return as 400%, and a timecode will return as a frame number.
   */
  double value();

  /**
   * @brief Returns whether a value has been set or not
   *
   * If a value has been entered by any means (i.e. if set_value() was called), this will be **TRUE**. If set_value()
   * has not been called and is_set() is **FALSE**, calling set_default_value() will automatically set the value
   * to the default value WITHOUT changing the is_set() state (i.e. it will still be **FALSE** and calling
   * set_default_value() again will change the current value again unless it's been changed through some other means
   * in that time).
   *
   * @return **TRUE** if a value has been set, **FALSE** if not.
   */
  bool is_set();

  /**
   * @brief Returns whether the user is currently dragging
   * @return **TRUE** if the user is dragging, **FALSE** if not.
   */
  bool is_dragging();

  /**
   * @brief Convert the internal value to a displayed string according to `display_type`
   * @return The internal value as a string
   */
  QString valueToString();

  /**
   * @brief Returns whatever value was set before the last set_value()
   *
   * For various reasons (largely undo capabilities) it is helpful to retrieve whatever the value was before the
   * current one. If the user dragged the current value, this will return the value just before the user started
   * dragging.
   *
   * @return The previous value
   */
  double getPreviousValue();

  /**
   * @brief Updates previous value
   *
   * Called internally to store the current value as the previous value in anticipation of an upcoming value change.
   * Can also be called externally (mainly for dragged EffectGizmos) in anticipation of an external change.
   */
  void set_previous_value();

  /**
   * @brief Set the display color
   * @param c
   *
   * Color to set to
   */
  void set_color(QString c = nullptr);

  /**
   * @brief Set how many decimal places to show for a floating-point number
   *
   * Defaults to 1
   */
  int decimal_places;
protected:
  void mousePressEvent(QMouseEvent *ev);
  void mouseMoveEvent(QMouseEvent *ev);
  void mouseReleaseEvent(QMouseEvent *ev);
private:
  double default_value;
  double internal_value;
  double drag_start_value;
  double previous_value;

  bool min_enabled;
  double min_value;
  bool max_enabled;
  double max_value;

  bool drag_start;
  bool drag_proc;
  int drag_start_x;
  int drag_start_y;

  bool set;

  DisplayType display_type;

  double frame_rate;

  /**
   * @brief Internal function to set the standard cursor (usually SizeHorCursor)
   */
  void set_default_cursor();

  /**
   * @brief Internal function to set the cursor while dragging (usually NoCursor aka invisible)
   */
  void set_active_cursor();
private slots:
  /**
   * @brief Context menu slot to be connected to QWidget::customContextMenuRequested(const QPoint&)
   *
   * @param pos
   *
   * Current cursor position
   */
  void show_context_menu(const QPoint& pos);

  /**
   * @brief Slot to reset current value to the default value
   */
  void reset_to_default_value();

  /**
   * @brief Show prompt asking for value
   *
   * Triggered when the user clicks on the LabelSlider without dragging or triggers right click > Edit.
   * Will show an input dialog asking for a new value to be entered manually. Asks for units in the selected
   * display type and converts them back to the internal value type.
   */
  void prompt_for_value();
signals:
  /**
   * @brief valueChanged signal
   *
   * Emitted if the value changed at it was instigated by the user.
   */
  void valueChanged(double d);

  /**
   * @brief clicked signal
   *
   * Emitted if the user clicks on the LabelSlider in any way
   */
  void clicked();
};

#endif // LABELSLIDER_H
