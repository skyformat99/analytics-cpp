#include "analytics.h"

using namespace segment;

int main () {
  Analytics *analytics = new Analytics("xxx", "http://localhost:55005");

  EventProperties props;
  props["foo"] = "bar";
  props["qux"] = "mux";
  analytics->Track("userId", "Did Something", props);

  delete analytics;
  return 0;
}
