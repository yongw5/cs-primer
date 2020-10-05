#ifndef DECORATOR_ALPHA_STREAM_H
#define DECORATOR_ALPHA_STREAM_H

namespace alpha {
class Stream {
 public:
  virtual char Read(int number) = 0;
  virtual void Seek(int position) = 0;
  virtual void Write(char data) = 0;
  virtual ~Stream() {}
};
}  // namespace alpha

#endif
