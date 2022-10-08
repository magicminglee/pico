# PICO

## Overview

Pico is a C++17/20 based HTTP web application framework running on Linux/macOS/Unix

Its main features are as follows:

- Use a non-blocking I/O network lib based on libevent to provide high-concurrency, high-performance network IO;
- Support Http1.0/1.1 (server side and client side);
- Support https (based on OpenSSL);
- Support WebSocket (server side and client side);
- Support JSON format request and response, very friendly to the Restful API application development;
- Support filter chains to facilitate the execution of unified logic (such as login verification, etc.) before or after handling HTTP requests;
- Support file download;
- Support Redis with reading and writing;
- Support MongoDB with reading and writing;
- Support reload configuration file at load time;
- Support Http2 (server side and client side);
- Support GRPC (server side and client side);
- Support TLS(thread local storage) mechanism;

## Quick Example

Below is the main program of a typical pico application:

```c++
#include "gamelibs/app.hpp"

USE_NAMESPACE_FRAMEWORK

int main(int argc, char** argv)
{
    CheckCondition(CApp::Start(argc, argv), EXIT_FAILURE);
    return EXIT_SUCCESS;
}

void CApp::WebRegister(CHTTPServer* srv) {
    srv->Register(
        "/game/v1/logevent",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            if (req->GetBody().empty())
                return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
            try {
                auto j = nlohmann::json::parse(req->GetBody());
                const auto& name = j["name"].get<std::string>();
                const auto& parm = j["parm"].get<std::string>();
                CClickHouseMgr::Instance().Push(name, parm);
            } catch (const std::exception& e) {
                SPDLOG_ERROR("CTX:{} /game/v1/logevent exception {}", MYARGS.CTXID, e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, e.what() });
            }

            nlohmann::json js;
            return rsp->Response({ ghttp::HttpStatusCode::OK, js.dump() });
        });
}

```

## Install Pico

### Install dependency libraries

```shell
cd external
sh install.sh
```

### Build

```shell
cmake -DCMAKE_BUILD_TYPE="Debug" -DCMAKE_PREFIX_PATH="/data/user00/pico/serverdev/src/external" -H. -Bbuild
cd build && make -j4
```

### Launch

```shell
./bin/PICO -f ../config/config.yaml
```
