## BlockBuilder 和 FilterBlockBuilder、FilterBlockReader
BlockBuilder 将添加的 Key-Value 数据构建为 Block 数据，BlockBuilder 不能处理无序的 Key-Value 数据。每次调用 Add() 函数添加 Key-Value 数据，都要求 Key 大于所有已添加的 Key 值。Add() 实现如下：
```
void BlockBuilder::Add(const Slice& key, const Slice& value) {
  Slice last_key_piece(last_key_);
  assert(!finished_);
  assert(counter_ <= options_->block_restart_interval);
  assert(buffer_.empty()  // No values yet?
         || options_->comparator->Compare(key, last_key_piece) > 0); // Key 递增
  size_t shared = 0;
  if (counter_ < options_->block_restart_interval) {
    // See how much sharing to do with previous string
    const size_t min_length = std::min(last_key_piece.size(), key.size());
    while ((shared < min_length) && (last_key_piece[shared] == key[shared])) {
      shared++;
    }
  } else {
    // Restart compression
    restarts_.push_back(buffer_.size());
    counter_ = 0;
  }
  const size_t non_shared = key.size() - shared;

  // Add "<shared><non_shared><value_size>" to buffer_
  PutVarint32(&buffer_, shared);
  PutVarint32(&buffer_, non_shared);
  PutVarint32(&buffer_, value.size());

  // Add string delta to buffer_ followed by value
  buffer_.append(key.data() + shared, non_shared);
  buffer_.append(value.data(), value.size());

  // Update state
  last_key_.resize(shared);
  last_key_.append(key.data() + shared, non_shared);
  assert(Slice(last_key_) == key);
  counter_++;
}
```
在添加 Key-Value 数据完成后（CurrentSizeEstimate() 大于阈值，默认为 4KB），调用 Finish() 将 Block 数据返回。
```
Slice BlockBuilder::Finish() {
  // Append restart array
  for (size_t i = 0; i < restarts_.size(); i++) {
    PutFixed32(&buffer_, restarts_[i]);
  }
  PutFixed32(&buffer_, restarts_.size());
  finished_ = true;
  return Slice(buffer_);
}
```
任何可以调用 Reset()，清理创建的 Block 数据，开始下一个 Block 数据创建。
```
void BlockBuilder::Reset() {
  buffer_.clear();
  restarts_.clear();
  restarts_.push_back(0);  // First restart point is at offset 0
  counter_ = 0;
  finished_ = false;
  last_key_.clear();
}
```
FilterBlockBuilder 用于创建 FilterBlock 数据。filter(i) 保存了（在 .ldb 文件）偏移位置在 [i * base, (i + 1) * base) 中所有 Key 生成的 FilterBlock 数据。FilterBlockBuild 定义了 base 大小为 2KB：
```
static const size_t kFilterBaseLg = 11;
static const size_t kFilterBase = 1 << kFilterBaseLg;
```
FilterBlockBuilder 成员变量及其含义如下：

|数据成员|类型|含义|
|:-|:-|:-|
|policy_|const FilterPolicy*|Filter 实现方式|
|keys_|std::string|添加的 Key 数据|
|start_|std::vector<size_t>|每个 Key 的在 keys_ 中的偏移|
|result_|std::string|目前生成的 filter 数据|
|tmp_keys_|std::vector<Slice>|用于生成一个 filter 的 Key 数据|
|filter_offsets_|std::vector<uint32_t>|每个 filter 在 result_ 的偏移|

调用 FilterBlockBuilder 函数需要满足一定的顺序，匹配如下正则化表达式：
```
(StartBlock AddKey*)* Finish
```
即每开始一个 DataBlock，调用 StartBlock() 传入当前 DataBlock 的偏移位置。然后多次调用 AddKey() 将当前 DataBlock 的 Key 逐一添加到 FilterBlock，用于生成 filter。最后调用 Finish() 函数获取 filter 数据。

StartBlock() 设置 DataBlock 位置偏移，然后依次将偏移在 [i * base, (i + 1) * base) 的 DataBlock（不是 Key-Value 数据的偏移量） 中的 Key 生成 filter
```
void FilterBlockBuilder::StartBlock(uint64_t block_offset) {
  uint64_t filter_index = (block_offset / kFilterBase);
  assert(filter_index >= filter_offsets_.size());
  while (filter_index > filter_offsets_.size()) {
    GenerateFilter();
  }
}
```
GenerateFilter() 用于生成 filter，调用 policy_->CreateFilter() 具体实现。
```
void FilterBlockBuilder::GenerateFilter() {
  const size_t num_keys = start_.size();
  if (num_keys == 0) {
    // Fast path if there are no keys for this filter
    filter_offsets_.push_back(result_.size());
    return;
  }

  // Make list of keys from flattened key structure
  start_.push_back(keys_.size());  // Simplify length computation
  tmp_keys_.resize(num_keys);
  for (size_t i = 0; i < num_keys; i++) {
    const char* base = keys_.data() + start_[i];
    size_t length = start_[i + 1] - start_[i];
    tmp_keys_[i] = Slice(base, length);
  }

  // Generate filter for current set of keys and append to result_.
  filter_offsets_.push_back(result_.size());
  policy_->CreateFilter(&tmp_keys_[0], static_cast<int>(num_keys), &result_);

  tmp_keys_.clear();
  keys_.clear();
  start_.clear();
}
```
AddKey() 很简单，将 Key 添加到 keys_ 中
```
void FilterBlockBuilder::AddKey(const Slice& key) {
  Slice k = key;
  start_.push_back(keys_.size());
  keys_.append(k.data(), k.size());
}
```
Finish() 函数返回生成的 filter 数据。可以看到，在 filter 尾部，记录了每个 filter 偏移量，这些偏移量组成偏移数组。最后记录了偏移数组起始（偏移）位置和 kFilterBaseLg 变量。
```
Slice FilterBlockBuilder::Finish() {
  if (!start_.empty()) {
    GenerateFilter();
  }

  // Append array of per-filter offsets
  const uint32_t array_offset = result_.size();
  for (size_t i = 0; i < filter_offsets_.size(); i++) {
    PutFixed32(&result_, filter_offsets_[i]);
  }

  PutFixed32(&result_, array_offset);
  result_.push_back(kFilterBaseLg);  // Save encoding parameter in result
  return Slice(result_);
}
```
FilterBlockReader 主要用于快速检查某个 Key 值是否存在。其成员变量以及含义如下：

|数据成员|类型|含义|
|:-|:-|:-|
|policy_|const FilterPolicy*|Filter 实现方式|
|data_|const char*|filter 数据|
|offset_|const char*|偏移数组数据|
|num_|size_t|filter 数组元素个数|
|base_lg_|size_t|kFilterBaseLg 数值|

函数 KeyMayMatch() 用于返回偏移位置为 block_offset 的 DataBlock 是否含有某个 Key。如果该函数返回 true，表示可能存在；若返回 false，则 DataBlock 肯定不包含这个 Key。
```
bool FilterBlockReader::KeyMayMatch(uint64_t block_offset, const Slice& key) {
  uint64_t index = block_offset >> base_lg_;
  if (index < num_) {
    uint32_t start = DecodeFixed32(offset_ + index * 4);
    uint32_t limit = DecodeFixed32(offset_ + index * 4 + 4);
    if (start <= limit && limit <= static_cast<size_t>(offset_ - data_)) {
      Slice filter = Slice(data_ + start, limit - start);
      return policy_->KeyMayMatch(key, filter);
    } else if (start == limit) {
      // Empty filters do not match any keys
      return false;
    }
  }
  return true;  // Errors are treated as potential matches
}
```