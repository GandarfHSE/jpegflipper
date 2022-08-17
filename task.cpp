#include <exception>
#include <memory>

#include <evhttp.h>
#include <Magick++.h>

#include "task.h"

// Do generic callback
void CbTask::operator() () {
    try {
        evbuffer* inBuf = evhttp_request_get_input_buffer(request);
        evbuffer* outBuf = evhttp_request_get_output_buffer(request);

        size_t imgLen = evbuffer_get_length(inBuf);
        std::unique_ptr<char[]> bufPtr(new char[imgLen + 3]);

        size_t readTotal = 0;
        while (evbuffer_get_length(inBuf)) {
            int readNow = evbuffer_remove(inBuf, &(bufPtr.get())[readTotal], imgLen);
            if (readNow > 0) {
                readTotal += readNow;
            }
        }

        // work with image
        Magick::Blob blob(static_cast<void*>(bufPtr.get()), imgLen + 1);
        Magick::Image image(blob);
        image.flop();
        image.write(&blob);

        if (evbuffer_add(outBuf, blob.data(), blob.length()) == -1) {
            evhttp_send_reply(request, HTTP_INTERNAL, "copy error occured", nullptr);
        }

        evhttp_send_reply(request, HTTP_OK, "", outBuf);
    }
    catch (std::exception& e) {
        evhttp_send_reply(request, HTTP_BADREQUEST, e.what(), nullptr);
    }
    catch (...) {
        evhttp_send_reply(request, HTTP_BADREQUEST, "some error occured", nullptr);
    }
}
