//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include <chrono>
#include <condition_variable>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "http.hpp"
#include "json.hpp"

#ifndef SEGMENT_ANALYTICS_HPP_
#define SEGMENT_ANALYTICS_HPP_

namespace segment {
namespace analytics {

    /// Traits are an arbitrary JSON object, containing the
    /// attributes for a user.  The value MUST be an object.
    /// Treat these like you would a dictionary.
    using Traits = nlohmann::json;

    /// Properties are arbitrary JSON objects.  Treat
    /// these like you would a dictionary.
    using Properties = nlohmann::json;

    /// Context is another JSON object type, with
    /// nested values.  The library populates some of these
    /// automatically.  Segments servers ignore fields that
    /// are not documented -- see the Context API spec for
    /// more details.
    using Context = nlohmann::json;

    typedef enum {
        EVENT_TYPE_IDENTIFY,
        EVENT_TYPE_TRACK,
        EVENT_TYPE_PAGE,
        EVENT_TYPE_SCREEN,
        EVENT_TYPE_GROUP,
        EVENT_TYPE_ALIAS
    } EventType;

    class Event {
    public:
        Event(EventType type);
        ~Event();

        std::string Serialize() const;
        std::string Type() const;

        std::string userId;
        std::string event;
        std::string groupId;
        std::string anonymousId;
        std::string previousId;
        std::map<std::string, std::string> properties;

    private:
        EventType type;
    };

    class Record {
    public:
        virtual std::string to_json() = 0;
        std::string recordType;
        std::string anonymousId;
        std::string userId;
        Context context;
        std::map<std::string, bool> integrations;
        // context
        // integrations
        // sentAt
        // timestmp
    };

    class Alias : public Record {
    public:
        std::string userId;
        std::string previousId;
    };

    class Identify : public Record {
    public:
        Traits traits;
    };

    class Track : public Record {
    public:
        std::string event;
        Properties properties;
    };

    /// Callback is the base class for analytics event callbacks.
    /// This should be subclassed, and an instance stored in the Analytics
    /// object, if necessary.  The default implementation does nothing.
    class Callback {
    public:
        virtual ~Callback(){};
        /// Success is called when the Event has successfully been uploaded
        /// to Segment.
        virtual void Success(std::shared_ptr<Event>) = 0;

        /// Failure is called when the Event was unable to be uploaded.
        /// @param ev [in] An incoming event.
        /// @param reason [in] Some reason why the failure occurrecd.
        virtual void Failure(std::shared_ptr<Event> ev, const std::string reason) = 0;
    };

    /// Analytics is the main object for accessing Segment's Analytics
    /// services; think of it as a handle or client object used to talk
    /// to Segment's servers.
    class Analytics {
    public:
        Analytics(std::string writeKey);
        Analytics(std::string writeKey, std::string host);
        ~Analytics();

        /// Flush flushes events to the server.  This just wakes up the
        /// asynchronous background thread, and starts it sending events
        /// to the server; it will process all events in the queue, unless
        /// an error occurs.
        void Flush();

        /// FlushWait flushes the queue, and waits for it to empty.  This
        /// should be called upon program exit; the destructor calls it
        /// automatically.  This can mean that it may take some time
        /// to destroy this object.
        void FlushWait();

        /// Scrub deletes all events that are queued for processing.
        /// If an immediate exit is required, call this first.  This
        /// method should be called with caution, as it generally will
        /// lead to lost events.
        void Scrub();

        /// Handler is the backend HTTP transport handler.  The constructor
        /// will initialize a default based upon compile time operations.
        std::shared_ptr<segment::http::HttpHandler> Handler;

        /// Callback is a callback object that wlll be called to inform
        /// the caller of success or failure of posting events to the
        /// service.
        std::shared_ptr<segment::analytics::Callback> Callback;

        /// MaxRetries represents the maximum number of retries to perform
        /// posting an event, before giving up.  The failure will not be
        /// reported until all retries are exhausted.
        int MaxRetries;

        /// FlushCount is the maximum number of messages to hold before flushing.
        /// Changing this value is not recommended.
        int FlushCount;

        /// FlushSize is the upper bound on batch size before we flush to the
        /// network.  This is actuallly a count in bytes; the entire marshalled
        /// object size is considered.  Note that this implemnentation uses
        /// compact encoding by default.
        size_t FlushSize;

        /// FlushInterval indicates how long we wait to collect messages to
        /// send them in a single batch.  We will upload a batch of messages
        /// whenever we have enough data to meet FlushCount, this much time
        /// has passed, or an explicit Flush or FlushWait is called (or this
        /// object is destroyed.)
        std::chrono::seconds FlushInterval;

        /// The amount of time to wait before retrying a post.
        std::chrono::seconds RetryInterval;

        void Track(std::string userId, std::string event, std::map<std::string, std::string> properties);
        void Track(std::string userId, std::string event);

        void Identify(std::string userId, std::map<std::string, std::string> traits);
        void Identify(std::string userId);

        void Page(std::string event, std::string userId, std::map<std::string, std::string> properties);
        void Page(std::string event, std::string userId);

        void Screen(std::string event, std::string userId, std::map<std::string, std::string> properties);
        void Screen(std::string event, std::string userId);

        void Alias(std::string previousId, std::string userId);

        void Group(std::string groupId, std::map<std::string, std::string> properties);
        void Group(std::string groupId);

    private:
        std::string writeKey;
        std::string host;

        std::mutex lock;
        std::condition_variable cv;
        std::thread thr;
        std::deque<std::shared_ptr<Event>> events;
        std::deque<std::shared_ptr<Event>> batch;
        std::chrono::system_clock::time_point flushTime;
        std::chrono::system_clock::time_point retryTime;
        std::chrono::system_clock::time_point wakeTime;

        bool needFlush;
        bool shutdown;

        void sendBatch();
        void queueEvent(std::shared_ptr<Event>);
        void processQueue();
        static void worker(Analytics*);
    };

} // namespace analytics
} // namespace segment

#endif // SEGMENT_ANALYTICS_HPP_