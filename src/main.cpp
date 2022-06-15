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

void CApp::Register(std::shared_ptr<CHTTPServer> hs)
{
    hs->RegEvent("start",
        [](ghttp::CGlobalData* g) -> std::optional<std::pair<uint32_t, std::string>> {
            if (g->GetRequestPath().value_or("") == "/game/v1/login") {
                CINFO("CTX:%s Http Request api %s body %s", MYARGS.CTXID.c_str(), g->GetRequestBody().value_or("").data(), g->GetRequestBody().value_or("").data());
                return std::nullopt;
            }
            auto auth = g->GetHeaderByKey("Authorization");
            if (!auth)
                return std::optional<std::pair<uint32_t, std::string>>({ HTTP_UNAUTHORIZED, "" });
            try {
                auto res = CStringTool::Split(auth.value(), " ");
                if (2 == res.size()) {
                    const auto decoded = jwt::decode<jwt::traits::nlohmann_json>(res[1].data());
                    auto verifier = jwt::verify<jwt::traits::nlohmann_json>()
                                        .allow_algorithm(jwt::algorithm::hs256 { MYARGS.ApiKey.value_or("") });
                    verifier.verify(decoded);

                    for (auto& e : decoded.get_payload_claims()) {
                        if (e.first == "uid")
                            g->SetUid(e.second.as_int());
                    }
                    CINFO("CTX:%s Http Request api %s body %s", MYARGS.CTXID.c_str(), g->GetRequestBody().value_or("").data(), g->GetRequestBody().value_or("").data());
                    return std::nullopt;
                }
            } catch (const std::exception& e) {
                CERROR("CTX:%s jwt verify exception %s", MYARGS.CTXID.c_str(), e.what());
            }
            return std::optional<std::pair<uint32_t, std::string>>({ HTTP_UNAUTHORIZED, "" });
        });
    hs->Register(
        "/game/v1/login",
        EVHTTP_REQ_POST,
        [](const ghttp::CGlobalData* g) -> std::pair<uint32_t, std::string> {
            if (!g->GetRequestBody())
                return { HTTP_BADREQUEST, "Invalid Parameters" };
            try {
                auto j = nlohmann::json::parse(g->GetRequestBody().value());

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
                    return { HTTP_INTERNAL, "Databse Exception" };
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
                                return { HTTP_INTERNAL, "Databse Exception" };
                            }
                        }
                    }
                }

                if (uid) {
                    auto token = jwt::create<jwt::traits::nlohmann_json>()
                                     .set_expires_at(jwt::date::clock::now() + std::chrono::minutes(MYARGS.TokenExpire.value_or(5)))
                                     .set_type("JWS")
                                     .set_payload_claim("uid", uid.value())
                                     .set_payload_claim("type", j["type"].get<std::string>())
                                     .sign(jwt::algorithm::hs256 { MYARGS.ApiKey.value_or("") });
                    nlohmann::json js;
                    js["token"] = token;
                    js["type"] = j["type"].get<std::string>();
                    return { HTTP_OK, js.dump() };
                }
            } catch (const std::exception& e) {
                CERROR("CTX:%s /game/v1/login exception %s", MYARGS.CTXID.c_str(), e.what());
            }
            return { HTTP_BADREQUEST, "Invalid Parameters" };
        });
    hs->Register(
        "/game/v1/getuser",
        EVHTTP_REQ_POST,
        [](const ghttp::CGlobalData* g) -> std::pair<uint32_t, std::string> {
            try {
                auto conn = CMongo::Instance().Conn();
                mongocxx::v_noabi::collection col = (*conn)[MYARGS.DbName[0]][MYARGS.DbColl[0][0]];
                auto query = make_document(kvp("_id", bsoncxx::types::b_int64 { g->GetUid().value_or(0) }));

                auto result = col.find_one(query.view());
                nlohmann::json j;
                if (result) {
                    bsoncxx::document::view doc = result->view();
                    if (auto it = doc.find("value"); it != doc.end()) {
                        j["value"] = it->get_utf8();
                    }
                }
                return { HTTP_OK, j.dump() };
            } catch (const std::exception& e) {
                CERROR("CTX:%s /game/v1/getuser mongo exception %s", MYARGS.CTXID.c_str(), e.what());
                return { HTTP_INTERNAL, "Database Exception" };
            }
        });
    hs->Register(
        "/game/v1/setuser",
        EVHTTP_REQ_POST,
        [](const ghttp::CGlobalData* g) -> std::pair<uint32_t, std::string> {
            if (!g->GetRequestBody())
                return { HTTP_BADREQUEST, "Invalid Parameters" };
            try {
                auto j = nlohmann::json::parse(g->GetRequestBody().value());
                auto conn = CMongo::Instance().Conn();
                mongocxx::v_noabi::collection col = (*conn)[MYARGS.DbName[0]][MYARGS.DbColl[0][0]];
                auto query = make_document(kvp("_id", bsoncxx::types::b_int64 { g->GetUid().value_or(0) }));
                auto update = make_document(kvp("$set", make_document(kvp("value", bsoncxx::types::b_utf8 { j["value"].get<std::string>() }))));

                col.update_one(query.view(), update.view());
            } catch (const std::exception& e) {
                CERROR("CTX:%s /game/v1/setuser mongo exception %s", MYARGS.CTXID.c_str(), e.what());
                return { HTTP_INTERNAL, "Database Exception" };
            }
            nlohmann::json js;
            // CRedisMgr::Instance().Handle(0)->set("example", j.dump());
            return { HTTP_OK, js.dump() };
        });
}
