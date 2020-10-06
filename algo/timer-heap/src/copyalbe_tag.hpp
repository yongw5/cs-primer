#pragma once

class Copyable {
 protected:
  Copyable() = default;
  virtual ~Copyable() = default;
};

class NotCopyable {
 protected:
  NotCopyable() {}
  virtual ~NotCopyable() {}

 private:
  NotCopyable(const NotCopyable&) = delete;
  NotCopyable(NotCopyable&&) = delete;
  NotCopyable& operator=(const NotCopyable&) = delete;
  NotCopyable& operator=(NotCopyable&&) = delete;
};