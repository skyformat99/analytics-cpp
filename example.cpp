#include "analytics.h"

using namespace segment;

int main () {
  //Analytics *analytics = new Analytics("xxx", "http://localhost:55005");
  auto writeKey = "LpSP8WJmW312Z0Yj1wluUcr76kd4F0xl";
  auto apiHost = "https://api.segment.io";
  Analytics *analytics = new Analytics(writeKey, apiHost);
  analytics->Track("userId", "Did Something", { { "foo", "bar" }, { "qux", "mux" }});
  delete analytics;
  return 0;
}
