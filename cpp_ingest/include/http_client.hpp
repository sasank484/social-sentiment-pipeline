
#pragma once
#include <string>
#include <vector>

std::string get_json(const std::string& url, const std::vector<std::string>& headers = {}, int max_retries = 5);
