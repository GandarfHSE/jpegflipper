#include <iostream>
#include <memory>
#include "evhttp.h"

#define TRY(pred, msg) do { \
    if (!(pred)) { \
        std::cerr << msg << ": FAIL\n"; \
        return 1; \
    } else { \
        std::cerr << msg << ": SUCCESS\n"; \
    } \
} while (0)

void gen_cb(evhttp_request* request, void* arg) {
    std::cerr << "Generic callback\n";
    evbuffer* inBuf = evhttp_request_get_input_buffer(request);
    evbuffer* outBuf = evhttp_request_get_output_buffer(request);
    
    while (evbuffer_get_length(inBuf)) {
        evbuffer_remove_buffer(inBuf, outBuf, 1024);
    }

    evhttp_send_reply(request, HTTP_OK, "", outBuf);
}

bool isWorking = true;

void exit_cb(evhttp_request* request, void* arg) {
    std::cerr << "Exit callback\n";
    isWorking = false;
    evhttp_send_reply(request, HTTP_OK, "", nullptr);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cout << "Usage: " << *argv[0] << " [address][port]\n";
        return 0;
    }

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
    while (isWorking) {
        event_base_loop(evBasePtr.get(), EVLOOP_NONBLOCK);
    }

    evhttp_del_accept_socket(serverPtr.get(), boundSocketPtr);

    std::cerr << "exit normally\n";

    return 0;
}
