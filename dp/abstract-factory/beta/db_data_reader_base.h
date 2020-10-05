#ifndef ABSTRACT_FACTORY_BETA_DB_DATA_READER_BASE_H
#define ABSTRACT_FACTORY_BETA_DB_DATA_READER_BASE_H

#include <iostream>
namespace beta {
class DBDataReaderBase {
 public:
  virtual void Read() = 0;
  virtual ~DBDataReaderBase() = default;
};
}  // namespace beta

#endif