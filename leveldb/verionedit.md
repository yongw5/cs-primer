## VersionEdit
VersionEdit 记录了两个 Version 之间的变更，也就是增量 Delta，满足 OldVersion + VersionEdit = LatestVersion。Compaction 过程中将会增加或者删除一些 .ldb 文件，这些操作被封装成 VersionEdit，当 Compaction 完成，将 VersionEdit 中的操作应用到当前 Version，便可得到最新状态的 Version。

函数 AddFile() 和 DeleteFile() 用于记录需要在当前 Version 上添加或者删除的文件。定义如下：
```
  void AddFile(int level, uint64_t file, uint64_t file_size,
               const InternalKey& smallest, const InternalKey& largest) {
    FileMetaData f;
    f.number = file;
    f.file_size = file_size;
    f.smallest = smallest;
    f.largest = largest;
    new_files_.push_back(std::make_pair(level, f));
  }

  // Delete the specified "file" from the specified "level".
  void DeleteFile(int level, uint64_t file) {
    deleted_files_.insert(std::make_pair(level, file));
  }
```
EncodeTo() 用于 VersionEdit 持久化，将本 VersionEdit 的所有信息持久化到磁盘的 MANIFEST 文件中。DecodeFrom() 表示相反的过程，表示从 MANIFEST 中恢复一个 VersionEdit。