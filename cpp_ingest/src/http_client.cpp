
#include "http_client.hpp"
#include <curl/curl.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <random>
#include <stdexcept>

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string get_json(const std::string& url, const std::vector<std::string>& headers, int max_retries) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    struct curl_slist* header_list = nullptr;

    for (const auto& h : headers) {
        header_list = curl_slist_append(header_list, h.c_str());
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> jitter_dist(100, 500);

    int attempt = 0;
    while (attempt < max_retries) {
        attempt++;
        curl = curl_easy_init();
        if (curl) {
            readBuffer.clear();
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

            res = curl_easy_perform(curl);
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            curl_easy_cleanup(curl);

            if (res == CURLE_OK && http_code >= 200 && http_code < 300) {
                if (header_list) curl_slist_free_all(header_list);
                return readBuffer;
            } else {
                spdlog::warn("HTTP attempt {} failed: {} (HTTP {}) body={}", attempt, curl_easy_strerror(res), http_code, readBuffer);
                // If it's a hard 403 or 404, don't keep retrying forever; break early
                if (http_code == 403 || http_code == 404) break;
                int backoff = (1 << attempt) * 500 + jitter_dist(gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(backoff));
            }
        }
    }
    if (header_list) curl_slist_free_all(header_list);
    throw std::runtime_error("GET failed: " + url);
}
