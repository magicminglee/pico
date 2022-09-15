#pragma once
#include "framework/singleton.hpp"

#include "clickhouse/client.h"

USE_NAMESPACE_FRAMEWORK
namespace gamelibs {
namespace ch {
    class CClickHouseMgr : public CTLSingleton<CClickHouseMgr> {
        using KQueue = std::vector<std::string>;
        using KMap = std::unordered_map<std::string, KQueue>;
        using EventFunc = std::map<std::string, std::function<void(clickhouse::Client*, const std::vector<std::string>&)>>;

    public:
        CClickHouseMgr() = default;
        ~CClickHouseMgr() = default;
        bool Init();
        clickhouse::Client* Handle();
        void Push(const std::string name, const std::string& data);
        void Consume();

    private:
        void registry();

    private:
        std::unique_ptr<clickhouse::Client> m_ch_client;
        KMap m_queue;
        EventFunc m_funcs;
    };
}
} // end namespace clickhouse