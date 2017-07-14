//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include "analytics.h"

#include <curl/curl.h> // So that we can clean up memory at the end.

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

using namespace segment;

TEST_CASE("Submissions to Segment work", "[analytics]")
{
    GIVEN("A valid writeKey")
    {
        auto writeKey = "LpSP8WJmW312Z0Yj1wluUcr76kd4F0xl";
        auto apiHost = "https://api.segment.io";
        Analytics analytics(writeKey, apiHost);

        THEN("we can submit tracked events")
        {
            REQUIRE_NOTHROW(analytics.Track("humptyDumpty", "Sat On A Wall",
                { { "crown", "broken" }, { "kingsHorses", "NoHelp" }, { "kingsMen", "NoHelp" } }));
        }
    }

    GIVEN("A bogus URL")
    {
        auto writeKey = "LpSP8WJmW312Z0Yj1wluUcr76kd4F0xl";
        auto apiHost = "https://api.segment.io/nobodyishome";
        Analytics analytics(writeKey, apiHost);

        THEN("gives a 404 HttpException")
        {
            REQUIRE_THROWS_WITH(
                analytics.Track("userId", "Did Something",
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
        Analytics analytics(writeKey, apiHost);

        THEN("Connection is refused")
        {
            REQUIRE_THROWS_WITH(
                analytics.Track("userId", "Did Something",
                    { { "foo", "bar" }, { "qux", "mux" } }),
                "Connection refused");
        }
    }
}

int main(int argc, char* argv[])
{
    // Technically we don't have to init curl, but doing so ensures that
    // any global libcurl leaks are blamed on libcurl in valgrind runs.
    curl_global_init(CURL_GLOBAL_DEFAULT);

    int result = Catch::Session().run(argc, argv);

    curl_global_cleanup();
    return (result < 0xff ? result : 0xff);
}
