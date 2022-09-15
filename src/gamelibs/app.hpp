#pragma once
#include "framework/argument.hpp"
#include "framework/chrono.hpp"
#include "framework/common.hpp"
#include "framework/connhandler.hpp"
#include "framework/global.hpp"
#include "framework/httpmsg.hpp"
#include "framework/httpserver.hpp"
#include "framework/localstorage.hpp"
#include "framework/random.hpp"
#include "framework/service.hpp"
#include "framework/signal.hpp"
#include "framework/singleton.hpp"
#include "framework/ssl.hpp"
#include "framework/stringtool.hpp"
#include "framework/tcpserver.hpp"
#include "framework/udpserver.hpp"
#include "framework/utils.hpp"
#include "framework/worker.hpp"
#include "framework/xlog.hpp"

#include "clickhouse.hpp"
#include "lstate.hpp"
#include "luabind.hpp"
#include "mongo.hpp"
#include "redis.hpp"

#include "nlohmann/json.hpp"

NAMESPACE_FRAMEWORK_BEGIN

using namespace gamelibs::redis;
using namespace gamelibs::mongo;
using namespace gamelibs::lstate;
using namespace gamelibs::ch;
namespace fs = std::filesystem;

class CApp {
    using CommandSyncFunc = std::function<bool(const nlohmann::json&)>;

public:
    class AppWorker final : public CWorker {
        std::map<std::string, CommandSyncFunc> m_command_sync_cbs;

    public:
        void CallByCommand(const std::string cmd, const nlohmann::json& j)
        {
            if (auto it = m_command_sync_cbs.find(cmd); it != m_command_sync_cbs.end()) {
                it->second(j);
            }
        }
        void RegisterCommand(const std::string cmd, CommandSyncFunc cb)
        {
            m_command_sync_cbs[cmd] = cb;
        }

        bool LoadLuaFiles()
        {
            if (!CLuaState::MAIN_LUASTATE)
                CLuaState::MAIN_LUASTATE.reset(CNEW CLuaState());
            CheckCondition(CLuaState::MAIN_LUASTATE->Init(), false);
            luaopen_luabind(CLuaState::MAIN_LUASTATE->Handle());
            auto s = CChrono::NowMs();
            auto p = fs::path(MYARGS.ScriptDir.value()) / "init.lua";
            if (LUA_OK != luaL_loadfile(CLuaState::MAIN_LUASTATE->Handle(), p.c_str())) {
                SPDLOG_ERROR("loadfile failed");
                return false;
            }
            CLuaState::MAIN_LUASTATE->Call<void>(0, 0);
            SPDLOG_INFO("LoadLuaFiles luascripts elapse {} ms", CChrono::NowMs() - s);
            return true;
        }

        virtual bool Init() final
        {
            CheckCondition(CWorker::Init(), false);
            CheckCondition(LoadLuaFiles(), false);

            OnLeftEvent(
                [](std::string_view data) {
                    if (data == "stop") {
                        CContex::MAIN_CONTEX->Exit(0);
                    }
                },
                [this](std::string_view data) {
                    try {
                        auto j = nlohmann::json::parse(data);
                        if (j["cmd"].is_string() && j["cmd"].get_ref<const std::string&>() == "newconn") {
                            auto schema = j["schema"].get<std::string>();
                            auto host = j["host"].get<std::string>();
                            auto fd = j["fd"].get<int32_t>();
                            auto server = j["server"].get<uintptr_t>();
                            if (schema == "http" || schema == "https") {
                                ((CHTTPServer*)server)->OnConnected(host, fd);
                            } else if (schema == "tcp") {
                                // CApp::TcpRegister("", nullptr);
                            }
                        } else if (j["cmd"].is_string() && j["cmd"].get_ref<const std::string&>() == "reload") {
                            if (j.contains("ssl") && j["ssl"].is_array()) {
                                MYARGS.SslConf.clear();
                                for (auto& v : j["ssl"]) {
                                    CArgument::SSLConf conf;
                                    conf.servername = v["servername"];
                                    conf.cert = v["certificate"];
                                    conf.prikey = v["privatekey"];
                                    MYARGS.SslConf.push_back(std::move(conf));
                                }
                                if (!CSSLContex::Instance().Init()) {
                                    SPDLOG_ERROR("CTX:{} init ssl contex", MYARGS.CTXID);
                                }
                            }
                            if (j.contains("alloworigin") && j["alloworigin"].is_boolean())
                                MYARGS.IsAllowOrigin = j["alloworigin"].get<bool>();
                            if (j.contains("forcehttps") && j["forcehttps"].is_boolean())
                                MYARGS.IsForceHttps = j["forcehttps"].get<bool>();
                            if (j.contains("redirectstatus") && j["redirectstatus"].is_number_unsigned())
                                MYARGS.RedirectStatus = j["redirectstatus"].get<uint16_t>();
                            if (j.contains("redirecturl") && j["redirecturl"].is_string())
                                MYARGS.RedirectUrl = j["redirecturl"].get<std::string>();
                            if (j.contains("apikey") && j["apikey"].is_string())
                                MYARGS.ApiKey = j["apikey"].get<std::string>();
                            if (j.contains("tokenexpire") && j["tokenexpire"].is_number_unsigned())
                                MYARGS.TokenExpire = j["tokenexpire"].get<uint32_t>();
                            if (j.contains("httptimeout") && j["httptimeout"].is_number_unsigned())
                                MYARGS.HttpTimeout = j["httptimeout"].get<uint32_t>();
                            if (j.contains("redisttl") && j["redisttl"].is_number_unsigned())
                                MYARGS.RedisTTL = j["redisttl"].get<uint64_t>();
                            if (j.contains("http2able") && j["http2able"].is_boolean())
                                MYARGS.Http2Able = j["http2able"].get<bool>();
                            if (j.contains("confmap") && j["confmap"].is_object()) {
                                MYARGS.ConfMap = j["confmap"].get<std::map<std::string, std::string>>();
                            }
                            SPDLOG_INFO("CTX:{} reload config {}", MYARGS.CTXID, j.dump());
                            this->LoadLuaFiles();
                        } else if (j["cmd"].is_string()) {
                            this->CallByCommand(j["cmd"].get<std::string>(), j);
                        }
                    } catch (const nlohmann::json::exception& e) {
                        SPDLOG_ERROR("CTX:{} {} json {} exception {}", MYARGS.CTXID, __FUNCTION__, data, e.what());
                    }
                },
                nullptr);

            CApp::CommandRegister(this);
            if (!CApp::Init()) {
                SPDLOG_ERROR("CTX:{} CApp::Init fail", MYARGS.CTXID);
                return false;
            }
            if (!CRedisMgr::Instance().Init()) {
                SPDLOG_ERROR("CTX:{} CRedisMgr::Init fail", MYARGS.CTXID);
                return false;
            }
            if (!CClickHouseMgr::Instance().Init()) {
                SPDLOG_ERROR("CTX:{} CClickHouseMgr::Init fail", MYARGS.CTXID);
                return false;
            }

            return true;
        }
    };

    class AppService final : public CService {
        std::vector<CTCPServer*> m_tcp_server;

    public:
        virtual bool Init() final
        {
            CheckCondition(CService::Init(), false);
            CWorkerMgr::Instance().Register(
                []() {
                    return CNEW AppWorker();
                },
                nullptr,
                [](std::string_view data) {
                    CWorkerMgr::Instance().SendMsgToAllWorkers(CChannel::MsgType::Json, data);
                },
                nullptr);

            CheckCondition(CMongo::Instance().Init(MYARGS.MongoUrl.value()), false);

            for (auto& v : MYARGS.Workers.back().Host) {
                std::string schema, host, port, path;
                std::tie(schema, host, port, path) = CConnection::SplitUri(v);
                m_tcp_server.push_back(CNEW CHTTPServer());
                CHTTPServer* ref = (CHTTPServer*)m_tcp_server.back();
                ref->ListenAndServe(v, [schema, ref, v](const int32_t fd) {
                    nlohmann::json j;
                    j["cmd"] = "newconn";
                    j["schema"] = schema;
                    j["host"] = v;
                    j["fd"] = fd;
                    j["server"] = (uintptr_t)ref;
                    CWorkerMgr::Instance().SendMsgToOneWorker(CChannel::MsgType::Json, j.dump());
                });

                if (schema == "http" || schema == "https") {
                    CApp::WebRegister(ref);
                } else if (schema == "tcp") {
                    // CApp::TcpRegister(v, ref);
                }
            }
#ifdef PLATFORMOS
            SPDLOG_INFO("Service has been initiated on platfrom {}", PLATFORMOS);
#endif
            return true;
        }
        virtual void Start() final
        {
        }
    };

    static bool Start(int argc, char** argv)
    {
        CheckCondition(MYARGS.ParseArg(argc, argv, "Pico", "Pico Program"), false);
        CGlobal::InitGlobal();

        XLOG::LogInit();

        AppService s;
        s.Loop();
        SPDLOG_INFO("Service stop");
        return true;
    }

    static bool Init() { return true; }
    static void WebRegister(CHTTPServer* srv);
    static void TcpRegister(const std::string host, std::shared_ptr<CTCPServer> srv);
    static void CommandRegister(AppWorker* w);
};

NAMESPACE_FRAMEWORK_END