#include "gamelibs/app.hpp"

#include "jwt-cpp/jwt.h"
#include "jwt-cpp/traits/nlohmann-json/traits.h"

USE_NAMESPACE_FRAMEWORK

int main(int argc, char** argv)
{
    CheckCondition(CApp::Start(argc, argv), EXIT_FAILURE);
    return EXIT_SUCCESS;
}

void CApp::TcpRegister(CWorker* w, const std::string host, std::shared_ptr<CTCPServer> srv)
{
    srv->ListenAndServe(host, [host](const int32_t fd) {
        CConnectionHandler* h = CNEW CConnectionHandler();
        h->Register(
            [](std::string_view data) {
                return 0;
            },
            []() {
            },
            [](const EnumConnEventType e) {
            });
        h->Init(fd, host);
    });
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

void CApp::WebRegister(CWorker* w, std::shared_ptr<CHTTPServer> srv)
{
    srv->RegEvent("finish",
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) -> std::optional<std::pair<ghttp::HttpStatusCode, std::string>> {
            auto status = rsp->GetStatus().value_or(ghttp::HttpStatusCode::OK);
            auto body = std::string(rsp->GetBody().value_or("").data(), rsp->GetBody().value_or("").length());
            CDEBUG("CTX:%s Http Response status %s body %s", MYARGS.CTXID.c_str(), ghttp::HttpReason(status).value_or(""), body.c_str());
            return std::nullopt;
        });
    srv->RegEvent("start",
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) -> std::optional<std::pair<ghttp::HttpStatusCode, std::string>> {
            if (req->GetPath().value_or("") == "/game/v1/login") {
                auto path = std::string(req->GetPath().value_or("").data(), req->GetPath().value_or("").length());
                auto body = std::string(req->GetBody().value_or("").data(), req->GetBody().value_or("").length());
                CDEBUG("CTX:%s Http Request api %s body %s", MYARGS.CTXID.c_str(), path.c_str(), body.c_str());
                return std::nullopt;
            }
            auto auth = req->GetHeaderByKey("authorization");
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
                    auto path = std::string(req->GetPath().value_or("").data(), req->GetPath().value_or("").length());
                    auto body = std::string(req->GetBody().value_or("").data(), req->GetBody().value_or("").length());
                    CDEBUG("CTX:%s Http Request api %s body %s", MYARGS.CTXID.c_str(), path.c_str(), body.c_str());
                    return std::nullopt;
                }
            } catch (const std::exception& e) {
                CERROR("CTX:%s jwt verify exception %s", MYARGS.CTXID.c_str(), e.what());
            }
            return std::optional<std::pair<ghttp::HttpStatusCode, std::string>>({ ghttp::HttpStatusCode::UNAUTHORIZED, "" });
        });
    srv->Register(
        "/game/v1/login",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            if (!req->GetBody())
                return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
            try {
                auto j = nlohmann::json::parse(req->GetBody().value());

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
                    js["uid"] = uid.value();
                    return rsp->Response({ ghttp::HttpStatusCode::OK, js.dump() });
                }
            } catch (const std::exception& e) {
                CERROR("CTX:%s /game/v1/login exception %s", MYARGS.CTXID.c_str(), e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, "Database Exception" });
            }
            return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
        });
    srv->Register(
        "/game/v1/getuser",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            auto v = CLocalStoreage<int64_t>::Instance().Get("user", req->GetUid().value());
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
                CLocalStoreage<int64_t>::Instance().Set("user", req->GetUid().value(), j.dump());
                return rsp->Response({ ghttp::HttpStatusCode::OK, j.dump() });
            } catch (const std::exception& e) {
                CERROR("CTX:%s /game/v1/getuser mongo exception %s", MYARGS.CTXID.c_str(), e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, "Database Exception" });
            }
            return true;
        });
    srv->Register(
        "/game/v1/setuser",
        ghttp::HttpMethod::POST,
        [w](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            if (!req->GetBody())
                return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
            try {
                auto j = nlohmann::json::parse(req->GetBody().value());
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
            nlohmann::json j;
            j["cmd"] = "UpdateUser";
            j["uid"] = req->GetUid().value();
            w->SendMsgToMain(CChannel::MsgType::Json, j.dump());
            nlohmann::json js;
            return rsp->Response({ ghttp::HttpStatusCode::OK, js.dump() });
        });

    srv->Register(
        "/game/v1/rpc",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            auto v = req->GetHeaderByKey("authorization").value_or("");
            auto auth = std::string(v.data(), v.size());
            ghttp::CResponse* r = rsp->Clone();
            CHTTPClient::Emit("https://dev.wgnice.com:9021/game/v1/getuser",
                [r](ghttp::CResponse* response) {
                    if (response->GetBody()) {
                        r->Response({ response->GetStatus().value(), std::string(response->GetBody().value().data(), response->GetBody().value().length()) });
                        delete r;
                    }
                    return true;
                },
                ghttp::HttpMethod::POST,
                std::nullopt,
                { { "authorization", auth.data() } });
            return false;
        });

    srv->ServeWs("/game/ws", [](CWebSocket* ws, std::string_view data) {
        std::string d(data.data(), data.length());
        CINFO("websocket on data %s", d.c_str());
        ws->SendCmd(CWSParser::WS_OPCODE_TXTFRAME, data);
        return true;
    });

    srv->Register(
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

void CApp::CommandRegister(std::map<std::string, CommandSyncFunc>& ref)
{
    ref["UpdateUser"] = [](const nlohmann::json& j) {
        if (j.contains("uid") && j["uid"].is_number_integer()) {
            CLocalStoreage<int64_t>::Instance().Remove("user", j["uid"].get<int64_t>());
        }
        return true;
    };
}
