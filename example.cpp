#include "analytics.h"

using namespace segment;

int main () {
  Analytics *analytics = new Analytics("xxx", "http://localhost:55005");
  analytics->Track("userId", "Did Something", { { "foo", "bar" }, { "qux", "mux" }});
  delete analytics;
  return 0;
}
