#include "clickhouse.hpp"

#include "framework/argument.hpp"
#include "framework/contex.hpp"
#include "framework/stringtool.hpp"
#include "framework/utils.hpp"
#include "framework/xlog.hpp"

USE_NAMESPACE_FRAMEWORK
using namespace clickhouse;

namespace gamelibs {
namespace ch {
    void CClickHouseMgr::registry()
    {
        m_funcs["payevent"] = [](clickhouse::Client* c, const std::vector<std::string>& data) {
            Block b;
            auto uid = std::make_shared<ColumnInt64>();
            auto paytype = std::make_shared<ColumnEnum8>(Type::CreateEnum8({ { "Google", 1 }, { "IOS", 2 }, { "IOSSandbox", 3 } }));
            auto orderid = std::make_shared<ColumnString>();
            auto productid = std::make_shared<ColumnString>();
            auto quantity = std::make_shared<ColumnInt32>();
            auto balance = std::make_shared<ColumnInt64>();
            auto ip = std::make_shared<ColumnIPv4>();

            try {
                for (auto& v : data) {
                    try {
                        auto j = nlohmann::json::parse(v);
                        auto juid = j["uid"].get<int64_t>();
                        auto jpaytype = j["paytype"].get<int8_t>();
                        auto jorderid = j["orderid"].get<std::string>();
                        auto jprodid = j["productid"].get<std::string>();
                        auto jquantity = j["quantity"].get<int32_t>();
                        auto jbalance = j["balance"].get<int64_t>();
                        auto jip = j["ip"].get<std::string>();
                        uid->Append(juid);
                        paytype->Append(jpaytype, true);
                        orderid->Append(jorderid);
                        productid->Append(jprodid);
                        quantity->Append(jquantity);
                        balance->Append(jbalance);
                        ip->Append(jip);
                    } catch (const std::exception& e) {
                        SPDLOG_ERROR("payevent json parse exception {}", e.what());
                        continue;
                    }
                }
                b.AppendColumn("uid", uid);
                b.AppendColumn("paytype", paytype);
                b.AppendColumn("orderid", orderid);
                b.AppendColumn("productid", productid);
                b.AppendColumn("quantity", quantity);
                b.AppendColumn("balance", balance);
                b.AppendColumn("ip", ip);
            } catch (const std::exception& e) {
                SPDLOG_ERROR("payevent exception {}", e.what());
                return;
            }
            try {
                c->Insert("payevent", b);
            } catch (const std::exception& e) {
                SPDLOG_ERROR("{} exception {}", __FUNCTION__, e.what());
                c->ResetConnection();
            }
        };
    }

    bool CClickHouseMgr::Init()
    {
        try {
            auto [scheme, user, passwd, host, port, db] = CUtils::ParseDBURL(MYARGS.ClickhouseUrl.value());
            auto opt = clickhouse::ClientOptions()
                           .SetUser(user)
                           .SetPassword(passwd)
                           .SetHost(host)
                           .SetPort(CStringTool::ToInteger<uint16_t>(port))
                           .SetDefaultDatabase(db)
                           .SetPingBeforeQuery(true);
            m_ch_client.reset(CNEW clickhouse::Client(opt));
        } catch (const std::exception& e) {
            SPDLOG_ERROR("connect clickhouse exception {}", e.what());
            return false;
        }
        registry();
        CContex::MAIN_CONTEX->AddPersistEvent(101, 10000, [this]() { this->Consume(); });
        return true;
    }

    void CClickHouseMgr::Push(const std::string name, const std::string& data)
    {
        m_queue[name].push_back(data);
    }

    void CClickHouseMgr::Consume()
    {
        for (auto& [k, v] : m_queue) {
            if (auto it = m_funcs.find(k); it != std::end(m_funcs)) {
                it->second(Handle(), v);
            }
        }
        m_queue.clear();
    }

    clickhouse::Client* CClickHouseMgr::Handle()
    {
        return m_ch_client.get();
    }
}
} // end namespace clickhouse