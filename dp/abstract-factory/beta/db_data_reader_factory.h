#ifndef ABSTRACT_FACTORY_BETA_DB_DATA_READER_FACTORY_H
#define ABSTRACT_FACTORY_BETA_DB_DATA_READER_FACTORY_H

namespace beta {
class DBDataReaderBase;
class DBDataReaderFactory {
 public:
  virtual DBDataReaderBase* CreateDBDataReader() = 0;
  virtual ~DBDataReaderFactory() = default;
};
}  // namespace beta
#endif