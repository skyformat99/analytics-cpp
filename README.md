# analytics-cpp

[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/segmentio/analytics-cpp/blob/master/LICENSE)
[![Linux Status](https://img.shields.io/travis/segementio/analytics-cpp/master.svg?label=linux)](https://travis-ci.org/segmentio/analytics-cpp)
[![Windows Status](https://img.shields.io/appveyor/ci/segmentio/analytics-cpp/master.svg?label=windows)](https://ci.appveyor.com/project/segmentio/analytics-cpp)
 [![Test Coverage](https://coveralls.io/repos/segmentio/analytics-cpp/badge.svg?branch=master)](https://coveralls.io/r/segmentio/analytics-cpp)

Build status on Garrett's Branch:
---------------------------------

[![Linux Status](https://img.shields.io/travis/segementio/analytics-cpp/garrett.svg?label=linux)](https://travis-ci.org/segmentio/analytics-cpp)
[![Windows Status](https://img.shields.io/appveyor/ci/gdamore/analytics-cpp/garrett.svg?label=windows)](https://ci.appveyor.com/project/gdamore/analytics-cpp)
 [![Test Coverage](https://coveralls.io/repos/gdamore/analytics-cpp/badge.svg?branch=garrett)](https://coveralls.io/r/gdamore/analytics-cpp)

A quick & dirty C++ implementation for sending analytics events to Segment.
This requires C++11, which means GCC 4.9 or newer; recent clang and MSVC 2015
are known to work well.

This library is in its early-POC phase and its API/ABI will certainly change. **This is not production-ready software.**

## Building

You will need CMake to build this.  Standard CMake recipes work, and a CTest
based test is included.

```
   $ mkdir build
   $ cd build
   $ cmake ..
   $ cmake --build .
```

On Windows, the default is to use the WinInet library for transport, which
is a standard system component supplied by Microsoft.

On other systems (Mac, Linux) you need to have libcurl installed.  Modern macOS
has a suitable version already installed.  Note that versions of libcurl built
against openssl do not work well with valgrind.  The GnuTLS version works well --
on Ubuntu do `apt-get install libcurl4-gnutls-dev` for the goodness.

You can elide these, and provide your own transport.  To eliminate the stock and
use a stub instead, use -DNO_DEFAULT_HTTP=ON on the CMake configure line.

To keep the codebase small/simple to work with, we rely on `libcurl` for making our HTTP requests. Install `libcurl` with `apt-get install -qq libcurl4-gnutls-dev`, then do:

There is an example program you can see for how to use it.  The API is documented
in the analytics.hpp header file.  (If you need to override the transport, you
will also need to look in the http.hpp file.)
