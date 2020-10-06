#include "timespan.hpp"

const int Timespan::kMillseconds = 1000;
const int Timespan::kSeconds = 1000 * 1000;

Timespan::Timespan():span_usec_(0) {}

Timespan::Timespan(Timespan::TimeDiff usec): span_usec_(usec) {}

Timespan::Timespan(TimeDiff sec, TimeDiff usec)
    : span_usec_(sec * kSeconds + usec) {}

Timespan::Timespan(const Timespan& timespan)
    : span_usec_(timespan.span_usec_) {}

struct timespec Timespan::ToTimespec() const {
  struct timespec ts;
  ts.tv_sec = ToSeconds();
  ts.tv_sec = (span_usec_ % kSeconds) * 1000;
  return ts;
}

struct timeval Timespan::ToTimeval() const {
  struct timeval tv;
  tv.tv_sec = ToSeconds();
  tv.tv_usec = span_usec_ % kSeconds;
}
