#include "gamelibs/app.hpp"

#include "jwt-cpp/jwt.h"
#include "jwt-cpp/traits/nlohmann-json/traits.h"

#include "google/protobuf/message.h"
#include "proto/helloworld.pb.h"

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
        SPDLOG_ERROR("CTX:{} /game/v1/login mongo exception {}", MYARGS.CTXID.c_str(), e.what());
    }

    return std::nullopt;
}

bool updateGoogleOrder(const int64_t uid, const std::string& oid)
{
    auto conn = CMongo::Instance().Conn();
    mongocxx::v_noabi::collection col = (*conn)[MYARGS.DbName[0]][MYARGS.DbColl[0][2]];
    mongocxx::options::find_one_and_update opts;
    opts.upsert(true);
    auto query = make_document(kvp("_id", bsoncxx::types::b_utf8 { oid }));
    auto update = make_document(kvp("$setOnInsert",
        make_document(
            kvp("uid", bsoncxx::types::b_int64 { uid }),
            kvp("creattime", bsoncxx::types::b_date { std::chrono::milliseconds { CChrono::NowMs() } }))));

    auto result = col.find_one_and_update(query.view(), update.view(), opts);
    return result ? true : false;
}

bool updateIOSOrder(const int64_t uid, const std::string& oid)
{
    auto conn = CMongo::Instance().Conn();
    mongocxx::v_noabi::collection col = (*conn)[MYARGS.DbName[0]][MYARGS.DbColl[0][3]];
    mongocxx::options::find_one_and_update opts;
    opts.upsert(true);
    auto query = make_document(kvp("_id", bsoncxx::types::b_utf8 { oid }));
    auto update = make_document(kvp("$setOnInsert",
        make_document(
            kvp("uid", bsoncxx::types::b_int64 { uid }),
            kvp("creattime", bsoncxx::types::b_date { std::chrono::milliseconds { CChrono::NowMs() } }))));

    auto result = col.find_one_and_update(query.view(), update.view(), opts);
    return result ? true : false;
}

void CApp::WebRegister(CHTTPServer* srv)
{
    srv->RegEvent("finish",
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) -> std::optional<std::pair<ghttp::HttpStatusCode, std::string>> {
            auto status = rsp->GetStatus().value_or(ghttp::HttpStatusCode::OK);
            SPDLOG_INFO("CTX: {} Response user {} status {} body {}",
                MYARGS.CTXID,
                req->GetUid().value_or(0),
                (int32_t)status,
                rsp->GetBody().value_or(""));
            return std::nullopt;
        });
    srv->RegEvent("start",
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) -> std::optional<std::pair<ghttp::HttpStatusCode, std::string>> {
            if (req->GetPath() == "/game/v1/login"
                || req->GetPath() == "/game/debug/echo"
                || req->GetPath() == "/game/debug/ws") {
                SPDLOG_INFO("CTX:{} user Request api {} body {} from ip {}",
                    MYARGS.CTXID,
                    req->GetPath(),
                    req->GetBody(),
                    req->Conn()->GetConnection()->GetPeerIp());
                return std::nullopt;
            }
            auto auth = req->GetHeaderByKey("authorization");
            if (auth.empty())
                return std::optional<std::pair<ghttp::HttpStatusCode, std::string>>({ ghttp::HttpStatusCode::UNAUTHORIZED, "" });
            try {
                auto res = CStringTool::Split(auth, " ");
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
                    SPDLOG_DEBUG("CTX:{} user {} Request api {} body {} from ip {}",
                        MYARGS.CTXID,
                        req->GetUid().value_or(0),
                        req->GetPath(),
                        req->GetBody(),
                        req->Conn()->GetConnection()->GetPeerIp());
                    return std::nullopt;
                }
            } catch (const std::exception& e) {
                SPDLOG_ERROR("CTX:{} jwt verify exception {}", MYARGS.CTXID, e.what());
            }
            return std::optional<std::pair<ghttp::HttpStatusCode, std::string>>({ ghttp::HttpStatusCode::UNAUTHORIZED, "" });
        });
    srv->Register(
        "/game/v1/login",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            if (req->GetBody().empty())
                return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
            try {
                auto j = nlohmann::json::parse(req->GetBody());

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
                    SPDLOG_ERROR("CTX:{} /game/v1/login mongo exception {}", MYARGS.CTXID, e.what());
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
                                SPDLOG_ERROR("CTX:{} /game/v1/login mongo exception {}", MYARGS.CTXID, e.what());
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
                SPDLOG_ERROR("CTX:{} /game/v1/login exception {}", MYARGS.CTXID, e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, "Database Exception" });
            }
            return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
        });
    srv->Register(
        "/game/v1/getuser",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            auto v = CLocalStorage<int64_t>::Instance().Get("user", req->GetUid().value());
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
                CLocalStorage<int64_t>::Instance().Set("user", req->GetUid().value(), j.dump());
                return rsp->Response({ ghttp::HttpStatusCode::OK, j.dump() });
            } catch (const std::exception& e) {
                SPDLOG_ERROR("CTX:{} /game/v1/getuser mongo exception {}", MYARGS.CTXID, e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, "Database Exception" });
            }
            return true;
        });
    srv->Register(
        "/game/v1/setuser",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            if (req->GetBody().empty())
                return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
            try {
                auto j = nlohmann::json::parse(req->GetBody());
                auto conn = CMongo::Instance().Conn();
                mongocxx::v_noabi::collection col = (*conn)[MYARGS.DbName[0]][MYARGS.DbColl[0][0]];
                auto query = make_document(kvp("_id", bsoncxx::types::b_int64 { req->GetUid().value() }));
                auto update = make_document(kvp("$set", make_document(kvp("value", bsoncxx::types::b_utf8 { j["value"].get<std::string>() }))));

                col.update_one(query.view(), update.view());
            } catch (const std::exception& e) {
                SPDLOG_ERROR("CTX:{} /game/v1/setuser exception {}", MYARGS.CTXID, e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, e.what() });
            }

            // CRedisMgr::Instance().Handle(0)->del(std::to_string(req->GetUid().value()));
            nlohmann::json j;
            j["cmd"] = "UpdateUser";
            j["uid"] = req->GetUid().value();
            CWorker::LOCAL_WORKER->SendMsgToMain(CChannel::MsgType::Json, j.dump());
            nlohmann::json js;
            return rsp->Response({ ghttp::HttpStatusCode::OK, js.dump() });
        });
    srv->Register(
        "/game/v1/google_purchase",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            if (req->GetBody().empty())
                return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
            try {
                auto j = nlohmann::json::parse(req->GetBody());
                std::string key = "-----BEGIN PUBLIC KEY-----\n" + MYARGS.ConfMap["gopubkey"] + "\n-----END PUBLIC KEY-----";
                std::string origin = j["receipt-data"].get<std::string>();
                if (CUtils::RSAVerify(CUtils::OPENSSL_ALGO::OPENSSL_ALGO_SHA1, key, origin, CUtils::Base64Decode(j["signature"].get<std::string>()))) {
                    auto oj = nlohmann::json::parse(origin);
                    updateGoogleOrder(req->GetUid().value(), oj["purchaseToken"].get<std::string>());
                    nlohmann::json js;
                    js["status"] = "ok";
                    js["productid"] = oj["productId"].get<std::string>();
                    js["quantity"] = oj["quantity"].get<int32_t>();
                    nlohmann::json ejs;
                    ejs["uid"] = req->GetUid().value();
                    ejs["paytype"] = 1;
                    ejs["orderid"] = oj["orderId"].get<std::string>();
                    ejs["productid"] = oj["productId"].get<std::string>();
                    ejs["quantity"] = oj["quantity"].get<int32_t>();
                    ejs["balance"] = 0;
                    ejs["ip"] = req->Conn()->GetConnection()->GetPeerIp();
                    CClickHouseMgr::Instance().Push("payevent", ejs.dump());

                    return rsp->Response({ ghttp::HttpStatusCode::OK, js.dump() });
                } else {
                    nlohmann::json js;
                    js["status"] = "failed";
                    js["msg"] = "failed to verify";
                    return rsp->Response({ ghttp::HttpStatusCode::OK, js.dump() });
                }
            } catch (const std::exception& e) {
                SPDLOG_ERROR("CTX:{} /game/v1/google_purchase exception {}", MYARGS.CTXID, e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, e.what() });
            }
        });
    srv->Register(
        "/game/v1/ios_purchase",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            if (req->GetBody().empty())
                return rsp->Response({ ghttp::HttpStatusCode::BADREQUEST, "Invalid Parameters" });
            try {
                auto j0 = nlohmann::json::parse(req->GetBody());
                std::string productid = j0["productid"].get<std::string>();
                auto j = nlohmann::json::parse(j0["receipt-data"].get<std::string>());
                nlohmann::json reqbody;
                reqbody["receipt-data"] = j["Payload"].get<std::string>();
                // CINFO("http %s response body", response->GetBody().value().data());
                reqbody["password"] = MYARGS.ConfMap["iossharedkey"];
                ghttp::CResponse* r = rsp->Clone();
                std::string body = reqbody.dump();
                const int64_t uid = req->GetUid().value();
                std::string ip = req->Conn()->GetConnection()->GetPeerIp();
                CHTTPClient::Emit(
                    MYARGS.ConfMap["iosprourl"],
                    [r, body, uid, ip, productid](ghttp::CResponse* response) {
                        if (response->GetBody()) {
                            try {
                                auto j = nlohmann::json::parse(response->GetBody().value());
                                if (21007 == j["status"].get<int32_t>()) {
                                    CHTTPClient::Emit(
                                        MYARGS.ConfMap["iossanboxurl"],
                                        [r, uid, ip, productid](ghttp::CResponse* response) {
                                            if (response->GetBody()) {
                                                try {
                                                    auto j = nlohmann::json::parse(response->GetBody().value());
                                                    if (0 == j["status"]) {
                                                        nlohmann::json js;
                                                        js["status"] = "ok";
                                                        js["productid"] = productid;
                                                        auto a = nlohmann::json::array();
                                                        for (auto it = j["receipt"]["in_app"].begin(); it != j["receipt"]["in_app"].end(); ++it) {
                                                            nlohmann::json vv;
                                                            vv["productid"] = (*it)["product_id"].get<std::string>();
                                                            vv["quantity"] = CStringTool::ToInteger<int32_t>((*it)["quantity"].get<std::string>());
                                                            a.push_back(vv);
                                                            if (!productid.empty() && productid == (*it)["product_id"].get<std::string>()) {
                                                                updateIOSOrder(uid, (*it)["transaction_id"].get<std::string>());
                                                                nlohmann::json ejs;
                                                                ejs["uid"] = uid;
                                                                ejs["paytype"] = 2;
                                                                ejs["orderid"] = (*it)["transaction_id"].get<std::string>();
                                                                ejs["productid"] = (*it)["product_id"].get<std::string>();
                                                                ejs["quantity"] = CStringTool::ToInteger<int32_t>((*it)["quantity"].get<std::string>());
                                                                ejs["balance"] = 0;
                                                                ejs["ip"] = ip;
                                                                CClickHouseMgr::Instance().Push("payevent", ejs.dump());
                                                            }
                                                        }
                                                        js["inapp"] = a;
                                                        r->Response({ ghttp::HttpStatusCode::OK, js.dump() });
                                                    }
                                                } catch (const std::exception& e) {
                                                    SPDLOG_ERROR("CTX:{} /game/v1/ios_purchase exception {}", MYARGS.CTXID, e.what());
                                                    r->Response({ ghttp::HttpStatusCode::INTERNAL, e.what() });
                                                }
                                            }
                                            delete r;
                                            return true;
                                        },
                                        ghttp::HttpMethod::POST, body);
                                    return true;
                                } else if (0 == j["status"].get<int32_t>()) {
                                    if (0 == j["status"]) {
                                        nlohmann::json js;
                                        js["status"] = "ok";
                                        js["productid"] = productid;
                                        auto a = nlohmann::json::array();
                                        for (auto it = j["receipt"]["in_app"].begin(); it != j["receipt"]["in_app"].end(); ++it) {
                                            nlohmann::json vv;
                                            vv["productid"] = (*it)["product_id"].get<std::string>();
                                            vv["quantity"] = CStringTool::ToInteger<int32_t>((*it)["quantity"].get<std::string>());
                                            a.push_back(vv);
                                            if (!productid.empty() && productid == (*it)["product_id"].get<std::string>()) {
                                                updateIOSOrder(uid, (*it)["transaction_id"].get<std::string>());
                                                nlohmann::json ejs;
                                                ejs["uid"] = uid;
                                                ejs["paytype"] = 2;
                                                ejs["orderid"] = (*it)["transaction_id"].get<std::string>();
                                                ejs["productid"] = (*it)["product_id"].get<std::string>();
                                                ejs["quantity"] = CStringTool::ToInteger<int32_t>((*it)["quantity"].get<std::string>());
                                                ejs["balance"] = 0;
                                                ejs["ip"] = ip;
                                                CClickHouseMgr::Instance().Push("payevent", ejs.dump());
                                            }
                                        }
                                        js["inapp"] = a;
                                        r->Response({ ghttp::HttpStatusCode::OK, js.dump() });
                                    }
                                    delete r;
                                }
                            } catch (const std::exception& e) {
                                SPDLOG_ERROR("CTX:{} /game/v1/ios_purchase exception {}", MYARGS.CTXID, e.what());
                                r->Response({ ghttp::HttpStatusCode::INTERNAL, e.what() });
                                delete r;
                                return true;
                            }
                        }
                        return true;
                    },
                    ghttp::HttpMethod::POST,
                    body);
                return false;
            } catch (const std::exception& e) {
                SPDLOG_ERROR("CTX:{} /game/v1/ios_purchase exception {}", MYARGS.CTXID, e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, e.what() });
            }
        });
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
    srv->Register(
        "/game/v1/statistic",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            std::map<std::string, std::map<int64_t, int64_t>> smap;
            try {
                auto conn = CMongo::Instance().Conn();
                mongocxx::v_noabi::collection col = (*conn)[MYARGS.DbName[0]][MYARGS.DbColl[0][0]];
                auto query = bsoncxx::builder::basic::document {};
                auto cursor = col.find(query.view());
                for (auto&& doc : cursor) {
                    if (auto it = doc.find("value"); it != doc.end()) {
                        std::string_view value = it->get_utf8();
                        auto j = nlohmann::json::parse(value);
                        for (auto& [k, v] : j.items()) {
                            if (k == "GameLevel" && v["value"].is_number()) {
                                smap[k][v["value"]] = smap[k][v["value"]] + 1;
                            }
                        }
                    }
                }
            } catch (const std::exception& e) {
                SPDLOG_ERROR("CTX:{} /game/v1/logevent exception {}", MYARGS.CTXID, e.what());
                return rsp->Response({ ghttp::HttpStatusCode::INTERNAL, e.what() });
            }

            nlohmann::json js(smap);
            return rsp->Response({ ghttp::HttpStatusCode::OK, js.dump() });
        });

    srv->Register(
        "/game/debug/rpc",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            auto auth = req->GetHeaderByKey("authorization");
            ghttp::CResponse* r = rsp->Clone();
            CHTTPClient::Emit(MYARGS.ConfMap["test_rpcurl"],
                [r](ghttp::CResponse* response) {
                    r->Response({ response->GetStatus().value(), std::string(response->GetBody().value_or("").data(), response->GetBody().value_or("").length()) });
                    delete r;
                    return true;
                },
                ghttp::HttpMethod::POST,
                std::nullopt,
                {
                    { "authorization", auth },
                });
            return false;
        });

    srv->ServeWs("/game/debug/ws", [](CWebSocket* ws, std::string_view data) {
        SPDLOG_DEBUG("websocket on data {}", data);
        ws->SendCmd(CWSParser::WS_OPCODE_TXTFRAME, data);
        return true;
    });

    srv->Register(
        "/game/debug/wscli",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            std::string url = MYARGS.ConfMap["test_wsurl"];
            CWebSocket::Connect(url, [](CWebSocket*, std::string_view data) {
                SPDLOG_DEBUG("websocket on data {}", data);
                return true;
            });
            return rsp->Response({ ghttp::HttpStatusCode::OK, "OK" });
        });

    srv->Register(
        "/game/debug/lua",
        ghttp::HttpMethod::DISABLE,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            CLuaState::MAIN_LUASTATE->PushFunc("Dispatch");
            CLuaState::MAIN_LUASTATE->PushInteger(req->GetUid().value_or(0));
            CLuaState::MAIN_LUASTATE->PushInteger(0);
            CLuaState::MAIN_LUASTATE->PushInteger(0);
            CLuaState::MAIN_LUASTATE->PushInteger(0);
            CLuaState::MAIN_LUASTATE->PushString(req->GetBody());
            auto&& res = CLuaState::MAIN_LUASTATE->Call<std::string>(5, 1);
            return rsp->Response({ ghttp::HttpStatusCode::OK, res });
        });

    srv->Register(
        "/game/debug/echo",
        ghttp::HttpMethod::GETPOST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            return rsp->Response({ ghttp::HttpStatusCode::OK, req->GetBody() });
        });

    srv->Register(
        "/helloworld.Greeter/SayHello",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            helloworld::HelloRequest hello;
            if (hello.ParseFromString(req->GetBody())) {
                SPDLOG_DEBUG("Grpc on data {}", hello.name());
            }
            helloworld::HelloReply reply;
            reply.set_message(fmt::format("hello {}", hello.name()));
            return rsp->Response({ ghttp::HttpStatusCode::OK, reply.SerializeAsString() });
        });

    srv->Register(
        "/game/debug/grpc",
        ghttp::HttpMethod::POST,
        [](ghttp::CRequest* req, ghttp::CResponse* rsp) {
            auto auth = req->GetHeaderByKey("authorization");
            ghttp::CResponse* r = rsp->Clone();
            helloworld::HelloRequest hello;
            hello.set_name("Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            Submits GOAWAY frame with the last stream ID last_stream_id and the error code error_code.\
            ");
            CHTTPClient::Emit(MYARGS.ConfMap["test_grpcurl"],
                [r](ghttp::CResponse* response) {
                    helloworld::HelloRequest hello;
                    hello.ParseFromArray(response->GetBody().value_or("").data(), response->GetBody().value_or("").length());
                    r->Response({ response->GetStatus().value(), hello.name() });
                    delete r;
                    return true;
                },
                ghttp::HttpMethod::POST,
                hello.SerializeAsString(),
                {
                    { "authorization", auth },
                    { "content-type", "application/grpc" },
                    { "te", "trailers" },
                    { "grpc-encoding", "identity" },
                    { "user-agent", "grpc-c++/1.46.3 grpc-c/24.0.0 (linux; chttp2)" },
                });
            return false;
        });
}

void CApp::CommandRegister(CApp::AppWorker* w)
{
    w->RegisterCommand("UpdateUser",
        [](const nlohmann::json& j) {
            if (j.contains("uid") && j["uid"].is_number_integer()) {
                CLocalStorage<int64_t>::Instance().Remove("user", j["uid"].get<int64_t>());
            }
            return true;
        });
}
