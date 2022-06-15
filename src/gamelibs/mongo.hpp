#pragma once
#include "framework/singleton.hpp"
#include "framework/xlog.hpp"

#include "bsoncxx/builder/basic/array.hpp"
#include "bsoncxx/builder/basic/document.hpp"
#include "bsoncxx/builder/basic/kvp.hpp"
#include "bsoncxx/builder/stream/array.hpp"
#include "bsoncxx/builder/stream/document.hpp"
#include "bsoncxx/builder/stream/helpers.hpp"
#include "bsoncxx/json.hpp"
#include "bsoncxx/stdx/make_unique.hpp"
#include "bsoncxx/stdx/optional.hpp"
#include "bsoncxx/stdx/string_view.hpp"
#include "bsoncxx/types.hpp"
#include "mongoc/mongoc.h"

#include "mongocxx/client.hpp"
#include "mongocxx/instance.hpp"
#include "mongocxx/logger.hpp"
#include "mongocxx/options/find.hpp"
#include "mongocxx/pool.hpp"
#include "mongocxx/result/update.hpp"
#include "mongocxx/stdx.hpp"
#include "mongocxx/uri.hpp"

using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::sub_array;
using bsoncxx::builder::basic::sub_document;
USE_NAMESPACE_FRAMEWORK

namespace gamelibs {
namespace mongo {
    // mongodb connection pool encapsulated
    using connection = mongocxx::pool::entry;
    class CMongo : public CSingleton<CMongo> {
    public:
        CMongo();
        ~CMongo();
        bool Init(const std::string_view uri);
        connection Conn()
        {
            return m_pool->acquire();
        }

    private:
        mongocxx::instance m_inst;
        mongocxx::options::client m_client_opts;
        std::unique_ptr<mongocxx::pool> m_pool = nullptr;
    };
}
} // end namespace mongo