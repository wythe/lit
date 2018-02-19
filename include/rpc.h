#pragma once
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/un.h>
#include <wythe/exception.h>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

using json = nlohmann::json;

namespace rpc {
    struct remote_endpoint {
        remote_endpoint() {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            c = curl_easy_init();
            if (!c) PANIC("internet error");
            curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(c, CURLOPT_WRITEDATA, &s);
        }
        remote_endpoint(const remote_endpoint &) = delete;
        remote_endpoint & operator=(const remote_endpoint &) = delete;
        ~remote_endpoint() {
            curl_global_cleanup();
        }
        static size_t write_callback(void * contents, size_t size, size_t n, std::string *s) {
            size_t bytes = size * n;
            size_t old_size = s->size();
            s->resize(old_size + bytes);
            std::copy((char *) contents, (char*) contents + bytes, s->begin() + old_size);
            return bytes;
        }

        CURL * c;
        std::string s;
    };

    json request_remote(const std::string url) {
        remote_endpoint c;
        curl_easy_setopt(c.c, CURLOPT_URL, url.c_str());
        auto r = curl_easy_perform(c.c);
        if (r != CURLE_OK) PANIC("curl fail: " << curl_easy_strerror(r));
        return json::parse(c.s);
    }
    
    struct conn {
        conn() {
            fd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd < 0) PANIC("socket() failed");
            strcpy(addr.sun_path, "lightning-rpc");
            addr.sun_family = AF_UNIX;

            if (connect(fd, (struct sockaddr *) & addr, sizeof(addr)) != 0) PANIC("cannot connect to " << addr.sun_path);
        }

        conn(const conn &) = delete;
        conn & operator=(const conn &) = delete;

        ~conn() {
            if (fd != -1) close(fd);
        }

        int fd = -1;
        struct sockaddr_un addr;
    };

    bool write_all(int fd, const void *data, size_t size)
    {
        while (size) {
            ssize_t done;

            done = write(fd, data, size);
            if (done < 0 && errno == EINTR)
                continue;
            if (done <= 0)
                return false;
            data = (const char *)data + done;
            size -= done;
        }

        return true;
    }

    inline json read_all(const conn & c) {
        std::string v;
        v.reserve(360);
        char ch;
        int r;
        while (r = read(c.fd, &ch, 1)) {
            if (r <= 0) PANIC("error talking to lightningd");
            v.push_back(ch);
            int avail;
            ioctl(c.fd, FIONREAD, &avail);
            if (avail == 0) break;
        }
        return json::parse(v);
    }

    json request_local(const json & req) {
        conn c;
        std::string s = req.dump();
        write_all(c.fd, s.data(), s.size());
        return read_all(c);
    }
}
