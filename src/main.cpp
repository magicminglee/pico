#include "gamelibs/app.hpp"

#include "jwt-cpp/jwt.h"
#include "jwt-cpp/traits/nlohmann-json/traits.h"

USE_NAMESPACE_FRAMEWORK

int main(int argc, char** argv)
{
    CheckCondition(CApp::Start(argc, argv), EXIT_FAILURE);
    return EXIT_SUCCESS;
}

void CApp::Register(std::shared_ptr<CHTTPServer> hs)
{
    hs->RegEvent("start",
        [](const CHTTPServer::RequestData& rd) -> std::optional<std::pair<uint32_t, std::string>> {
            CINFO("%s", rd.path.value_or("").data());
            if (rd.path && rd.path.value() == "/game/v1/login") {
                return std::nullopt;
            }
            auto auth = CHTTPServer::GetValueByKey(rd.headers, "Authorization");
            if (!auth)
                return std::optional<std::pair<uint32_t, std::string>>({ HTTP_UNAUTHORIZED, "" });
            try {
                auto res = CStringTool::Split(auth.value(), " ");
                if (2 == res.size()) {
                    const auto decoded = jwt::decode<jwt::traits::nlohmann_json>(res[1].data());
                    auto verifier = jwt::verify<jwt::traits::nlohmann_json>()
                                        .allow_algorithm(jwt::algorithm::hs256 { MYARGS.ApiKey.value_or("") });
                    verifier.verify(decoded);
                    return std::nullopt;
                }
            } catch (const std::exception& e) {
                CERROR("CTX:%s %s jwt verify exception %s", MYARGS.CTXID.c_str(), __FUNCTION__, e.what());
            }
            return std::optional<std::pair<uint32_t, std::string>>({ HTTP_UNAUTHORIZED, "" });
        });
    hs->Register(
        "/game/v1/login",
        {
            .cmd = EVHTTP_REQ_POST,
            .cb = [](evkeyvalq* qheaders, evkeyvalq* headers, std::string idata) -> std::pair<uint32_t, std::string> {
                CINFO("CTX:%s request /game/v1/login body %s", MYARGS.CTXID.c_str(), idata.c_str());
                if (!idata.empty()) {
                    try {
                        auto j = nlohmann::json::parse(idata);
                        std::string token = jwt::create<jwt::traits::nlohmann_json>()
                                                .set_expires_at(jwt::date::clock::now() + std::chrono::hours(24))
                                                .set_type("JWS")
                                                .set_payload_claim("account", j["account"].get<std::string>())
                                                .set_payload_claim("passwd", j["passwd"].get<std::string>())
                                                .set_payload_claim("type", j["type"].get<std::string>())
                                                .sign(jwt::algorithm::hs256 { MYARGS.ApiKey.value_or("") });
                        return { HTTP_OK, token };
                    } catch (const nlohmann::json::exception& e) {
                        CERROR("CTX:%s %s json exception %s", MYARGS.CTXID.c_str(), __FUNCTION__, e.what());
                    }
                }
                return { HTTP_BADREQUEST, "" };
            },
        });
    hs->Register(
        "/game/v1/getuser",
        {
            .cmd = EVHTTP_REQ_GET | EVHTTP_REQ_POST,
            .cb = [](evkeyvalq* qheaders, evkeyvalq* headers, std::string idata) -> std::pair<uint32_t, std::string> {
                CINFO("CTX:%s request /game/v1/example body %s", MYARGS.CTXID.c_str(), idata.c_str());
                if (!idata.empty()) {
                    try {
                        auto j = nlohmann::json::parse(idata);
                    } catch (const nlohmann::json::exception& e) {
                        CERROR("CTX:%s %s json exception %s", MYARGS.CTXID.c_str(), __FUNCTION__, e.what());
                    }
                }
                nlohmann::json j;
                auto a = nlohmann::json::array();
                auto v = (*MongoCtx)->list_database_names();
                for (auto& vv : v) {
                    a.push_back(vv);
                }
                j["example"] = a;

                CRedisMgr::Instance().Handle(0)->set("example", j.dump());
                return { HTTP_OK, j.dump() };
            },
        });
}
