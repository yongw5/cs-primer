#ifndef DECORATOR_ALPHA_CRYPTO_MEMORY_STREAM_H
#define DECORATOR_ALPHA_CRYPTO_MEMORY_STREAM_H

#include "memory_stream.h"

namespace alpha {
class CryptoMemoryStream : public MemoryStream {
 public:
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace alpha

#endif
