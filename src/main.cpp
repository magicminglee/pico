#include "gamelibs/app.hpp"

USE_NAMESPACE_FRAMEWORK

void CApp::Register(std::shared_ptr<CHTTPServer> ref)
{
    ref->JsonRegister("/game/v1/example", EVHTTP_REQ_GET | EVHTTP_REQ_POST, [](evkeyvalq* qheaders, evkeyvalq* headers, std::string idata) -> std::string {
        if (auto v = CHTTPServer::GetValueByKey(qheaders, "userid"); v) {
            CINFO("userid:%s", v.value());
        }
        if (auto v = CHTTPServer::GetValueByKey(headers, "ACCEPT-ENCODING"); v) {
            CINFO("accept-encoding:%s", v.value());
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
    });
}

int main(int argc, char** argv)
{
    CheckCondition(MYARGS.ParseArg(argc, argv, "Pico", "Pico Program"), EXIT_FAILURE);
    CApp::Start();
    return EXIT_SUCCESS;
}
