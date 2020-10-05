## TableBuilder
TableBuilder 提供构造 .ldb 文件的接口，将添加的 Key-Value 数据构造成 DataBlock 或者 FilterDataBlock，然后将其输出到文件中永久保存。TableBuilder 用 rep_ 统一管理成员变量，主要如下：

|数据成员|类型|含义|
|:-|:-|:-|
|file|WritableFile*|可写 .ldb 文件|
|offset|uint64_t||
|data_block|BlockBuilder|构造 DataBlock|
|index_block|BlockBuilder|构造 IndexBlock|
|last_key|std::string|上一个插入的 Key|
|num_entries|int64_t|目前插入 Key-Value 的数据|
|closed|bool|是否调用 Finish() 或者 Abandon()|
|filter_block|FilterBlockBuilder*|构造 FilterBlock|
|pending_index_entry|bool|是否有 index 待处理|
|pending_handle|BlockHandle|构建一个索引对应 DataBlock 的大小及其偏移|
|compressed_output|std::string||

其 Add() 函数实现如下：
```
void TableBuilder::Add(const Slice& key, const Slice& value) {
  Rep* r = rep_;
  assert(!r->closed);
  if (!ok()) return;
  if (r->num_entries > 0) {
    assert(r->options.comparator->Compare(key, Slice(r->last_key)) > 0);
  }

  if (r->pending_index_entry) { // 为上一个 DataBlock 添加索引
    assert(r->data_block.empty());
    r->options.comparator->FindShortestSeparator(&r->last_key, key);
    std::string handle_encoding;
    r->pending_handle.EncodeTo(&handle_encoding);
    r->index_block.Add(r->last_key, Slice(handle_encoding));
    r->pending_index_entry = false;
  }

  if (r->filter_block != nullptr) {
    r->filter_block->AddKey(key);
  }

  r->last_key.assign(key.data(), key.size());
  r->num_entries++;
  r->data_block.Add(key, value);

  const size_t estimated_block_size = r->data_block.CurrentSizeEstimate();
  if (estimated_block_size >= r->options.block_size) {
    Flush();
  }
}
```
可以看到，Add() 函数要求每次添加的 Key-Value 数据的 Key 值都比已添加的 Key 值大。TableBuilder 会为每个 DataBlock 添加一个索引，该索引满足：（1）不小于当前 DataBlock 数据中所有 Key 值；（2）一定小于下一个 DataBlock 中所有 Key 值。而且，为了减少索引数据占用的空间，TableBuilder 在“看到”下一个 DataBlock 的第一个 Key-Value 数据时，才为上一个 DataBlock 数据添加一个索引。如此，可以选择一个满足要求且最短的一个字符串为索引。当 DataBlock 数据量超过阈值（默认 4KB），调用 Flush() 将其刷新到磁盘。函数 Flush() 定义如下：
```
void TableBuilder::Flush() {
  Rep* r = rep_;
  assert(!r->closed);
  if (!ok()) return;
  if (r->data_block.empty()) return;
  assert(!r->pending_index_entry);
  WriteBlock(&r->data_block, &r->pending_handle); // 将数据写入文件缓冲区
  if (ok()) {
    r->pending_index_entry = true;
    r->status = r->file->Flush();   // 刷新到磁盘
  }
  if (r->filter_block != nullptr) {
    r->filter_block->StartBlock(r->offset); // 下一个 DataBlock 偏移位置
  }
}
```
WriteBlock() 从 BlockBuilder 中提取 BlockContents 数据，然后根据压缩选项压缩数据。然后调用 WriteRawBlock() 函数将处理后的 BlockContents 数据写入到文件缓冲区。WriteRawBlock() 函数定义如下：
```
void TableBuilder::WriteRawBlock(const Slice& block_contents,
                                 CompressionType type, BlockHandle* handle) {
  Rep* r = rep_;
  handle->set_offset(r->offset);
  handle->set_size(block_contents.size());
  r->status = r->file->Append(block_contents);
  if (r->status.ok()) {
    char trailer[kBlockTrailerSize];
    trailer[0] = type;
    uint32_t crc = crc32c::Value(block_contents.data(), block_contents.size());
    crc = crc32c::Extend(crc, trailer, 1);  // Extend crc to cover block type
    EncodeFixed32(trailer + 1, crc32c::Mask(crc));
    r->status = r->file->Append(Slice(trailer, kBlockTrailerSize));
    if (r->status.ok()) {
      r->offset += block_contents.size() + kBlockTrailerSize;
    }
  }
}
```
可以看到，WriteRawBlock() 首先记录当前 DataBlock 数据的偏移及其大小，以供生成索引数据。需要注意的是，大小是 BlockContents 的大小，没有包含 5 字节的压缩类型和 CRC 校验和。然后将整个 DataBlock 数据写入到文件缓冲区。最后更新 offset 变量，也就是下一个 DataBlock 开始位置偏移。

在 Key-Value 数据添加完成，需要调用 Finish() 函数，将 filter 数据、索引数据以及尾部。其定义如下：
```
Status TableBuilder::Finish() {
  Rep* r = rep_;
  Flush(); // 处理 DataBlock
  assert(!r->closed);
  r->closed = true; // 添加结束，不能在调用 Add() 继续添加

  BlockHandle filter_block_handle, metaindex_block_handle, index_block_handle;

  // Write filter block
  if (ok() && r->filter_block != nullptr) { // 写入 filter 数据
    WriteRawBlock(r->filter_block->Finish(), kNoCompression,
                  &filter_block_handle);
  }

  // Write metaindex block
  if (ok()) { // 记录 filter 数据相关信息：filter 实现方式，大小以及位置偏移
    BlockBuilder meta_index_block(&r->options);
    if (r->filter_block != nullptr) {
      // Add mapping from "filter.Name" to location of filter data
      std::string key = "filter.";
      key.append(r->options.filter_policy->Name());
      std::string handle_encoding;
      filter_block_handle.EncodeTo(&handle_encoding);
      meta_index_block.Add(key, handle_encoding);
    }

    // TODO(postrelease): Add stats and other meta blocks
    WriteBlock(&meta_index_block, &metaindex_block_handle);
  }

  // Write index block
  if (ok()) { // 写入索引数据
    if (r->pending_index_entry) {
      r->options.comparator->FindShortSuccessor(&r->last_key);
      std::string handle_encoding;
      r->pending_handle.EncodeTo(&handle_encoding);
      r->index_block.Add(r->last_key, Slice(handle_encoding));
      r->pending_index_entry = false;
    }
    WriteBlock(&r->index_block, &index_block_handle);
  }

  // Write footer
  if (ok()) { // 写入尾部，固定 48 字节
    Footer footer;
    footer.set_metaindex_handle(metaindex_block_handle);
    footer.set_index_handle(index_block_handle);
    std::string footer_encoding;
    footer.EncodeTo(&footer_encoding);
    r->status = r->file->Append(footer_encoding);
    if (r->status.ok()) {
      r->offset += footer_encoding.size();
    }
  }
  return r->status;
}
```
TableBuilder 还提供 Abandon() 函数，可以终止 Table 的创建，当前已经添加的 Key-Value 数据，全部舍弃。