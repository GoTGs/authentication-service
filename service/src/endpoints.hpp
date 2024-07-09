#include <CppHttp.hpp>
#include "dependencies/cpphttp/include/nlohmann/json.hpp"
#include "database.hpp"
#include <iostream>
#include <string>
#include <tuple>
#include <optional>
#include <vector>
#include <fstream>
#include <unordered_map>

using returnType = std::tuple<CppHttp::Net::ResponseType, std::string, std::optional<std::vector<std::string>>>;
using json = nlohmann::json;

returnType Index(CppHttp::Net::Request req) {
    return {CppHttp::Net::ResponseType::OK, "", {}};
}

returnType Register(CppHttp::Net::Request req) {
    soci::session* sql = Database::GetInstance()->GetSession();

    json body;
    try {
        body = json::parse(req.m_info.body);
    }
    catch (json::parse_error& e) {
        return {CppHttp::Net::ResponseType::BAD_REQUEST, e.what(), {}};
    }

    return {CppHttp::Net::ResponseType::JSON, "", {}};
}