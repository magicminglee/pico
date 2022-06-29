#include "gamelibs/app.hpp"

#include "jwt-cpp/jwt.h"
#include "jwt-cpp/traits/nlohmann-json/traits.h"

USE_NAMESPACE_FRAMEWORK

int main(int argc, char** argv)
{
    CheckCondition(CApp::Start(argc, argv), EXIT_FAILURE);
    return EXIT_SUCCESS;
}

std::optional<int64_t> getNextSequenceId(const std::string& name)
{
    try {
        auto conn = CMongo::Instance().Conn();
        mongocxx::v_noabi::collection col = (*conn)[MYARGS.DbName[0]][MYARGS.DbColl[0][1]];
        mongocxx::options::find_one_and_update option {};
        option.upsert(true);
        option.return_document(mongocxx::options::return_document::k_after);
        auto query = make_document(kvp("_id", bsoncxx::types::b_utf8 { name }));
        auto update = make_document(kvp("$inc", make_document(kvp("uuid", bsoncxx::types::b_int64 { 1 }))));

        auto result = col.find_one_and_update(query.view(), update.view(), option);
        if (result) {
            bsoncxx::document::view doc = result->view();
            if (auto it = doc.find("uuid"); it != doc.end()) {
                return it->get_int64();
            }
        }
    } catch (const std::exception& e) {
        CERROR("CTX:%s /game/v1/login mongo exception %s", MYARGS.CTXID.c_str(), e.what());
    }

    return std::nullopt;
}

void CApp::WebRegister(std::shared_ptr<CHTTPServer> hs)
{
    hs->RegEvent("finish",
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) -> std::optional<std::pair<ghttp::HttpStatusCode, std::string>> {
#ifdef _DEBUG_MODE
            auto status = rsp->GetResponseStatus().value_or(ghttp::HttpStatusCode::OK);
            auto body = std::string(rsp->GetResponseBody().value_or("").data(), rsp->GetResponseBody().value_or("").length());
            CDEBUG("CTX:%s Http Response status %s body %s", MYARGS.CTXID.c_str(), ghttp::HttpReason(status).value_or(""), body.c_str());
#endif
            return std::nullopt;
        });
    hs->RegEvent("start",
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) -> std::optional<std::pair<ghttp::HttpStatusCode, std::string>> {
            if (req->GetRequestPath().value_or("") == "/game/v1/login") {
#ifdef _DEBUG_MODE
                auto path = std::string(req->GetRequestPath().value_or("").data(), req->GetRequestPath().value_or("").length());
                auto body = std::string(req->GetRequestBody().value_or("").data(), req->GetRequestBody().value_or("").length());
                CDEBUG("CTX:%s Http Request api %s body %s", MYARGS.CTXID.c_str(), path.c_str(), body.c_str());
#endif
                return std::nullopt;
            }
            auto auth = req->GetHeaderByKey("Authorization");
            if (!auth)
                return std::optional<std::pair<ghttp::HttpStatusCode, std::string>>({ ghttp::HttpStatusCode::UNAUTHORIZED, "" });
            try {
                auto res = CStringTool::Split(auth.value(), " ");
                if (2 == res.size()) {
                    const auto decoded = jwt::decode<jwt::traits::nlohmann_json>(res[1].data());
                    auto verifier = jwt::verify<jwt::traits::nlohmann_json>()
                                        .allow_algorithm(jwt::algorithm::hs256 { MYARGS.ApiKey.value_or("") });
                    verifier.verify(decoded);

                    for (auto& e : decoded.get_payload_claims()) {
                        if (e.first == "uid")
                            req->SetUid(e.second.as_int());
                    }
                    if (!req->GetUid())
                        return std::optional<std::pair<ghttp::HttpStatusCode, std::string>>({ ghttp::HttpStatusCode::UNAUTHORIZED, "No UserId" });
#ifdef _DEBUG_MODE
                    auto path = std::string(req->GetRequestPath().value_or("").data(), req->GetRequestPath().value_or("").length());
                    auto body = std::string(req->GetRequestBody().value_or("").data(), req->GetRequestBody().value_or("").length());
                    CDEBUG("CTX:%s Http Request api %s body %s", MYARGS.CTXID.c_str(), path.c_str(), body.c_str());
#endif
                    return std::nullopt;
                }
            } catch (const std::exception& e) {
                CERROR("CTX:%s jwt verify exception %s", MYARGS.CTXID.c_str(), e.what());
            }
            return std::optional<std::pair<ghttp::HttpStatusCode, std::string>>({ ghttp::HttpStatusCode::UNAUTHORIZED, "" });
        });
    hs->Register(
        "/game/v1/login",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            if (!req->GetRequestBody())
                return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
            try {
                auto j = nlohmann::json::parse(req->GetRequestBody().value());

                auto conn = CMongo::Instance().Conn();
                mongocxx::v_noabi::collection col = (*conn)[MYARGS.DbName[0]][MYARGS.DbColl[0][0]];
                mongocxx::options::find_one_and_update option {};
                option.return_document(mongocxx::options::return_document::k_after);
                bsoncxx::stdx::optional<bsoncxx::document::value> result;
                auto query = make_document(kvp("account", bsoncxx::types::b_utf8 { j["account"].get<std::string>() }));
                auto update = make_document(
                    kvp("$set",
                        make_document(kvp("type", bsoncxx::types::b_utf8 { j["type"].get<std::string>() }),
                            kvp("logintime", bsoncxx::types::b_date { std::chrono::system_clock::now() }))));

                try {
                    result = col.find_one_and_update(query.view(), update.view(), option);
                } catch (const std::exception& e) {
                    CERROR("CTX:%s /game/v1/login mongo exception %s", MYARGS.CTXID.c_str(), e.what());
                    return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, "Database Exception" });
                }

                std::optional<int64_t> uid;
                if (result) {
                    bsoncxx::document::view doc = result->view();
                    if (auto it = doc.find("_id"); it != doc.end()) {
                        uid = it->get_int64();
                    }
                } else {
                    if (j["type"].get<std::string>() == "anonymous") {
                        auto uuid = getNextSequenceId("account");
                        if (uuid) {
                            option.upsert(true);
                            auto query = make_document(kvp("account", bsoncxx::types::b_utf8 { j["account"].get<std::string>() }));
                            auto update = make_document(
                                kvp("$setOnInsert",
                                    make_document(kvp("passwd", bsoncxx::types::b_utf8 { j["passwd"].get<std::string>() }),
                                        kvp("_id", bsoncxx::types::b_int64 { uuid.value() }))),
                                kvp("$set",
                                    make_document(kvp("type", bsoncxx::types::b_utf8 { j["type"].get<std::string>() }),
                                        kvp("logintime", bsoncxx::types::b_date { std::chrono::system_clock::now() }))));

                            try {
                                result = col.find_one_and_update(query.view(), update.view(), option);
                                if (result) {
                                    bsoncxx::document::view doc = result->view();
                                    if (auto it = doc.find("_id"); it != doc.end()) {
                                        uid = it->get_int64();
                                    }
                                }
                            } catch (const std::exception& e) {
                                CERROR("CTX:%s /game/v1/login mongo exception %s", MYARGS.CTXID.c_str(), e.what());
                                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, "Database Exception" });
                            }
                        }
                    }
                }

                if (uid) {
                    auto token = jwt::create<jwt::traits::nlohmann_json>()
                                     .set_expires_at(jwt::date::clock::now() + std::chrono::minutes(MYARGS.TokenExpire.value()))
                                     .set_type("JWS")
                                     .set_payload_claim("uid", uid.value())
                                     .set_payload_claim("type", j["type"].get<std::string>())
                                     .sign(jwt::algorithm::hs256 { MYARGS.ApiKey.value_or("") });
                    nlohmann::json js;
                    js["token"] = token;
                    js["type"] = j["type"].get<std::string>();
                    return rsp->Response({ ghttp::HttpStatusCode::OK, js.dump() });
                }
            } catch (const std::exception& e) {
                CERROR("CTX:%s /game/v1/login exception %s", MYARGS.CTXID.c_str(), e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, "Database Exception" });
            }
            return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
        });
    hs->Register(
        "/game/v1/getuser",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            auto v = CLocalStoreage<int64_t>::Instance().Get(req->GetUid().value());
            if (v) {
                return rsp->Response({ ghttp::HttpStatusCode::OK, v.value()->data() });
            }
            try {
                auto conn = CMongo::Instance().Conn();
                mongocxx::v_noabi::collection col = (*conn)[MYARGS.DbName[0]][MYARGS.DbColl[0][0]];
                auto query = make_document(kvp("_id", bsoncxx::types::b_int64 { req->GetUid().value_or(0) }));

                auto result = col.find_one(query.view());
                nlohmann::json j;
                if (result) {
                    bsoncxx::document::view doc = result->view();
                    if (auto it = doc.find("value"); it != doc.end()) {
                        j["value"] = it->get_utf8();
                    }
                }
                CLocalStoreage<int64_t>::Instance().Set(req->GetUid().value(), j.dump());
                return rsp->Response({ ghttp::HttpStatusCode::OK, j.dump() });
            } catch (const std::exception& e) {
                CERROR("CTX:%s /game/v1/getuser mongo exception %s", MYARGS.CTXID.c_str(), e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, "Database Exception" });
            }
            return true;
        });
    hs->Register(
        "/game/v1/setuser",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            if (!req->GetRequestBody())
                return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
            try {
                auto j = nlohmann::json::parse(req->GetRequestBody().value());
                auto conn = CMongo::Instance().Conn();
                mongocxx::v_noabi::collection col = (*conn)[MYARGS.DbName[0]][MYARGS.DbColl[0][0]];
                auto query = make_document(kvp("_id", bsoncxx::types::b_int64 { req->GetUid().value() }));
                auto update = make_document(kvp("$set", make_document(kvp("value", bsoncxx::types::b_utf8 { j["value"].get<std::string>() }))));

                col.update_one(query.view(), update.view());
            } catch (const std::exception& e) {
                CERROR("CTX:%s /game/v1/setuser mongo exception %s", MYARGS.CTXID.c_str(), e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, "Database Exception" });
            }

            // CRedisMgr::Instance().Handle(0)->del(std::to_string(req->GetUid().value()));
            CLocalStoreage<int64_t>::Instance().Remove(req->GetUid().value());
            nlohmann::json js;
            return rsp->Response({ ghttp::HttpStatusCode::OK, js.dump() });
        });

    hs->Register(
        "/game/v1/rpc",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            auto v = req->GetHeaderByKey("Authorization").value_or("");
            auto auth = std::string(v.data(), v.size());
            ghttp::CResponse r = *rsp;
            CHTTPClient::Emit("https://dev.wgnice.com:9021/game/v1/getuser",
                [r](ghttp::CResponse* response) {
                    if (response->GetResponseBody()) {
                        r.Response({ response->GetResponseStatus().value(), std::string(response->GetResponseBody().value().data(), response->GetResponseBody().value().length()) });
                    }
                    return true;
                },
                ghttp::HttpMethod::POST,
                std::nullopt,
                { { "Authorization", auth.data() } });
            return false;
        });

    hs->ServeWs("/game/ws", [](CWebSocket* ws, std::string_view data) {
        std::string d(data.data(), data.length());
        CINFO("websocket on data %s", d.c_str());
        ws->SendCmd(CWSParser::WS_OPCODE_TXTFRAME, data);
        return true;
    });

    hs->Register(
        "/game/wscli",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            std::string url = "wss://access.indigames.in:9028";
            CWebSocket::Connect(url, [](CWebSocket*, std::string_view data) {
                std::string d(data.data(), data.length());
                CINFO("websocket on data %s", d.c_str());
                return true;
            });
            return rsp->Response({ ghttp::HttpStatusCode::OK, "OK" });
        });
}

void CApp::TcpRegister(const std::string host, std::shared_ptr<CTCPServer> hs)
{
    hs->ListenAndServe(host, [host](const int32_t fd) {
        CConnectionHandler* h = CNEW CConnectionHandler();
        h->Register(
            [h](std::string_view) { return true; },
            [h]() {
                CINFO("closed");
            },
            [h]() {
                CINFO("%s has been connected", h->Connection()->PeerIp());
                const unsigned char* alpn = nullptr;
                unsigned int alpnlen = 0;
                auto bv = h->Connection()->GetBufEvent();
                auto ssl = bufferevent_openssl_get_ssl(bv);
                if (!alpn) {
                    SSL_get0_alpn_selected(ssl, &alpn, &alpnlen);
                }
                if (!alpn || alpnlen != 2 || memcmp("h2", alpn, 2) != 0) {
                    CERROR("%s h2 is not negotiated", h->Connection()->PeerIp());
                }
            });
        h->Init(fd, host);
    });
}