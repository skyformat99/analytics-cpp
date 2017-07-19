//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include "analytics.h"

#include "json.hpp"

#include <cstring>
#include <iostream>
#include <string>

#include "http-curl.hpp"
#include "http-none.hpp"

// To keep this simple, we include the entire implementation in this one
// C++ file.  Organizationally we'd probably have rather split this up,
// but having a single file to integrate makes it easier to integrate
// into other projects.

using json = nlohmann::json;

namespace segment {

HttpError::HttpError(int code)
{
    this->code = code;
    this->msg = "HTTP Error " + std::to_string(code);
}

const char* HttpError::what() const throw()
{
    return (this->msg.c_str());
}

int HttpError::HttpCode() const
{
    return (this->code);
}

Event::Event(EventType type)
    : type(type)
{
}

Event::~Event()
{
    properties.clear();
}

std::string Event::Type()
{
    switch (type) {
    case EVENT_TYPE_IDENTIFY:
        return "identify";
        break;
    case EVENT_TYPE_SCREEN:
        return "screen";
        break;
    case EVENT_TYPE_PAGE:
        return "page";
        break;
    case EVENT_TYPE_ALIAS:
        return "alias";
        break;
    case EVENT_TYPE_TRACK:
        return "track";
        break;
    default:
        throw std::invalid_argument("Invalid event type");
    }
}

// super dirty JSON serialization... should clean this up
std::string Event::Serialize()
{
    json j;

    j["type"] = this->Type();
    if (event != "") {
        j["event"] = event;
    }
    if (userId != "") {
        j["userId"] = userId;
    }
    if (groupId != "") {
        j["groupId"] = groupId;
    }
    if (anonymousId != "") {
        j["anonymousId"] = anonymousId;
    }
    if (previousId != "") {
        j["previousId"] = previousId;
    }
    if (!properties.empty()) {
        if (this->Type() == "identify") {
            j["traits"] = properties;
        } else {
            j["properties"] = properties;
        }
    }
    return (j.dump());
}

Analytics::Analytics(std::string writeKey)
    : writeKey(writeKey)
{
    host = "https://api.segment.io";
    Handler = std::make_shared<HttpHandlerCurl>();
}

Analytics::Analytics(std::string writeKey, std::string host)
    : writeKey(writeKey)
    , host(host)
{
    Handler = std::make_shared<HttpHandlerCurl>();
}

Analytics::~Analytics() {}

// XXX: when we start batching events, we will want to to ensure that
// analytics kicks off a flush event.
void Analytics::Flush(bool wait)
{
    return;
}

void Analytics::Track(std::string userId, std::string event)
{
    this->Track(userId, event, {});
}

void Analytics::Track(std::string userId, std::string event, std::map<std::string, std::string> properties)
{
    Event e(EVENT_TYPE_TRACK);
    e.userId = userId;
    e.event = event;
    e.properties = properties; // TODO: probably need to copy this

    this->sendEvent(e);
}

void Analytics::Identify(std::string userId)
{
    this->Identify(userId, {});
}

void Analytics::Identify(std::string userId, std::map<std::string, std::string> traits)
{
    Event e(EVENT_TYPE_IDENTIFY);
    e.userId = userId;
    e.properties = traits;

    this->sendEvent(e);
}

void Analytics::Page(std::string event, std::string userId)
{
    this->Page(event, userId, {});
}

void Analytics::Page(std::string event, std::string userId, std::map<std::string, std::string> properties)
{
    Event e(EVENT_TYPE_PAGE);
    e.userId = userId;
    e.properties = properties;

    this->sendEvent(e);
}

void Analytics::Screen(std::string event, std::string userId)
{
    this->Screen(event, userId, {});
}

void Analytics::Screen(std::string event, std::string userId, std::map<std::string, std::string> properties)
{
    Event e(EVENT_TYPE_SCREEN);
    e.userId = userId;
    e.properties = properties;

    this->sendEvent(e);
}

void Analytics::Alias(std::string previousId, std::string userId)
{
    Event e(EVENT_TYPE_ALIAS);
    e.userId = userId;
    e.previousId = previousId;

    this->sendEvent(e);
}

void Analytics::Group(std::string groupId)
{
    this->Group(groupId);
}

void Analytics::Group(std::string groupId, std::map<std::string, std::string> properties)
{
    Event e(EVENT_TYPE_GROUP);
    e.groupId = groupId;
    e.properties = properties;

    this->sendEvent(e);
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

void Analytics::sendEvent(Event& e)
{
    HttpRequest req;

    req.Method = "POST";
    req.URL = this->host + "/v1/" + e.Type();
    req.Headers["User-Agent"] = "AnalyticsCPP/0.1";
    // Note: libcurl could do this for us, but for other transports
    // we do it here -- keeping the transports as unaware as possible.
    req.Headers["Authorization"] = "Basic " + base64_encode(this->writeKey);
    req.Headers["Content-Type"] = "application/json";
    req.Headers["Accept"] = "application/json";
    req.Body = e.Serialize();

    auto resp = this->Handler->Handle(req);
}
}
