#pragma once
#include <evhttp.h>

class CbTask {
public:
    CbTask(evhttp_request* req) : request(req) {
    }

    // Do generic callback
    void operator() ();

private:
    evhttp_request* request;
};
