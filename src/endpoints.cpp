#include "../include/endpoints.hpp"

returnType Register(CppHttp::Net::Request req) {
    soci::session* sql = Database::GetInstance()->GetSession();

    json body;
    try {
        body = json::parse(req.m_info.body);
    }
    catch (json::parse_error& e) {
        return {CppHttp::Net::ResponseType::BAD_REQUEST, e.what(), {}};
    }

    std::string email = body["email"];
    std::string password = body["password"];
    std::string firstName = body["first_name"];
    std::string lastName = body["last_name"];

    if (email.empty() || password.empty() || firstName.empty() || lastName.empty()) {
		return {CppHttp::Net::ResponseType::BAD_REQUEST, "Missing required fields", {}};
	}

    {
        std::lock_guard<std::mutex> lock(Database::dbMutex);
        *sql << "SELECT * FROM users WHERE email = :email", soci::use(email);

        if (sql->got_data()) {
		    return { CppHttp::Net::ResponseType::BAD_REQUEST, "Email already in use", {} };
	    }
    }

    if (email.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@.") != std::string::npos) {
        return { CppHttp::Net::ResponseType::BAD_REQUEST, "Invalid email", {} };
    }

    if (email.find_first_of("@") == std::string::npos) {
        return { CppHttp::Net::ResponseType::BAD_REQUEST, "Invalid email", {} };
    }

    if (email.find_first_of(".") == std::string::npos) {
        return { CppHttp::Net::ResponseType::BAD_REQUEST, "Invalid email", {} };
    }

    if (password.length() < 8) {
        return { CppHttp::Net::ResponseType::BAD_REQUEST, "Password must be at least 8 characters", {} };
    }

    if (password.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%^&*") != std::string::npos) {
        return { CppHttp::Net::ResponseType::BAD_REQUEST, "Password must only contain alphanumeric characters", {} };
    }

    std::string salt = RandomCode(12);

    std::string hashedSalted = Hash(password + salt);

    User user;
    {
        std::lock_guard<std::mutex> lock(Database::dbMutex);
        *sql << "INSERT INTO users (id, email, password, salt, first_name, last_name, role) VALUES (DEFAULT, :email, :password, :salt, :first_name, :last_name, DEFAULT) RETURNING *", soci::use(email), soci::use(hashedSalted), soci::use(salt), soci::use(firstName), soci::use(lastName), soci::into(user);
    }

    json response;
    response["id"] = user.id;
    response["email"] = user.email;
    response["firstName"] = user.firstName;
    response["lastName"] = user.lastName;
    response["role"] = user.role;

    return {CppHttp::Net::ResponseType::JSON, response.dump(4), {}};
}

returnType Login(CppHttp::Net::Request req) {
    soci::session* sql = Database::GetInstance()->GetSession();

    json body;

    try {
        body = json::parse(req.m_info.body);
    }
    catch (json::parse_error& e) {
        return { CppHttp::Net::ResponseType::BAD_REQUEST, e.what(), {} };
    }

    std::string email = body["email"];
    std::string password = body["password"];

    if (email.empty() || password.empty()) {
        return { CppHttp::Net::ResponseType::BAD_REQUEST, "Missing required fields", {} };
    }

    User user;
    {
        std::lock_guard<std::mutex> lock(Database::dbMutex);
        *sql << "SELECT * FROM users WHERE email = :email", soci::use(email), soci::into(user);
        
        if (!sql->got_data()) {
            return { CppHttp::Net::ResponseType::BAD_REQUEST, "User with email " + email + " not found", {}};
        }
    }


    std::string hashedSalted = Hash(password + user.salt);

    if (hashedSalted != user.password) {
		return { CppHttp::Net::ResponseType::BAD_REQUEST, "Invalid password", {} };
	}

    auto jwtToken = jwt::create<jwt::traits::nlohmann_json>()
        .set_issuer("auth0")
        .set_type("JWT")
        .set_payload_claim("id", std::to_string(user.id))
        .set_payload_claim("email", email)
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{ 24 })
        .set_issued_at(std::chrono::system_clock::now());

    std::string rsaSecret = std::getenv("RSASECRET");

    size_t pos = 0;

    while ((pos = rsaSecret.find("\\n", pos)) != std::string::npos) {
        rsaSecret.replace(pos, 2, "\n");
    }

    std::cout << rsaSecret << std::endl;

    std::string signedToken = jwtToken.sign(jwt::algorithm::rs512{ "", rsaSecret, "", ""});

    json response;
    response["token"] = signedToken;

    return {CppHttp::Net::ResponseType::JSON, response.dump(4), {}};
}