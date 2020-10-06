#pragma once
#include <stdint.h>
#include <time.h>
#include <utility>
#include "copyalbe_tag.hpp"
#include "timespan.hpp"

class Timestamp : private Copyable {
 public:
  typedef uint64_t TimePoint;
  static const int kResolution;

 public:
  Timestamp();
  explicit Timestamp(TimePoint usec);
  Timestamp(const Timestamp& timestamp);
  Timestamp& operator=(const Timestamp& timestamp);
  void Swap(Timestamp& other);
  Timestamp& AddTimespan(const Timespan& timespan);
  void Update();
  Timespan Elapsed() const;
  bool IsElapsed(const Timespan timespan) const;
  TimePoint EpochMicroseconds() const { return epoch_usec_; }
  time_t EpochTime() const {
    return static_cast<time_t>(epoch_usec_ / kResolution);
  }

 public:
  friend Timespan operator-(Timestamp lhs, Timestamp rhs);
  friend bool operator<(Timestamp lhs, Timestamp rhs);
  friend bool operator<=(Timestamp lhs, Timestamp rhs);
  friend bool operator>(Timestamp lhs, Timestamp rhs);
  friend bool operator>=(Timestamp lhs, Timestamp rhs);
  friend bool operator==(Timestamp lhs, Timestamp rhs);
  friend bool operator!=(Timestamp lhs, Timestamp rhs);

 private:
  TimePoint epoch_usec_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
  return lhs.epoch_usec_ < rhs.epoch_usec_;
}
inline bool operator<=(Timestamp lhs, Timestamp rhs) {
  return lhs.epoch_usec_ <= rhs.epoch_usec_;
}
inline bool operator>(Timestamp lhs, Timestamp rhs) { return !(lhs <= rhs); }
inline bool operator>=(Timestamp lhs, Timestamp rhs) { return !(lhs < rhs); }
inline bool operator==(Timestamp lhs, Timestamp rhs) {
  return lhs.epoch_usec_ == rhs.epoch_usec_;
}
inline bool operator!=(Timestamp lhs, Timestamp rhs) { return !(lhs == rhs); }
inline Timespan operator-(Timestamp lhs, Timestamp rhs) {
  return Timespan(
      static_cast<Timespan::TimeDiff>(lhs.epoch_usec_ - rhs.epoch_usec_));
}
Timespan TimespanFromNow(Timestamp when);