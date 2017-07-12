//
// XXX: Insert BoilerPlate Here.
//
#include "analytics.h"

#define	CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace segment;

TEST_CASE("Submissions to Segment work", "[analytics]") {
	GIVEN("A valid writeKey") {
		auto writeKey = "LpSP8WJmW312Z0Yj1wluUcr76kd4F0xl";
		auto apiHost = "https://api.segment.io";
		Analytics *analytics = new Analytics(writeKey, apiHost);

		THEN("we can submit tracked events") {
			analytics->Track("userId", "Did Something", { { "foo", "bar" }, { "qux", "mux" }});
		}
		// XXX: Right now we have no way to validate the return.
		delete analytics;
	}
}
