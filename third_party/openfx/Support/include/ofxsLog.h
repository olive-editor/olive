

#ifndef _ofxsLog_H_
#define _ofxsLog_H_
// Copyright OpenFX and contributors to the OpenFX project.
// SPDX-License-Identifier: BSD-3-Clause

/** @file This file contains OFX logging header code
*/

/** @brief The core 'OFX Support' namespace, used by plugin implementations. All code for these are defined in the common support libraries.
*/
namespace OFX {

  /** @brief this namespace wraps up logging functionality */
  namespace Log {
    /** @brief Indent it, not MP sane at the moment */
    void indent(void);

    /** @brief Outdent it, not MP sane at the moment */
    void outdent(void);

    /** @brief Sets the name of the log file. */
    void setFileName(const std::string &value);

    /** @brief Opens the log file, returns whether this was sucessful or not. */
    bool open(void);

    /** @brief Closes the log file. */
    void close(void);

    /** @brief Prints to the log file. */
    void print(const char *format, ...);

    /** @brief Prints to the log file only if the condition is true and prepends a warning notice. */
    void warning(bool condition, const char *format, ...);

    /** @brief Prints to the log file only if the condition is true and prepends an error notice. */
    void error(bool condition, const char *format, ...);
  };
};

#endif
