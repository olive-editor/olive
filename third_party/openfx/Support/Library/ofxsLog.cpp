

/** @file This file contains the body of functions used for logging ofx problems etc...

The log file is written to using printf style functions, rather than via c++ iostreams.

*/

#include <cassert>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <string>

#include "ofxsLog.h"

namespace OFX {
  namespace Log {

    /** @brief log file */
    static FILE *gLogFP = 0;

    /// environment variable for the log file
#define kLogFileEnvVar "OFX_PLUGIN_LOGFILE"

    /** @brief the global logfile name */
    static std::string gLogFileName(getenv(kLogFileEnvVar) ? getenv(kLogFileEnvVar) : "ofxPluginLog.txt");

    /** @brief global indent level, not MP sane */
    static int gIndent = 0;

    /** @brief Sets the name of the log file. */
    void setFileName(const std::string &value)
    {
      gLogFileName = value;
    }

    /** @brief Opens the log file, returns whether this was sucessful or not. */
    bool open(void)
    {
#ifdef DEBUG
      if(!gLogFP) {
        gLogFP = fopen(gLogFileName.c_str(), "a");
        return gLogFP != 0;
      }
#endif
      return gLogFP != 0;
    }

    /** @brief Closes the log file. */
    void close(void)
    {
      if(gLogFP) {
        fclose(gLogFP);
      }
      gLogFP = 0;
    }

    /** @brief Indent it, not MP sane at the moment */
    void indent(void)
    {
      ++gIndent;
    }

    /** @brief Outdent it, not MP sane at the moment */
    void outdent(void)
    {
      --gIndent;
    }

    /** @brief do the indenting */
    static void doIndent(void)
    {
      if(open()) {
        for(int i = 0; i < gIndent; i++) {
          fputs("    ", gLogFP);
        }
      }
    }

    /** @brief Prints to the log file. */
    void print(const char *format, ...)
    {
      if(open()) {
        doIndent();
        va_list args;
        va_start(args, format);
        vfprintf(gLogFP, format, args);
        fputc('\n', gLogFP);
        fflush(gLogFP);
        va_end(args);
      }  
    }

    /** @brief Prints to the log file only if the condition is true and prepends a warning notice. */
    void warning(bool condition, const char *format, ...)
    {
      if(condition && open()) {
        doIndent();
        fputs("WARNING : ", gLogFP);

        va_list args;
        va_start(args, format);
        vfprintf(gLogFP, format, args);
        fputc('\n', gLogFP);
        va_end(args);

        fflush(gLogFP);
      }  
    }

    /** @brief Prints to the log file only if the condition is true and prepends an error notice. */
    void error(bool condition, const char *format, ...)
    {
      if(condition && open()) {
        doIndent();
        fputs("ERROR : ", gLogFP);

        va_list args;
        va_start(args, format);
        vfprintf(gLogFP, format, args);
        fputc('\n', gLogFP);
        va_end(args);

        fflush(gLogFP);
      }  
    }
  };
};
