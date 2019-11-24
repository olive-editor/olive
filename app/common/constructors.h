#ifndef CONSTRUCTORS_H
#define CONSTRUCTORS_H

/**
 * Copy/move deleters. Similar to Q_DISABLE_COPY_MOVE, et al. but those functions
 */

#define DISABLE_COPY(Class) \
    Class(const Class &) = delete;\
    Class &operator=(const Class &) = delete;

#define DISABLE_MOVE(Class) \
  Class(Class &&) = delete; \
  Class &operator=(Class &&) = delete;

#define DISABLE_COPY_MOVE(Class) \
  DISABLE_COPY(Class) \
  DISABLE_MOVE(Class)

#endif // CONSTRUCTORS_H
