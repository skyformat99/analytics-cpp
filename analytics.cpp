//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "analytics.hpp"
#include "date.hpp"
#include "json.hpp"

#ifdef SEGMENT_USE_CURL
#include "http-curl.hpp"
#elif defined(SEGMENT_USE_WININET)
#include "http-wininet.hpp"
#else
#include "http-none.hpp"
#endif

// To keep this simple, we include the entire implementation in this one
// C++ file.  Organizationally we'd probably have rather split this up,
// but having a single file to integrate makes it easier to integrate
// into other projects.

using json = nlohmann::json;

namespace segment {
namespace analytics {

    std::string TimeStamp()
    {
        auto now = std::chrono::system_clock::now();
        auto tmstamp = date::format("%FT%TZ",
            std::chrono::time_point_cast<std::chrono::milliseconds>(now));
        return tmstamp;
    }

    Event::Event(std::string type, std::string userId, std::string anonymousId, Object context, Object integrations)
    {
        this->object = json::object();
        object["timestamp"] = TimeStamp();
        object["type"] = type;
        if (userId != "") {
            object["userId"] = userId;
        }
        if (anonymousId != "") {
            object["anonymousId"] = anonymousId;
        }
        if (context.is_object()) {
            object["context"] = context;
        }
        if (integrations.is_object()) {
            object["integrations"] = integrations;
        }
    }

    void to_json(json& j, std::shared_ptr<Event> ev)
    {
        j = ev->object;
    }

    Analytics::Analytics(std::string writeKey)
        : writeKey(writeKey)
    {
        host = "https://api.segment.io";
#ifdef SEGMENT_USE_CURL
        Handler = std::make_shared<segment::http::HttpHandlerCurl>();
#elif defined(SEGMENT_USE_WININET)
        Handler = std::make_shared<segment::http::HttpHandlerWinHttp>();
#else
        Handler = std::make_shared<segment::http::HttpHandlerNone>();
#endif
        thr = std::thread(worker, this);
        MaxRetries = 5;
        RetryInterval = std::chrono::seconds(1);
        shutdown = false;
        FlushCount = 250;
        FlushSize = 500 * 1024;
        FlushInterval = std::chrono::seconds(10);
        needFlush = false;
        wakeTime = std::chrono::time_point<std::chrono::system_clock>::max();
    }

    Analytics::Analytics(std::string writeKey, std::string host)
        : writeKey(writeKey)
        , host(host)
    {
#ifdef SEGMENT_USE_CURL
        Handler = std::make_shared<segment::http::HttpHandlerCurl>();
#elif defined(SEGMENT_USE_WININET)
        Handler = std::make_shared<segment::http::HttpHandlerWinHttp>();
#else
        Handler = std::make_shared<segment::http::HttpHandlerNone>();
#endif

        thr = std::thread(worker, this);
        MaxRetries = 5;
        RetryInterval = std::chrono::seconds(1);
        shutdown = false;
        FlushCount = 250;
        FlushSize = 500 * 1024;
        FlushInterval = std::chrono::seconds(10);
        needFlush = false;
        wakeTime = std::chrono::time_point<std::chrono::system_clock>::max();
    }

    Analytics::~Analytics()
    {
        FlushWait();
        std::unique_lock<std::mutex> lk(this->lock);
        shutdown = true;
        flushCv.notify_one();
        lk.unlock();

        thr.join();
    }

    void Analytics::FlushWait()
    {
        std::unique_lock<std::mutex> lk(this->lock);

        // NB: If an event has been taken off the queue and is being
        // processed, then the lock will be held, preventing us from
        // executing this check.
        while (!events.empty()) {
            needFlush = true;
            flushCv.notify_one();
            emptyCv.wait(lk);
        }
    }

    void Analytics::Flush()
    {
        std::lock_guard<std::mutex> lk(this->lock);
        needFlush = true;
        flushCv.notify_one();
    }

    void Analytics::Scrub()
    {
        std::lock_guard<std::mutex> lk(this->lock);
        events.clear();
        emptyCv.notify_all();
        flushCv.notify_one();
    }

    void Analytics::Track(std::string userId, std::string event, Object properties)
    {
        Track(userId, "", event, properties, nullptr, nullptr);
    }

    void Analytics::Track(std::string userId, std::string anonymousId, std::string event, Object properties, Object context, Object integrations)
    {
        auto e = std::make_shared<Event>("track", userId, anonymousId, context, integrations);
        e->object["event"] = event;
        if (properties.is_object()) {
            e->object["properties"] = properties;
        }

        queueEvent(e);
    }

    void Analytics::Identify(std::string userId, Object traits)
    {
        Identify(userId, "", traits, nullptr, nullptr);
    }

    void Analytics::Identify(std::string userId, std::string anonymousId, Object traits, Object context, Object integrations)
    {
        auto e = std::make_shared<Event>("identify", userId, anonymousId, context, integrations);
        if (traits.is_object()) {
            e->object["traits"] = traits;
        }
        queueEvent(e);
    }

    void Analytics::Page(std::string name, std::string userId, Object properties)
    {
        Page(name, userId, "", properties, nullptr, nullptr);
    }

    void Analytics::Page(std::string name, std::string userId, std::string anonymousId, Object properties, Object context, Object integrations)
    {
        auto e = std::make_shared<Event>("page", userId, anonymousId, context, integrations);
        if (properties.is_object()) {
            e->object["properties"] = properties;
        }

        queueEvent(e);
    }
    void Analytics::Screen(std::string name, std::string userId, Object properties)
    {
        this->Screen(name, userId, "", properties, nullptr, nullptr);
    }

    void Analytics::Screen(std::string name, std::string userId, std::string anonymousId, Object properties, Object context, Object integrations)
    {
        auto e = std::make_shared<Event>("screen", userId, anonymousId, context, integrations);
        if (properties.is_object()) {
            e->object["properties"] = properties;
        }

        queueEvent(e);
    }

    void Analytics::Alias(std::string previousId, std::string userId)
    {
        Alias(previousId, userId, "", nullptr, nullptr);
    }

    void Analytics::Alias(std::string previousId, std::string userId, std::string anonymousId, Object context, Object integrations)
    {
        auto e = std::make_shared<Event>("alias", userId, anonymousId, context, integrations);
        e->object["previousId"] = previousId;

        queueEvent(e);
    }

    void Analytics::Group(std::string groupId, Object traits)
    {
        // The docs seem to claim that a userId or anonymousId must be set,
        // but the code I've seen suggests otherwise.
        Group(groupId, "", "", traits, nullptr, nullptr);
    }

    void Analytics::Group(std::string groupId, std::string userId, std::string anonymousId, Object traits, Object context, Object integrations)
    {
        auto e = std::make_shared<Event>("group", userId, anonymousId, context, integrations);
        e->object["groupId"] = groupId;

        queueEvent(e);
    }

    // This implementation of base64 is taken from StackOverflow:
    // https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
    // We use it here perform the base64 encode for user authentication with
    // Basic Auth, freeing the transport providers from dealing with this.
    // (Note that libcurl has this builtin though.)
    static std::string base64_encode(const std::string& in)
    {
        std::string out;

        int val = 0, valb = -6;
        for (unsigned char c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6)
            out.push_back("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4)
            out.push_back('=');
        return out;
    }

    void Analytics::sendBatch()
    {
        segment::http::HttpRequest req;
        // XXX add default context or integrations?

        auto tmstamp = TimeStamp();
        // Update the time on the elements of the batch.  We do this
        // on each new attempt, since we're trying to synchronize our clock
        // with the server's.
        for (auto ev : batch) {
            ev->object["sentAt"] = tmstamp;
        }

        json body;
        body["batch"] = batch;

        req.Method = "POST";
        req.URL = this->host + "/v1/batch";
        req.Headers["User-Agent"] = "AnalyticsCPP/0.1";
        // Note: libcurl could do this for us, but for other transports
        // we do it here -- keeping the transports as unaware as possible.
        req.Headers["Authorization"] = "Basic " + base64_encode(this->writeKey + ":");
        req.Headers["Content-Type"] = "application/json";
        req.Headers["Accept"] = "application/json";
        req.Body = body.dump();

        auto resp = this->Handler->Handle(req);
        if (resp->Code != 200) {
            throw(segment::http::HttpError(resp->Code));
        }
    }

    void Analytics::queueEvent(std::shared_ptr<Event> ev)
    {
        std::lock_guard<std::mutex> lk(lock);
        events.push_back(ev);
        if (events.size() == 1) {
            flushTime = std::chrono::system_clock::now() + FlushInterval;
            if (flushTime < wakeTime) {
                wakeTime = flushTime;
            }
        }
        flushCv.notify_one();
    }

    void Analytics::processQueue()
    {
        int fails = 0;
        bool ok;
        std::string reason;
        std::deque<std::shared_ptr<Event>> notifyq;
        std::unique_lock<std::mutex> lk(this->lock);

        for (;;) {
            // There is a little mystery here.  Without this sleep,
            // the Win32 system seems to get stuck; perhaps there is
            // a subtle bug in the handling of condition variables
            // or locks in the C++ runtime or threading libraries.
            // POSIX systems don't seem to need it, but 10 millis
            // is no sweat.
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            if (events.empty() && batch.empty()) {
                // Reset failure count so we start with a clean slate.
                // Otherwise we could have a failure hours earlier that
                // allows only one failed post hours later.
                fails = 0;
                wakeTime = std::chrono::time_point<std::chrono::system_clock>::max();

                // We might have a flusher waiting
                emptyCv.notify_all();

                // We only shut down if the queue was empty.  To force
                // a shutdown without draining, just clear the queue
                // independently.
                if (shutdown) {
                    return;
                }

                flushCv.wait(lk);
                continue;
            }

            // Gather up new items into the batch, assuming that the batch
            // is not already full.
            while ((!events.empty()) && (batch.size() < FlushCount)) {
                json j;

                // Try adding an event to the batch and checking the
                // serialization...  This is pretty ineffecient as we
                // serialize the objects multiple times, but it's easier
                // to understand.  Later look at saving the last size,
                // and only adding the size of the serialized event.

                auto ev = events.front();
                batch.push_back(ev);
                j["batch"] = batch;
                if (j.dump().size() >= FlushSize) {
                    batch.pop_back(); // remove what we just added.
                    needFlush = true;
                    break;
                }
                // We inclucded this event, so remove it from the queue
                // (it is already inthe batch.)
                events.pop_front();
            }

            // We hit the limit.
            if (batch.size() >= FlushCount) {
                needFlush = true;
            }

            auto now = std::chrono::system_clock::now();
            auto tmstamp = date::format("%FT%TZ",
                std::chrono::time_point_cast<std::chrono::milliseconds>(now));

            if ((!needFlush) && (now < wakeTime)) {
                flushCv.wait_until(lk, wakeTime);
                continue;
            }

            // We're trying to flush, so clear our "need".
            needFlush = false;

            try {
                sendBatch();
                ok = true;
                fails = false;
            } catch (std::exception& e) {
                if (fails < MaxRetries) {
                    // Something bad happened.  Let's wait a bit and
                    // try again later.  We return this even to the
                    // front.
                    fails++;
                    retryTime = now + RetryInterval;
                    if (retryTime < wakeTime) {
                        wakeTime = retryTime;
                    }
                    flushCv.wait_until(lk, wakeTime);
                    continue;
                }
                ok = false;
                reason = e.what();
                // We intentionally have chosen not to reset the failure
                // count.  Which means if we wind up failing to send one
                // event after maxtries, we only try each of the following
                // one time, until either the queue is empty or we have
                // a success.
            }
            auto cb = Callback;
            notifyq = batch;
            batch.clear();
            lk.unlock();

            while (!notifyq.empty()) {
                auto ev = notifyq.front();
                notifyq.pop_front();
                try {
                    if (cb != nullptr) {
                        if (ok) {
                            cb->Success(ev);
                        } else {
                            cb->Failure(ev, reason);
                        }
                    }
                } catch (std::exception&) {
                    // User supplied callback code failed.  There isn't really
                    // anything else we can do.  Muddle on.  This prevents a
                    // failure there from silently causing the processing thread
                    // to stop functioning.
                }
            }

            lk.lock();
        }
    }

    void Analytics::worker(Analytics* self)
    {
        self->processQueue();
    }

} // namespace analytics
} // namespace segment
