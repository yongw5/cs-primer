#ifndef DECORATOR_BETA_STREAM_H
#define DECORATOR_BETA_STREAM_H

namespace beta {
class Stream {
 public:
  virtual char Read(int number) = 0;
  virtual void Seek(int position) = 0;
  virtual void Write(char data) = 0;
  virtual ~Stream() {}
};
}  // namespace beta
#endif
