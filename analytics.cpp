//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include "analytics.h"
#include <curl/curl.h>
#include <iostream>
#include <string>

namespace segment {

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
    std::string r = "{\n";

    r += "  \"type\": \"" + this->Type() + "\"";
    if (event != "")
        r += ",\n  \"event\": \"" + event + "\"";
    if (userId != "")
        r += ",\n  \"userId\": \"" + userId + "\"";
    if (groupId != "")
        r += ",\n  \"groupId\": \"" + groupId + "\"";
    if (anonymousId != "")
        r += ",\n  \"anonymousId\": \"" + anonymousId + "\"";
    if (previousId != "")
        r += ",\n  \"previousId\": \"" + previousId + "\"";

    if (!properties.empty()) {
        std::map<std::string, std::string>::iterator itor;
        int i = 0;

        if (this->Type() == "identify") {
            r += ",\n  \"traits\": {";
        } else {
            r += ",\n  \"properties\": {";
        }
        for (itor = properties.begin(); itor != properties.end(); itor++, i++) {
            if (i > 0) {
                r += ",";
            }

            std::string key = itor->first;
            std::string value = itor->second;
            r += "\n    \"" + key + "\": \"" + value + "\"";
        }
        r += "\n  }";
    }

    return r + "\n}";
}

Response::Response()
{
    this->ok = false;
    this->status = -1;
}

Response::~Response()
{
    this->data.clear();
}

Analytics::Analytics(std::string writeKey)
    : writeKey(writeKey)
{
    host = "https://api.segment.io";
}

Analytics::Analytics(std::string writeKey, std::string host)
    : writeKey(writeKey)
    , host(host)
{
}

Analytics::~Analytics() {}

void Analytics::Track(std::string userId, std::string event)
{
    this->Track(userId, event, {});
}

void Analytics::Track(std::string userId, std::string event, std::map<std::string, std::string> properties)
{
    Event* e = new Event(EVENT_TYPE_TRACK);
    e->userId = userId;
    e->event = event;
    e->properties = properties; // TODO: probably need to copy this

    Response* res = this->sendEvent(e);

    delete res;
    delete e;
}

void Analytics::Identify(std::string userId)
{
    this->Identify(userId, {});
}

void Analytics::Identify(std::string userId, std::map<std::string, std::string> traits)
{
    Event* e = new Event(EVENT_TYPE_IDENTIFY);
    e->userId = userId;
    e->properties = traits;

    Response* res = this->sendEvent(e);

    delete res;
    delete e;
}

void Analytics::Page(std::string event, std::string userId)
{
    this->Page(event, userId, {});
}

void Analytics::Page(std::string event, std::string userId, std::map<std::string, std::string> properties)
{
    Event* e = new Event(EVENT_TYPE_PAGE);
    e->userId = userId;
    e->properties = properties;

    Response* res = this->sendEvent(e);

    delete res;
    delete e;
}

void Analytics::Screen(std::string event, std::string userId)
{
    this->Screen(event, userId, {});
}

void Analytics::Screen(std::string event, std::string userId, std::map<std::string, std::string> properties)
{
    Event* e = new Event(EVENT_TYPE_SCREEN);
    e->userId = userId;
    e->properties = properties;

    Response* res = this->sendEvent(e);

    delete res;
    delete e;
}

void Analytics::Alias(std::string previousId, std::string userId)
{
    Event* e = new Event(EVENT_TYPE_ALIAS);
    e->userId = userId;
    e->previousId = previousId;

    Response* res = this->sendEvent(e);

    delete res;
    delete e;
}

void Analytics::Group(std::string groupId)
{
    this->Group(groupId);
}

void Analytics::Group(std::string groupId, std::map<std::string, std::string> properties)
{
    Event* e = new Event(EVENT_TYPE_GROUP);
    e->groupId = groupId;
    e->properties = properties;

    Response* res = this->sendEvent(e);

    delete res;
    delete e;
}

Response* Analytics::sendEvent(Event* e)
{
    Response* res = new Response;
    CURL* req = NULL;
    CURLcode c = CURLE_OK;
    struct curl_slist* headers = NULL;
    long status = 0;

    if (!(req = curl_easy_init()))
        return res;

    std::string url = this->host + "/v1/" + e->Type();
    std::string data = e->Serialize();

    std::cout << "POST " << url << std::endl;
    std::cout << data << std::endl;

    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

#define option(x, y) curl_easy_setopt(req, x, y);
    option(CURLOPT_USERAGENT, "AnalyticsCPP/0.0");
    option(CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    option(CURLOPT_USERPWD, this->writeKey.c_str());
    c = option(CURLOPT_HTTPHEADER, headers);
    option(CURLOPT_URL, url.c_str());
    option(CURLOPT_POST, 1);
    option(CURLOPT_POSTFIELDS, data.c_str());
    option(CURLOPT_POSTFIELDSIZE, data.size());
    option(CURLOPT_WRITEFUNCTION, this->sendEventWriteCallback);
    option(CURLOPT_WRITEDATA, res);
    option(CURLOPT_HEADERDATA, res);
#undef option

    // make request
    c = curl_easy_perform(req);

    // set status
    curl_easy_getinfo(req, CURLINFO_RESPONSE_CODE, &status);
    res->ok = status == 200 && c != CURLE_ABORTED_BY_CALLBACK;
    res->status = static_cast<int>(status);

    std::cout << "STATUS CODE " << res->status << std::endl;
    std::cout << res->data << std::endl;

    // cleanup
    curl_easy_cleanup(req);
    curl_global_cleanup();
    curl_slist_free_all(headers);

    return res;
}

size_t Analytics::sendEventWriteCallback(void* data, size_t size, size_t nmemb, void* userdata)
{
    Response* res;
    res = reinterpret_cast<Response*>(userdata);
    res->data.append(reinterpret_cast<char*>(data), size * nmemb);
    return size * nmemb;
}
}
