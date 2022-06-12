#include "gamelibs/app.hpp"

USE_NAMESPACE_FRAMEWORK

void CApp::Register(std::shared_ptr<CHTTPServer> hs)
{
    hs->Register(
        "/game/v1/example",
        {
            .cmd = EVHTTP_REQ_GET | EVHTTP_REQ_POST,
            .cb = [](evkeyvalq* qheaders, evkeyvalq* headers, std::string idata) -> std::string {
                CINFO("CTX:%s request /game/v1/example body %s", MYARGS.CTXID.c_str(), idata.c_str());
                if (auto v = CHTTPServer::GetValueByKey(qheaders, "userid"); v) {
                }
                if (auto v = CHTTPServer::GetValueByKey(headers, "ACCEPT-ENCODING"); v) {
                }
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
                return j.dump();
            },
        });
}

int main(int argc, char** argv)
{
    CheckCondition(CApp::Start(argc, argv), EXIT_FAILURE);
    return EXIT_SUCCESS;
}
