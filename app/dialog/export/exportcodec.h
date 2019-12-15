#ifndef EXPORTCODEC_H
#define EXPORTCODEC_H

#include <QString>

class ExportCodec {
public:
  enum Flag {
    kNone,
    kStillImage
  };

  ExportCodec(const QString& name, const QString& id, Flag flags = kNone) :
    name_(name),
    id_(id),
    flags_(flags)
  {
  }

  const QString& name() const {return name_;}
  const QString& id() const {return id_;}
  const Flag& flags() const {return flags_;}

private:
  QString name_;
  QString id_;
  Flag flags_;

};

#endif // EXPORTCODEC_H
