#ifndef DECORATOR_ALPHA_CRYPTO_NETWORK_STREAM_H
#define DECORATOR_ALPHA_CRYPTO_NETWORK_STREAM_H

#include "network_stream.h"

namespace alpha {
class CryptoNetowrkStream : public NetworkStream {
 public:
  char Read(int number) override;
  void Seek(int position) override;
  void Write(char data) override;
};
}  // namespace alpha

#endif
