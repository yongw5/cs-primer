#ifndef DECORATOR_RELEASE_STREAM_H
#define DECORATOR_RELEASE_STREAM_H

namespace release {
class Stream {
 public:
  virtual char Read(int number) = 0;
  virtual void Seek(int position) = 0;
  virtual void Write(char data) = 0;
  virtual ~Stream() {}
};
}  // namespace release
#endif
