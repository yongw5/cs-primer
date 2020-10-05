#ifndef DECORATOR_ALPHA_CRYPTO_FILE_STREAM_H
#define DECORATOR_ALPHA_CRYPTO_FILE_STREAM_H

#include "file_stream.h"

namespace alpha {
class CryptoFileStream : public FileStream {
 public:
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace alpha

#endif
