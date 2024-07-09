#include "../include/endpoints.hpp"

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