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

EncodeTo() 定义如下：
```
void VersionEdit::EncodeTo(std::string* dst) const {
  if (has_comparator_) {
    PutVarint32(dst, kComparator);
    PutLengthPrefixedSlice(dst, comparator_); // comparator 名称
  }
  if (has_log_number_) {
    PutVarint32(dst, kLogNumber);
    PutVarint64(dst, log_number_); 
  }
  if (has_prev_log_number_) {
    PutVarint32(dst, kPrevLogNumber);
    PutVarint64(dst, prev_log_number_);
  }
  if (has_next_file_number_) {
    PutVarint32(dst, kNextFileNumber);
    PutVarint64(dst, next_file_number_);
  }
  if (has_last_sequence_) {
    PutVarint32(dst, kLastSequence);
    PutVarint64(dst, last_sequence_);
  }
  // 每层下次合并时的起始 Key
  for (size_t i = 0; i < compact_pointers_.size(); i++) {
    PutVarint32(dst, kCompactPointer);
    PutVarint32(dst, compact_pointers_[i].first);  // level
    PutLengthPrefixedSlice(dst, compact_pointers_[i].second.Encode());
  }
  // 已经删除文件 (level, fileno)
  for (const auto& deleted_file_kvp : deleted_files_) {
    PutVarint32(dst, kDeletedFile);
    PutVarint32(dst, deleted_file_kvp.first);   // level
    PutVarint64(dst, deleted_file_kvp.second);  // file number
  }
  // 新增文件 (level, FileMetadata)
  for (size_t i = 0; i < new_files_.size(); i++) {
    const FileMetaData& f = new_files_[i].second;
    PutVarint32(dst, kNewFile);
    PutVarint32(dst, new_files_[i].first);  // level
    PutVarint64(dst, f.number);
    PutVarint64(dst, f.file_size); // 文件大小
    PutLengthPrefixedSlice(dst, f.smallest.Encode()); // 最小key
    PutLengthPrefixedSlice(dst, f.largest.Encode()); // 最大key
  }
}
```
Tag 定义如下：
```
// Tag numbers for serialized VersionEdit.  These numbers are written to
// disk and should not be changed.
enum Tag {
  kComparator = 1,
  kLogNumber = 2,
  kNextFileNumber = 3,
  kLastSequence = 4,
  kCompactPointer = 5,
  kDeletedFile = 6,
  kNewFile = 7,
  // 8 was used for large value refs
  kPrevLogNumber = 9
};
```