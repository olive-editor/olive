#include "nodecode.h"

NodeCode::NodeCode()
{

}

NodeCode::NodeCode(const QString &function_name, const QString &code) :
  function_name_(function_name),
  code_(code)
{
}

bool NodeCode::IsValid()
{
  return (!function_name_.isEmpty() && !code_.isEmpty());
}

const QString &NodeCode::function_name()
{
  return function_name_;
}

const QString &NodeCode::code()
{
  return code_;
}
