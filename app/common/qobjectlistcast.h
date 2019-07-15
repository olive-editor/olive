#ifndef QOBJECTLISTCAST_H
#define QOBJECTLISTCAST_H

#include <QObject>

template<class T>
/**
 * @brief Statically cast a QObjectList (aka QList<QObject*>) to a QList of any type (must be a QObject derivative)
 *
 * Best used to convert the list from a QObject's children() list to a list of another type, assuming it's known that
 * all children are of a certain type.
 *
 * As a static cast, the objects in the QObjectList must all be of the SAME type or the behavior
 * is undefined.
 */
QList<T*> static_qobjectlist_cast(const QObjectList& list) {
  QList<T*> new_list;

  new_list.reserve(list.size());

  for (int i=0;i<list.size();i++) {
    new_list.append(static_cast<T*>(list.at(i)));
  }

  return new_list;
}

template<class T>
/**
 * @brief Dynamically cast a QObjectList (aka QList<QObject*>) to a QList of any type (must be a QObject derivative)
 *
 * Best used to convert the list from a QObject's children() list to a list of another type, assuming it's known that
 * all children are of a certain type.
 *
 * Unlike static_qobjectlist_cast(), not all of the objects must be of the same type. Objects that are of a different
 * type are not added to the list. Therefore it is guarantee the list will contain valid objects of the type you
 * specify (or an empty list if there are none).
 */
QList<T*> dynamic_qobjectlist_cast(const QObjectList& list) {
  QList<T*> new_list;

  new_list.reserve(list.size());

  for (int i=0;i<list.size();i++) {
    T* casted_obj = dynamic_cast<T*>(list.at(i));

    // Test cast (casted_obj will be nullptr if the dynamic_cast fails)
    if (casted_obj != nullptr) {
      new_list.append(casted_obj);
    }
  }

  return new_list;
}

#endif // QOBJECTLISTCAST_H
