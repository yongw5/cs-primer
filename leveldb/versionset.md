## VersionSet
VersionSet 管理 LevelDB 存在的 Version，多个 Version 按照双向链表连接，并且记录了当前最新的 Version（成员变量 current_ 指向）。除另外，VersionSet 还记录了文件序号相关信息，不如 .log、.ldb 以及 MANIFEST 文件当前的序号，并且 sequence 也是 VersionSet 管理。

LogAndApply() 函数将 VersionEdit 持久化到磁盘并将其应用于当前版本以形成一个新 Version。定义如下：
```
Status VersionSet::LogAndApply(VersionEdit* edit, port::Mutex* mu) {
  if (edit->has_log_number_) {
    assert(edit->log_number_ >= log_number_);
    assert(edit->log_number_ < next_file_number_);
  } else {
    edit->SetLogNumber(log_number_);
  }

  if (!edit->has_prev_log_number_) {
    edit->SetPrevLogNumber(prev_log_number_);
  }

  edit->SetNextFile(next_file_number_);
  edit->SetLastSequence(last_sequence_);

  Version* v = new Version(this);
  {
    Builder builder(this, current_);
    builder.Apply(edit);
    builder.SaveTo(v);  // 生成新的 Version
  }
  Finalize(v);  // 计算下一次合并的最佳 Level

  // VersionEdit 持久化到磁盘。首次需要讲 LevelDB 最初的 Version 写入一个
  // 快照到磁盘，也就是每个 Level 的文件。此后只持久化 VerionEdit 内容。
  std::string new_manifest_file;
  Status s;
  if (descriptor_log_ == nullptr) { // 首次
    // No reason to unlock *mu here since we only hit this path in the
    // first call to LogAndApply (when opening the database).
    assert(descriptor_file_ == nullptr);
    new_manifest_file = DescriptorFileName(dbname_, manifest_file_number_);
    edit->SetNextFile(next_file_number_);
    s = env_->NewWritableFile(new_manifest_file, &descriptor_file_);
    if (s.ok()) {
      descriptor_log_ = new log::Writer(descriptor_file_);
      s = WriteSnapshot(descriptor_log_); // 持久化一个快照
    }
  }

  // Unlock during expensive MANIFEST log write
  {
    mu->Unlock();

    // Write new record to MANIFEST log
    if (s.ok()) {
      std::string record;
      edit->EncodeTo(&record);
      s = descriptor_log_->AddRecord(record);
      if (s.ok()) {
        s = descriptor_file_->Sync();
      }
      if (!s.ok()) {
        Log(options_->info_log, "MANIFEST write: %s\n", s.ToString().c_str());
      }
    }

    // If we just created a new descriptor file, install it by writing a
    // new CURRENT file that points to it.
    if (s.ok() && !new_manifest_file.empty()) {
      s = SetCurrentFile(env_, dbname_, manifest_file_number_);
    }

    mu->Lock();
  }

  // Install the new version
  if (s.ok()) {
    AppendVersion(v);
    log_number_ = edit->log_number_;
    prev_log_number_ = edit->prev_log_number_;
  } else {
    delete v;
    if (!new_manifest_file.empty()) {
      delete descriptor_log_;
      delete descriptor_file_;
      descriptor_log_ = nullptr;
      descriptor_file_ = nullptr;
      env_->DeleteFile(new_manifest_file);
    }
  }

  return s;
}
```
Finalize() 函数用于计算新 Version 需要合并的 Level 以及合并的紧迫程度，定义如下：
```
void VersionSet::Finalize(Version* v) {
  // Precomputed best level for next compaction
  int best_level = -1;
  double best_score = -1;

  for (int level = 0; level < config::kNumLevels - 1; level++) {
    double score;
    if (level == 0) { // kL0_CompactionTrigger 为 4
      score = v->files_[level].size() /
              static_cast<double>(config::kL0_CompactionTrigger);
    } else {
      // Compute the ratio of current size to size limit.
      const uint64_t level_bytes = TotalFileSize(v->files_[level]);
      score =
          static_cast<double>(level_bytes) / MaxBytesForLevel(options_, level);
    }

    if (score > best_score) { // 分数越高，越紧迫
      best_level = level;
      best_score = score;
    }
  }

  v->compaction_level_ = best_level;
  v->compaction_score_ = best_score;
}
```
compaction_score_ 计算跟一个 Level 的文件数量或者数据量（字节数）有关。对于 Level-0，利用文件数量而不是字节数计算 score，理由如下：

1. 对于较大的写缓冲区，最好不要进行太多的 Level-0 合并；
2. Level-0 的文件每次读取时都会合并，因此 LevelDB 希望在单个文件大小较小（可能写缓冲较小，较高的数据压缩率，或者太多的重写或者删除操作）时避免 Level-0 有过多的文件。

对于其他 Level，根据字节数计算 score。函数 MaxBytesForLevel() 计算每层最大字节数，定义如下：
```
static double MaxBytesForLevel(const Options* options, int level) {
  // Level-0 数值未使用
  double result = 10. * 1048576.0;
  while (level > 1) {
    result *= 10;
    level--;
  }
  return result;
}
```
可以看到，Level-1 的最大字节数为 10MB，下一个 Level 是上一个 Level 字节数的 10 倍。

PickCompaction() 函数负责构造一次合并所需要的信息。PickCompaction() 会区分两种触发条件，一种是容量触发，另一种是查找（seek）触发，其中容量触发具有较高的优先级。PickCompaction() 定义如下：
```
Compaction* VersionSet::PickCompaction() {
  Compaction* c;
  int level;

  // We prefer compactions triggered by too much data in a level over
  // the compactions triggered by seeks.
  const bool size_compaction = (current_->compaction_score_ >= 1); // 容量触发
  const bool seek_compaction = (current_->file_to_compact_ != nullptr); // 查找触发
  if (size_compaction) { // 容量触发优先
    level = current_->compaction_level_;
    assert(level >= 0);
    assert(level + 1 < config::kNumLevels);
    c = new Compaction(options_, level);

    // Pick the first file that comes after compact_pointer_[level]
    for (size_t i = 0; i < current_->files_[level].size(); i++) {
      FileMetaData* f = current_->files_[level][i];
      if (compact_pointer_[level].empty() ||
          icmp_.Compare(f->largest.Encode(), compact_pointer_[level]) > 0) {
        c->inputs_[0].push_back(f); // 需要合并的文件
        break;
      }
    }
    if (c->inputs_[0].empty()) {
      // Wrap-around to the beginning of the key space
      c->inputs_[0].push_back(current_->files_[level][0]);
    }
  } else if (seek_compaction) { // 查找触发
    level = current_->file_to_compact_level_;
    c = new Compaction(options_, level);
    c->inputs_[0].push_back(current_->file_to_compact_);
  } else {
    return nullptr;
  }

  c->input_version_ = current_;
  c->input_version_->Ref();

  // Files in level 0 may overlap each other, so pick up all overlapping ones
  if (level == 0) { // Level-0 文件有重合，需要将所有的文件都包含进来
    InternalKey smallest, largest;
    GetRange(c->inputs_[0], &smallest, &largest);
    // Note that the next call will discard the file we placed in
    // c->inputs_[0] earlier and replace it with an overlapping set
    // which will include the picked file.
    current_->GetOverlappingInputs(0, &smallest, &largest, &c->inputs_[0]);
    assert(!c->inputs_[0].empty());
  }

  SetupOtherInputs(c);

  return c;
}
```
SetupOtherInputs() 函数用于计算 Level + 1 和 Level + 2 上需要参与合并的文件，记录到 c->inputs_[1]。SetupOtherInputs() 定义如下：
```
void VersionSet::SetupOtherInputs(Compaction* c) {
  const int level = c->level();
  InternalKey smallest, largest;

  AddBoundaryInputs(icmp_, current_->files_[level], &c->inputs_[0]);
  GetRange(c->inputs_[0], &smallest, &largest);

  current_->GetOverlappingInputs(level + 1, &smallest, &largest,
                                 &c->inputs_[1]);

  // Get entire range covered by compaction
  InternalKey all_start, all_limit;
  GetRange2(c->inputs_[0], c->inputs_[1], &all_start, &all_limit);

  // See if we can grow the number of inputs in "level" without
  // changing the number of "level+1" files we pick up.
  if (!c->inputs_[1].empty()) {
    std::vector<FileMetaData*> expanded0;
    current_->GetOverlappingInputs(level, &all_start, &all_limit, &expanded0);
    AddBoundaryInputs(icmp_, current_->files_[level], &expanded0);
    const int64_t inputs0_size = TotalFileSize(c->inputs_[0]);
    const int64_t inputs1_size = TotalFileSize(c->inputs_[1]);
    const int64_t expanded0_size = TotalFileSize(expanded0);
    if (expanded0.size() > c->inputs_[0].size() &&
        inputs1_size + expanded0_size <
            ExpandedCompactionByteSizeLimit(options_)) {
      InternalKey new_start, new_limit;
      GetRange(expanded0, &new_start, &new_limit);
      std::vector<FileMetaData*> expanded1;
      current_->GetOverlappingInputs(level + 1, &new_start, &new_limit,
                                     &expanded1);
      if (expanded1.size() == c->inputs_[1].size()) {
        Log(options_->info_log,
            "Expanding@%d %d+%d (%ld+%ld bytes) to %d+%d (%ld+%ld bytes)\n",
            level, int(c->inputs_[0].size()), int(c->inputs_[1].size()),
            long(inputs0_size), long(inputs1_size), int(expanded0.size()),
            int(expanded1.size()), long(expanded0_size), long(inputs1_size));
        smallest = new_start;
        largest = new_limit;
        c->inputs_[0] = expanded0;
        c->inputs_[1] = expanded1;
        GetRange2(c->inputs_[0], c->inputs_[1], &all_start, &all_limit);
      }
    }
  }

  // Compute the set of grandparent files that overlap this compaction
  // (parent == level+1; grandparent == level+2)
  if (level + 2 < config::kNumLevels) {
    current_->GetOverlappingInputs(level + 2, &all_start, &all_limit,
                                   &c->grandparents_);
  }

  // Update the place where we will do the next compaction for this level.
  // We update this immediately instead of waiting for the VersionEdit
  // to be applied so that if the compaction fails, we will try a different
  // key range next time.
  compact_pointer_[level] = largest.Encode().ToString();
  c->edit_.SetCompactPointer(level, largest);
}
```
首先调用 AddBoundaryInputs() 处理 c->inputs_[0]，对 c->inputs_[0] 作精细化处理。AddBoundaryInputs() 首先从 c->inputs_[0] 选取 Key 的最值 largest_key，然后在 level_files 查找满足如下条件的文件 b2=(low2, upper2)：

1. InternalKey(low2) > InternalKey(largest)
2. user_key(low2) == user_key(largest_key)

然后将其 b2 添加到 c->inputs_[0] 等待合并，循环进行直到找不到满足条件的文件。这样做原因如下：如果在某个 Level 上存在 b1=(l1, u1) 和 b2=(l2, u2) 两个文件，并且满足 user_key(u1) = user_key(l2) 和 InternalKey(l2) > InternalKey(u1)，如果仅仅将 b1 合并到 Level + 1 层，会造成数据不一致。因为查询时会在较低 Level 的文件 b2 中找到 l2，但是 l2 比 u1 “旧”，不是最新数据（InternalKeyComparator 在比较 InternalKey 时，按照 user_key 递增，sequence 递减的优先级比较）。在处理完 c->inputs_[0] 后，将 Level + 1 上与 c->inputs_[0] 重合的所有文件添加到 c->inputs_[1] 等待合并。

if (!c->inputs_[1].empty()) {...} 部分尝试增加 c->inputs_[0] 参与合并的文件，这里需要满足如下：

1. 增加 c->inputs_[0] 参与合并的文件不能增加 c->inputs_[1] 参与合并的文件
2. 总的文件（c->inputs_[0] 扩充后文件和 c->inputs_[1] 的文件）大小小于 ExpandedCompactionByteSizeLimit()

此时 Level 和 Level + 1 层上参与合并的文件已经确定，最后为了避免这些文件合并到 Level + 1 层后，跟 Level + 2 层有重叠的文件太多，届时合并 Level + 1 和 Level + 2 层压力太大，因此还需要记录下 Level + 2 层的文件，后续合并时提前结束。
