//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include <exception>
#include <map>
#include <string>

namespace segment {

class HttpError : public std::exception {

public:
    HttpError(int);
    const char* what() const throw();
    int HttpCode() const;

private:
    char buf[128];
    int code;
};

class SocketError : public std::exception {
public:
    SocketError();
    SocketError(int);
    const char* what() const throw();

private:
    int os_errno;
};

class Response {
public:
    Response();
    ~Response();

    bool ok;
    int status;
    std::string data;
    static size_t writeCallback(void*, size_t, size_t, void*);
};

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

    std::string Serialize();
    std::string Type();

    std::string userId;
    std::string event;
    std::string groupId;
    std::string anonymousId;
    std::string previousId;
    std::map<std::string, std::string> properties;

private:
    EventType type;
};

class Analytics {
public:
    Analytics(std::string writeKey);
    Analytics(std::string writeKey, std::string host);
    ~Analytics();

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
    std::string userUage;

    void sendEvent(Event& e);
};

} // segment
