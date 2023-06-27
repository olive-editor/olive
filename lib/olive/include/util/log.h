#ifndef LOG_H
#define LOG_H

#include <iostream>

namespace olive {

class Log
{
public:
  Log(const char *type)
  {
    std::cerr << "[" << type << "]";
  }

  ~Log()
  {
    std::cerr << std::endl;
  }

  template <typename T>
  Log &operator<<(const T &t)
  {
    std::cerr << " " << t;
    return *this;
  }

  static Log Debug()
  {
    return Log("DEBUG");
  }

  static Log Info()
  {
    return Log("INFO");
  }

  static Log Warning()
  {
    return Log("WARNING");
  }

  static Log Error()
  {
    return Log("ERROR");
  }

};

}

#endif // LOG_H
