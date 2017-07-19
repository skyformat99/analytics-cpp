//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include "analytics.h"

#ifndef HTTP_CURL_HPP
#define HTTP_CURL_HPP

namespace segment {

class HttpHandlerCurl : public HttpHandler {

public:
    HttpHandlerCurl(){};
    virtual ~HttpHandlerCurl(){};
    virtual std::shared_ptr<HttpResponse> Handle(const HttpRequest& req);
};

}; // namespace segment

#endif // HTTP_CURL_HPP