#ifndef ABSTRACT_FACTORY_BETA_SQL_DATA_READER_FACTORY_H
#define ABSTRACT_FACTORY_BETA_SQL_DATA_READER_FACTORY_H

#include "db_data_reader_factory.h"
#include "sql_data_reader.h"

namespace beta {
class DBDataReaderBase;
class SQLDataReaderFactory : public DBDataReaderFactory {
 public:
  DBDataReaderBase* CreateDBDataReader() override {
    return new SQLDataReader();
  }
  virtual ~SQLDataReaderFactory() = default;
};
}  // namespace beta
#endif