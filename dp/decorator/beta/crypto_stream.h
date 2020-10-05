#ifndef DECORATOR_BETA_CRYPTO_STREAM_H
#define DECORATOR_BETA_CRYPTO_STREAM_H

#include "stream.h"

namespace beta {
class CryptoStream : public Stream {
 public:
  CryptoStream(Stream *stream) : stream_(stream) {}
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;

 private:
  Stream *stream_;
};
}  // namespace beta

#endif
