# analytics-cpp

A quick & dirty C++ implementation for sending analytics events to Segment.

This library is in its early-POC phase and its API/ABI will certainly change. **This is not production-ready software.**

## Building

To keep the codebase small/simple to work with, we rely on `libcurl` for making our HTTP requests. Install `libcurl` with `apt-get install -qq libcurl4-gnutls-dev`, then do:

```
$ make example
$ ./example
```
