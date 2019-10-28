#ifndef NODECODE_H
#define NODECODE_H

#include <QString>

class NodeCode
{
public:
  NodeCode();
  NodeCode(const QString& function_name, const QString& code);

  bool IsValid();

  const QString& function_name();
  const QString& code();

private:
  QString function_name_;

  QString code_;
};

#endif // NODECODE_H
