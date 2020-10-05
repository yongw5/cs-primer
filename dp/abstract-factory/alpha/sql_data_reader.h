#ifndef ABSTRACT_FACTORY_ALPHA_SQL_DATA_READER_H
#define ABSTRACT_FACTORY_ALPHA_SQL_DATA_READER_H

#include <iostream>
namespace alpha {
class SQLDataReader {
 public:
  void Read() { std::cout << "SQLDataReader::Read\n"; }
};
}  // namespace alpha

#endif