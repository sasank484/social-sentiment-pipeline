
#pragma once
#include <nlohmann/json.hpp>
#include <string>

void append_ndjson(const nlohmann::json& obj, const std::string& path);
