#ifndef ABSTRACT_FACTORY_RELEASE_DB_DATA_READER_BASE_H
#define ABSTRACT_FACTORY_RELEASE_DB_DATA_READER_BASE_H

#include <iostream>
namespace release {
class DBDataReaderBase {
 public:
  virtual void Read() = 0;
  virtual ~DBDataReaderBase() = default;
};
}  // namespace release

#endif