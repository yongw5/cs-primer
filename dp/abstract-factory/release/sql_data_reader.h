#ifndef ABSTRACT_FACTORY_RELEASE_SQL_DATA_READER_H
#define ABSTRACT_FACTORY_RELEASE_SQL_DATA_READER_H

#include <iostream>
#include "db_data_reader_base.h"
namespace release {
class SQLDataReader : public DBDataReaderBase {
 public:
  void Read() override { std::cout << "SQLDataReader::Read\n"; }
};
}  // namespace release

#endif