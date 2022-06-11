#include "framework/argument.hpp"

#include "mongo.hpp"

USE_NAMESPACE_FRAMEWORK

namespace gamelibs {
namespace mongo {
    // mongodbcxx driver logger delegate
    class MongoLogger : public mongocxx::v_noabi::logger {
    public:
        virtual void operator()(mongocxx::v_noabi::log_level level, bsoncxx::stdx::string_view domain, bsoncxx::stdx::string_view message) noexcept final
        {
            CINFO("MONGOLOGGER: %s,%s,%s", mongocxx::v_noabi::to_string(level).data(), domain.data(), message.data());
        }
    };

    CMongo::CMongo()
        : m_inst(mongocxx::stdx::make_unique<MongoLogger>())
    {
    }

    CMongo::~CMongo()
    {
    }

    bool CMongo::Init(const std::string_view uri)
    {
        if (m_pool)
            return true;
        mongocxx::uri muri(uri);
        mongocxx::options::apm apm_opts;
        apm_opts.on_command_started([&](const mongocxx::events::command_started_event& event) {
            // CINFO("CTX:%s on_command_started operationid:%ld requestid:%ld command:%s", MYARGS.CTXID.c_str(), event.operation_id(), event.request_id(), bsoncxx::to_json(event.command()).c_str());
        });
        apm_opts.on_command_succeeded([&](const mongocxx::events::command_succeeded_event& event) {
            // CINFO("CTX:%s on_command_succeeded operationid:%ld requestid:%ld duration:%ld us", MYARGS.CTXID.c_str(), event.operation_id(), event.request_id(), event.duration());
        });
        apm_opts.on_command_failed([&](const mongocxx::events::command_failed_event& event) {
            CINFO("CTX:%s on_command_failed operationid:%ld requestid:%ld duration:%ld failure:%s command:%s", MYARGS.CTXID.c_str(), event.operation_id(), event.request_id(), event.duration(), event.failure().data(), event.command_name().data());
        });
        m_client_opts.apm_opts(apm_opts);
        m_pool = mongocxx::stdx::make_unique<mongocxx::pool>(std::move(muri), m_client_opts);
        return m_pool != nullptr;
    }
}
} // end namespace mongo