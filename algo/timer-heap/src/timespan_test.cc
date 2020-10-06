#include "timespan.hpp"
#include <assert.h>
int main() {
  const Timespan::TimeDiff base_sec = 99, base_usec = 199;
  {
    Timespan timespan;
    assert(timespan.ToMicroseconds() == 0);
  }
  {
    Timespan timespan(base_usec);
    assert(timespan.ToMicroseconds() == base_usec);
  }
  {
    Timespan timespan(base_sec, base_usec);
    Timespan::TimeDiff usec = base_sec * 1000 * 1000 + base_usec;
    assert(timespan.ToMicroseconds() == usec);
    assert(timespan.ToSeconds() == base_sec);
    assert(timespan.ToMilliseconds() == base_sec * 1000);

    Timespan timespan1 = timespan;
    assert(timespan1.ToMicroseconds() == timespan.ToMicroseconds());
  }
  {
    Timespan lhs(base_usec), rhs(base_sec, base_usec);
    assert(lhs != rhs);
    assert(lhs < rhs);
    assert(lhs <= rhs);
    assert(rhs > lhs);
    assert(rhs >= lhs);
    lhs += Timespan(base_sec, 0);
    assert(lhs == rhs);
  }
}