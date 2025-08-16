#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ------------------- HTTP POST JSON -------------------
json PostJson(const std::string& url, const json& data);

// ------------------- HTTP GET JSON -------------------
json GetJson(const std::string& url);

// ------------------- Callback libcurl -------------------
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
std::vector<unsigned char> PostRaw(const std::string& url, const nlohmann::json& jsonData);
