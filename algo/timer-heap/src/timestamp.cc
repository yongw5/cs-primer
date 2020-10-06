#include "timestamp.hpp"
#include <sys/time.h>

const int Timestamp::kResolution = 1000 * 1000;

Timestamp::Timestamp() { Update(); }
Timestamp::Timestamp(Timestamp::TimePoint usec) : epoch_usec_(usec) {}
Timestamp::Timestamp(const Timestamp& timestamp)
    : epoch_usec_(timestamp.epoch_usec_) {}
Timestamp& Timestamp::operator=(const Timestamp& timestamp) {
  Timestamp(timestamp).Swap(*this);
  return *this;
}
Timestamp& Timestamp::AddTimespan(const Timespan& timespan) {
  TimePoint old_value = epoch_usec_;
  epoch_usec_ += timespan.span_usec_;
  bool is_positive = timespan > Timespan(0);
  if ((is_positive && old_value > epoch_usec_) ||
      !is_positive && old_value < epoch_usec_) {  // overflow
    epoch_usec_ = 0;
  }
}
void Timestamp::Update() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  epoch_usec_ = static_cast<TimePoint>(tv.tv_sec * kResolution + tv.tv_usec);
}
Timespan Timestamp::Elapsed() const {
  Timestamp now;
  return now - *this;
}
bool Timestamp::IsElapsed(const Timespan timespan) const {
  Timestamp now;
  Timespan diff = now - *this;
  return diff >= timespan;
}

Timespan TimespanFromNow(Timestamp when) {
  Timestamp now;
  return when - now;
}