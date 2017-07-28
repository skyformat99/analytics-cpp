//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#ifndef SEGMENT_HTTP_WINHTTP_HPP_
#define SEGMENT_HTTP_WINHTTP_HPP_

#ifdef _WIN32
#include "http.hpp"

namespace segment {
namespace http {

    /// HttpWinHttp is an implementation of the HttpHandler API
    /// based on Windows 32 HTTP.  At present it only supports POST, and it
    /// does not actually populate the response fields or data, since
    /// they are not used by the framework.
    class HttpHandlerWinHttp : public HttpHandler {

    public:
        HttpHandlerWinHttp();
        virtual ~HttpHandlerWinHttp(){};
        virtual std::shared_ptr<HttpResponse> Handle(const HttpRequest& req);
    };

} // namespace http
} // namespace segment

#endif // SEGMENT_HTTP_WINHTTP_HPP_

#endif _WIN32
