//
// XXX: Insert BoilerPlate Here.
//
#include "analytics.h"

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

using namespace segment;

TEST_CASE("Submissions to Segment work", "[analytics]")
{
    GIVEN("A valid writeKey")
    {
        auto writeKey = "LpSP8WJmW312Z0Yj1wluUcr76kd4F0xl";
        auto apiHost = "https://api.segment.io";
        Analytics* analytics = new Analytics(writeKey, apiHost);

        THEN("we can submit tracked events")
        {
            REQUIRE_NOTHROW(analytics->Track("userId", "Did Something",
                { { "foo", "bar" }, { "qux", "mux" } }));
        }
    }

    GIVEN("A bogus URL")
    {
        auto writeKey = "LpSP8WJmW312Z0Yj1wluUcr76kd4F0xl";
        auto apiHost = "https://api.segment.io/nobodyishome";
        Analytics* analytics = new Analytics(writeKey, apiHost);

        THEN("gives a 404 HttpException")
        {
            REQUIRE_THROWS_WITH(
                analytics->Track("userId", "Did Something",
                    { { "foo", "bar" }, { "qux", "mux" } }),
                Catch::Contains("404"));
        }
    }

#if 0
    // Apparently the remote server does not validate the writeKey!
    GIVEN("A bogus writeKey")
    {
        auto writeKey = "youShallNotPass!";
        auto apiHost = "https://api.segment.io";
        Analytics* analytics = new Analytics(writeKey, apiHost);

        THEN("we can submit tracked events")
        {
            analytics->Track("userId", "Did Something",
                { { "foo", "bar" }, { "qux", "mux" } });
        }
    }
#endif

    GIVEN("Localhost (random port)")
    {
        auto writeKey = "LpSP8WJmW312Z0Yj1wluUcr76kd4F0xl";
        auto apiHost = "https://localhost:50051";
        Analytics* analytics = new Analytics(writeKey, apiHost);

        THEN("Connection is refused")
        {
            REQUIRE_THROWS_WITH(
                analytics->Track("userId", "Did Something",
                    { { "foo", "bar" }, { "qux", "mux" } }),
                "Connection refused");
        }
    }
}
