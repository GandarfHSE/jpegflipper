#include <iostream>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

#include <evhttp.h>
#include <Magick++.h>

#include "task.h"
#include "threadpool.h"

#define TRY(pred, msg) do { \
    if (!(pred)) { \
        std::cerr << msg << ": FAIL\n"; \
        return 1; \
    } else { \
        std::cerr << msg << ": SUCCESS\n"; \
    } \
} while (0)

bool _isWorking = true;
ThreadPool pool;

void gen_cb(evhttp_request* request, void* arg) {
    std::cerr << "Generic callback\n";
    CbTask task(request);
    if (!pool.PushTask(task)) {
        evhttp_send_reply(request, HTTP_SERVUNAVAIL, "", nullptr);
    }
}

void exit_cb(evhttp_request* request, void* arg) {
    std::cerr << "Exit callback\n";
    _isWorking = false;
    evhttp_send_reply(request, HTTP_OK, "", nullptr);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: %s [address][port]\n", argv[0]);
        return 0;
    }

    Magick::InitializeMagick(nullptr);

    char* address = argv[1];
    uint16_t port = atoi(argv[2]);

    std::unique_ptr<event_base, decltype(&event_base_free)> evBasePtr(event_base_new(), &event_base_free);
    TRY(evBasePtr, "evBase init");
    std::unique_ptr<evhttp, decltype(&evhttp_free)> serverPtr(evhttp_new(evBasePtr.get()), &evhttp_free);
    TRY(serverPtr, "server init");

    // set callback for server shutdown
    TRY(evhttp_set_cb(serverPtr.get(), "/exit", exit_cb, nullptr) == 0, "set cb on exit");
    // set generic callback - jpeg flips
    evhttp_set_gencb(serverPtr.get(), gen_cb, nullptr);

    evhttp_bound_socket* boundSocketPtr = evhttp_bind_socket_with_handle(serverPtr.get(), address, port);
    TRY(boundSocketPtr, "bind socket");
    evutil_socket_t socketFD = evhttp_bound_socket_get_fd(boundSocketPtr);
    TRY(evhttp_accept_socket(serverPtr.get(), socketFD) == 0, "connection accept");

    // event loop
    while (_isWorking) {
        event_base_loop(evBasePtr.get(), EVLOOP_NONBLOCK);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    evhttp_del_accept_socket(serverPtr.get(), boundSocketPtr);

    std::cerr << "exit normally\n";

    return 0;
}
