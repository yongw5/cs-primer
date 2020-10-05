## TableCache
LevelDB 每打开一个 .ldb 文件，就把其 MetaIndexBlock 数据和 DataIndexBlock 数据读入内存（详情见 Table::Open() 函数）。TableCache 用于缓存打开的 .ldb 文件，其底层是 SharedLRUCache 结构，其中 Key 值为 .ldb 文件序号，Value 值为 TableAndFile 类型的指针。TableAndFile 表示打开的 .ldb 文件以及读入的元数据，其定义如下：
```
struct TableAndFile {
  RandomAccessFile* file;
  Table* table;
};
```
TableCache 提供 Get() 接口供“客户”查询缓存的 Table。其定义如下：
```
Status TableCache::Get(const ReadOptions& options, uint64_t file_number,
                       uint64_t file_size, const Slice& k, void* arg,
                       void (*handle_result)(void*, const Slice&,
                                             const Slice&)) {
  Cache::Handle* handle = nullptr;
  Status s = FindTable(file_number, file_size, &handle);
  if (s.ok()) {
    Table* t = reinterpret_cast<TableAndFile*>(cache_->Value(handle))->table;
    s = t->InternalGet(options, k, arg, handle_result);
    cache_->Release(handle);
  }
  return s;
}
```
可以看到，首先调用 FindTable() 函数在 SharedLRUCache 中查询名字为 file_number，大小为 file_size 的 Table，然后调用 Table 的函数开始处理对应的 Key-Value 查询。FindTable() 定义如下：
```
Status TableCache::FindTable(uint64_t file_number, uint64_t file_size,
                             Cache::Handle** handle) {
  Status s;
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  Slice key(buf, sizeof(buf));
  *handle = cache_->Lookup(key);
  if (*handle == nullptr) {
    std::string fname = TableFileName(dbname_, file_number);
    RandomAccessFile* file = nullptr;
    Table* table = nullptr;
    s = env_->NewRandomAccessFile(fname, &file);
    if (!s.ok()) {
      std::string old_fname = SSTTableFileName(dbname_, file_number);
      if (env_->NewRandomAccessFile(old_fname, &file).ok()) {
        s = Status::OK();
      }
    }
    if (s.ok()) {
      s = Table::Open(options_, file, file_size, &table);
    }

    if (!s.ok()) {
      assert(table == nullptr);
      delete file;
      // We do not cache error results so that if the error is transient,
      // or somebody repairs the file, we recover automatically.
    } else {
      TableAndFile* tf = new TableAndFile;
      tf->file = file;
      tf->table = table;
      *handle = cache_->Insert(key, tf, 1, &DeleteEntry);
    }
  }
  return s;
}
```
FindTable() 函数首先查询 SharedLRUCache 中是否已经缓存该 .ldb 文件，如果已经存在，直接返回查询结果，否则，就打开该 .ldb 文件，然后将其插入到 SharedLRUCache 中，最后返回。

TableCache 也提供 Evict() 接口，用于从缓存中删除一个条目。其定义如下：
```
void TableCache::Evict(uint64_t file_number) {
  char buf[sizeof(file_number)];
  EncodeFixed64(buf, file_number);
  cache_->Erase(Slice(buf, sizeof(buf)));
}
```
另外，TableCache 提供 NewIterator() 接口，用于返回对应 Table 的迭代器。但是，如果 SharedLRUCache 中不存在对应的 Table，NewIterator() 函数直接返回错误迭代器，不会打开 .ldb 文件。

LevelDB 用户可以在 Options 里为 BlockCache 指定其他缓存实现方式（需要继承 Cache 基类，统一接口），如果没有指定，LevelDB 默认使用容量为 8MB 的 SharedLRUCache。 BlockCache 也是以 Key-Value 方式存储，Key 值为 cache_id 和 DataBlock 的 offset，Value 值为 DataBlock 的 BlockContents。如果查找 BlockCache 失败，则将 DataBlock 数据加载到内存，并插入到 BlockCache。