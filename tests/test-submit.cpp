//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include "analytics.h"

#include <condition_variable>
#include <mutex>
#include <thread>

#include <curl/curl.h> // So that we can clean up memory at the end.

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

using namespace segment;

class myTestCB : public Callback {
public:
    void Success(std::shared_ptr<Event> ev)
    {
        success++;
        Wake();
    }

    void Failure(std::shared_ptr<Event> ev, std::string reason)
    {
        last_reason = reason;
        fail++;
        Wake();
    }

    void Wake()
    {
        std::lock_guard<std::mutex> l(lk);
        done = true;
        cv.notify_all();
    }

    void Wait()
    {
        std::unique_lock<std::mutex> l(lk);

        while (!done) {
            cv.wait(l);
        }
    }

    bool done;
    std::mutex lk;
    std::condition_variable cv;
    int success;
    int fail;
    std::string last_reason;
};

TEST_CASE("Submissions to Segment work", "[analytics]")
{
    GIVEN("A valid writeKey")
    {
        auto writeKey = "LpSP8WJmW312Z0Yj1wluUcr76kd4F0xl";
        auto apiHost = "https://api.segment.io";

        THEN("we can submit tracked events")
        {
            auto cb = std::make_shared<myTestCB>();
            Analytics analytics(writeKey, apiHost);
            analytics.MaxRetries = 0;
            analytics.Callback = cb;

            analytics.Track("humptyDumpty", "Sat On A Wall", { { "crown", "broken" }, { "kingsHorses", "NoHelp" }, { "kingsMen", "NoHelp" } });
            cb->Wait();
            analytics.FlushWait();
            REQUIRE(cb->fail == 0);
        }
    }

    GIVEN("A bogus URL")
    {
        auto writeKey = "LpSP8WJmW312Z0Yj1wluUcr76kd4F0xl";
        auto apiHost = "https://api.segment.io/nobodyishome";

        THEN("gives a 404 HttpException")
        {
            auto cb = std::make_shared<myTestCB>();
            Analytics analytics(writeKey, apiHost);
            analytics.MaxRetries = 0;
            analytics.Callback = cb;

            analytics.Track("bogosURL", "Did Something", { { "foo", "bar" } });

            cb->Wait();
            analytics.FlushWait();
            REQUIRE_THAT(cb->last_reason, Catch::Contains("404"));
            REQUIRE(cb->fail == 1);
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
        THEN("Connection is refused")
        {
            auto cb = std::make_shared<myTestCB>();
            Analytics analytics(writeKey, apiHost);
            analytics.MaxRetries = 0;
            analytics.Callback = cb;

            analytics.Track("userId", "Did Something", { { "foo", "bar" }, { "qux", "mux" } });
            cb->Wait();
            analytics.FlushWait();
            REQUIRE(cb->fail == 1);
            REQUIRE(cb->last_reason == "Connection refused");
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
