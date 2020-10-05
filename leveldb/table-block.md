## Block 和 BlockHandle
Block 类管理 Block 数据，只提供用于查找 Block 数据内容的接口，不提供构造 Block 数据的接口，BlockBuilder 类负责构造 Block 数据。Block 类禁止拷贝和赋值，只有一个构造函数
```
Block::Block(const BlockContents& contents)
    : data_(contents.data.data()),
      size_(contents.data.size()),
      owned_(contents.heap_allocated) {
  if (size_ < sizeof(uint32_t)) {
    size_ = 0;  // Error marker
  } else {
    size_t max_restarts_allowed = (size_ - sizeof(uint32_t)) / sizeof(uint32_t);
    if (NumRestarts() > max_restarts_allowed) {
      // The size is too small for NumRestarts()
      size_ = 0;
    } else {
      restart_offset_ = size_ - (1 + NumRestarts()) * sizeof(uint32_t);
    }
  }
}
```
其中 data_ 指向 Block 数据，size_ 记录数据长度，restart_offset_ 记录 Block 重启点数组的偏移，owned_ 指示 Block 在析构时是否需要释放 data_ 管理的数据，即：
```
Block::~Block() {
  if (owned_) {
    delete[] data_;
  }
}
```
Block 类的 NewIterator() 函数用于返回一个迭代器，用于数据查找。其迭代器继承 Iterator，私有数据成员及其含义如下：

|数据成员|类型|含义|
|:-|:-|:-|
|comparator_|const Comparator* const|比较器|
|data_|const char* const|Block 数据|
|restarts_|uint32_t const|重启点数组位置偏移|
|num_restarts_|uint32_t const|重启点数量|
|current_|uint32_t|当前 Key-Value 偏移，不能超过重启点偏移|
|restart_index_|uint32_t|当前 Key-Value 所属的重启点|
|key_|std::string||
|value_|Slice||
|status_|Status||

重启点数组元素是固定32位整数，按照索引计算偏移便可读取该重启点数据，即该重启点的偏移
```
  uint32_t GetRestartPoint(uint32_t index) {
    assert(index < num_restarts_);
    return DecodeFixed32(data_ + restarts_ + index * sizeof(uint32_t));
  }
```
value_ 用于保存当前 Value 的数据，可以从 vaule_ 很容易获取下一条 Key-Value 数据的偏移
```
  inline uint32_t NextEntryOffset() const {
    return (value_.data() + value_.size()) - data_;
  }
```
解析下一条 Key-Value 数据的 Key 相对麻烦，需要解码前缀压缩和调整当前重启点索引值。
```
  bool ParseNextKey() {
    current_ = NextEntryOffset();
    const char* p = data_ + current_;
    const char* limit = data_ + restarts_;  // Restarts come right after data
    if (p >= limit) {
      // No more entries to return.  Mark as invalid.
      current_ = restarts_;
      restart_index_ = num_restarts_;
      return false;
    }

    // Decode next entry
    uint32_t shared, non_shared, value_length;
    p = DecodeEntry(p, limit, &shared, &non_shared, &value_length);
    if (p == nullptr || key_.size() < shared) {
      CorruptionError();
      return false;
    } else {
      key_.resize(shared);
      key_.append(p, non_shared);
      value_ = Slice(p + non_shared, value_length);
      while (restart_index_ + 1 < num_restarts_ &&
             GetRestartPoint(restart_index_ + 1) < current_) {
        ++restart_index_;
      }
      return true;
    }
  }
```
DecodeEntry() 解码前缀压缩，返回指向 UnsharedChars 的地址。
```
static inline const char* DecodeEntry(const char* p, const char* limit,
                                      uint32_t* shared, uint32_t* non_shared,
                                      uint32_t* value_length) {
  if (limit - p < 3) return nullptr;
  *shared = reinterpret_cast<const unsigned char*>(p)[0];
  *non_shared = reinterpret_cast<const unsigned char*>(p)[1];
  *value_length = reinterpret_cast<const unsigned char*>(p)[2];
  if ((*shared | *non_shared | *value_length) < 128) {
    // Fast path: all three values are encoded in one byte each
    p += 3;
  } else {
    if ((p = GetVarint32Ptr(p, limit, shared)) == nullptr) return nullptr;
    if ((p = GetVarint32Ptr(p, limit, non_shared)) == nullptr) return nullptr;
    if ((p = GetVarint32Ptr(p, limit, value_length)) == nullptr) return nullptr;
  }

  if (static_cast<uint32_t>(limit - p) < (*non_shared + *value_length)) {
    return nullptr;
  }
  return p;
}
```
Block 数据查找借助重启点使用二分查找，然后在局部使用线性查找
```
  virtual void Seek(const Slice& target) {
    // Binary search in restart array to find the last restart point
    // with a key < target
    uint32_t left = 0;
    uint32_t right = num_restarts_ - 1;
    while (left < right) {
      uint32_t mid = (left + right + 1) / 2;
      uint32_t region_offset = GetRestartPoint(mid);
      uint32_t shared, non_shared, value_length;
      const char* key_ptr =
          DecodeEntry(data_ + region_offset, data_ + restarts_, &shared,
                      &non_shared, &value_length);
      if (key_ptr == nullptr || (shared != 0)) {
        CorruptionError();
        return;
      }
      Slice mid_key(key_ptr, non_shared);
      if (Compare(mid_key, target) < 0) {
        // Key at "mid" is smaller than "target".  Therefore all
        // blocks before "mid" are uninteresting.
        left = mid;
      } else {
        // Key at "mid" is >= "target".  Therefore all blocks at or
        // after "mid" are uninteresting.
        right = mid - 1;
      }
    }

    // Linear search (within restart block) for first key >= target
    SeekToRestartPoint(left);
    while (true) {
      if (!ParseNextKey()) {
        return;
      }
      if (Compare(key_, target) >= 0) {
        return;
      }
    }
  }
```
Block 数据前向查找也借助重启点，然后从重启点向后线性移动，直到 NextEntryOffset() 满足条件
```
  virtual void Next() {
    assert(Valid());
    ParseNextKey();
  }

  virtual void Prev() {
    assert(Valid());

    // Scan backwards to a restart point before current_
    const uint32_t original = current_;
    while (GetRestartPoint(restart_index_) >= original) {
      if (restart_index_ == 0) {
        // No more entries
        current_ = restarts_;
        restart_index_ = num_restarts_;
        return;
      }
      restart_index_--;
    }

    SeekToRestartPoint(restart_index_);
    do {
      // Loop until end of current entry hits the start of original entry
    } while (ParseNextKey() && NextEntryOffset() < original);
  }
```
BlockHandle 用于标记 DataBlock 或者 MetaBlock 在 .ldb 文件中的位置，记录 Block 的大小（size_）和在 .log 文件中的偏移（offset_）。EncodeTo() 用于将 size_ 和 offset_ 输出。而 DecodeFrom() 函数相反，从外界获取 size_ 和 offset_。
```
void BlockHandle::EncodeTo(std::string* dst) const {
  // Sanity check that all fields have been set
  assert(offset_ != ~static_cast<uint64_t>(0));
  assert(size_ != ~static_cast<uint64_t>(0));
  PutVarint64(dst, offset_);
  PutVarint64(dst, size_);
}

Status BlockHandle::DecodeFrom(Slice* input) {
  if (GetVarint64(input, &offset_) && GetVarint64(input, &size_)) {
    return Status::OK();
  } else {
    return Status::Corruption("bad block handle");
  }
}
```