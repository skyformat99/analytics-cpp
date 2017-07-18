//
// Copyright 2017 Segment Inc. <friends@segment.com>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#include "analytics.h"

// This is a dumb-do-nothing implementation of the HttpHandler.
// It is intended to stand-in for a default, if no better
// implementation is available.

namespace segment {

class HttpHandlerNone : public HttpHandler {
public:
    void Handle(const HttpRequest&, HttpResponse&)
    {
        resp.Code = 0;
        resp.Message = "Unimplemented.";
    };
};

}; // namespace segment