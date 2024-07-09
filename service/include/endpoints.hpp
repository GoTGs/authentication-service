#pragma once

#include "CppHttp.hpp"
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

returnType Index(CppHttp::Net::Request req);

returnType Register(CppHttp::Net::Request req);