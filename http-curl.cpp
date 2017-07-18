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

#include "http-curl.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include <curl/curl.h>

// To keep this simple, we include the entire implementation in this one
// C++ file.  Organizationally we'd probably have rather split this up,
// but having a single file to integrate makes it easier to integrate
// into other projects.

using json = nlohmann::json;

namespace segment {

// Internal curl exception error.  Most of these will be ENOMEM.
class curlExcept : public std::exception {
public:
    curlExcept(CURLcode rv)
    {
        this->code = rv;
    };
    const char* what() const throw()
    {
        return (curl_easy_strerror(this->code));
    }
    CURLcode code;
};

class curlReq {
public:
    curlReq()
    {
        this->headers = NULL;
        this->req = NULL;
    }
    ~curlReq()
    {
        if (this->headers != NULL) {
            curl_slist_free_all(this->headers);
        }
        if (this->req != NULL) {
            curl_easy_cleanup(this->req);
        }
    }

    void addHeader(std::string key, std::string value)
    {
        struct curl_slist* hdrs;
        std::string line;

        line = key + ": " + value;
        hdrs = curl_slist_append(this->headers, line.c_str());
        if (hdrs == NULL) {
            throw std::bad_alloc(); // cannot think of any other reason...
        }
        this->headers = hdrs;
    }

    void setBody(std::vector<char> body)
    {
        this->body = body;
    }

    static size_t writeCallback(char* ptr, size_t sz, size_t nmemb, void* udata)
    {
        curlReq* creq = (curlReq*)udata;
        char* ucdata = (char*)udata;
        size_t nbytes = sz * nmemb;
        for (int i = 0; i < nbytes; i++) {
            creq->respData.push_back(*ucdata);
            ucdata++;
        }
        return (nbytes);
    }

    void perform(std::string meth, std::string url)
    {
        CURL* req;
        CURLcode rv;
        long code;

        if (meth != "POST") {
            throw std::invalid_argument("Only POST supported");
        }

        if (this->req == NULL) {
            this->req = curl_easy_init();
        }
        if ((req = this->req) == NULL) {
            throw std::bad_alloc();
        }

#define setopt(k, v)                                      \
    if ((rv = curl_easy_setopt(req, k, v)) != CURLE_OK) { \
        throw curlExcept(rv);                             \
    }

        std::string body(this->body.begin(), this->body.end());

        // We only handle post.
        setopt(CURLOPT_HTTPHEADER, this->headers);
        setopt(CURLOPT_POST, 1);
        setopt(CURLOPT_URL, url.c_str());
        setopt(CURLOPT_POSTFIELDS, body.c_str());
        setopt(CURLOPT_POSTFIELDSIZE, body.length());
        setopt(CURLOPT_WRITEFUNCTION, this->writeCallback);
        setopt(CURLOPT_WRITEDATA, this);

        if ((rv = curl_easy_perform(req)) != CURLE_OK) {
            throw curlExcept(rv);
        }

#define getinfo(k, v)                                      \
    if ((rv = curl_easy_getinfo(req, k, v)) != CURLE_OK) { \
        throw curlExcept(rv);                              \
    }

        // get status
        getinfo(CURLINFO_RESPONSE_CODE, &code);
        this->respCode = (int)code;
        if (code == 0) {
            long errn;
            getinfo(CURLINFO_OS_ERRNO, &errn);
            this->respMessage = std::strerror((int)errn);
        }
        if (code > 299) {
            this->respMessage = "HTTP Error " + std::to_string(code);
        }
    }

    // This is probably not terribly idiomatic C++, but we put them
    // here to make things easy.  C++ class friends would be better,
    // but we're leaving this here in the implementation, so it should
    // be fine.  These are used by the HttpHandlerCurl class.

    std::vector<char> respData;
    std::string respMessage;
    int respCode;

private:
    struct curl_slist* headers;
    std::vector<char> body;
    CURL* req;
};

void HttpHandlerCurl::Handle(const HttpRequest& req, HttpResponse& resp)
{

    try {
        curlReq creq;

        for (auto const& item : req.Headers) {
            creq.addHeader(item.first, item.second);
        }

        creq.setBody(req.Body);
        creq.perform("POST", req.URL);
        resp.Code = creq.respCode;
        resp.Message = creq.respMessage;
        resp.Body = creq.respData;
        // Now start adding request details.
    } catch (std::exception e) {
        resp.Code = 0;
        resp.Message = e.what();
    }
}

} // namespace segment
