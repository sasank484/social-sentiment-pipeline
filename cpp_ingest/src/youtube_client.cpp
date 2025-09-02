#include "youtube_client.hpp"
#include "http_client.hpp"
#include "youtube_errors.hpp"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>

static bool should_retry_yt_error(YTErrorReason reason) {
    return reason == YTErrorReason::RateLimitExceeded ||
           reason == YTErrorReason::UserRateLimitExceeded ||
           reason == YTErrorReason::QuotaExceeded ||
           reason == YTErrorReason::DailyLimitExceeded;
}

static std::string url_encode(const std::string& s) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (auto c : s) {
        if (isalnum((unsigned char)c) || c=='-' || c=='_' || c=='.' || c=='~') {
            escaped << c;
        } else if (c == ' ') {
            escaped << "%20";
        } else {
            escaped << '%' << std::uppercase << std::setw(2) << int((unsigned char) c) << std::nouppercase;
        }
    }
    return escaped.str();
}

std::vector<std::string> search_video_ids(const std::string& query,
                                          const std::string& published_after_iso,
                                          int max_results,
                                          const std::string& api_key) {
    std::vector<std::string> ids;
    if (max_results <= 0) return ids;

    const int page_size = 50;
    std::string nextPageToken;

    while ((int)ids.size() < max_results) {
        int remaining = max_results - (int)ids.size();
        int req_max = remaining < page_size ? remaining : page_size;

        std::string url = "https://www.googleapis.com/youtube/v3/search"
                          "?part=snippet"
                          "&type=video"
                          "&order=date"
                          "&maxResults=" + std::to_string(req_max) +
                          "&q=" + url_encode(query) +
                          "&publishedAfter=" + url_encode(published_after_iso) +
                          "&key=" + url_encode(api_key);
        if (!nextPageToken.empty()) {
            url += "&pageToken=" + url_encode(nextPageToken);
        }

        std::string body;
        int attempt = 0;
        const int max_attempts = 5;
        while (attempt < max_attempts) {
            try {
                body = get_json(url);
                break;
            } catch (const std::exception& ex) {
                std::string msg;
                auto reason = parse_youtube_error_reason(ex.what(), &msg);
                if (should_retry_yt_error(reason)) {
                    int delay = (1 << attempt) * 1000; // exponential backoff in ms
                    spdlog::warn("YouTube API rate/quota error ({}): {}. Retrying in {} ms", static_cast<int>(reason), msg, delay);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    attempt++;
                } else {
                    spdlog::error("YouTube API error ({}): {}", static_cast<int>(reason), msg);
                    break;
                }
            }
        }
        if (body.empty()) break;
        auto j = nlohmann::json::parse(body, nullptr, false);
        if (j.is_discarded()) {
            spdlog::warn("search_video_ids: JSON parse failed");
            break;
        }

        if (j.contains("items") && j["items"].is_array()) {
            for (const auto& it : j["items"]) {
                try {
                    auto vid = it.at("id").at("videoId").get<std::string>();
                    ids.push_back(vid);
                } catch (...) { /* skip */ }
            }
        }
        if (j.contains("nextPageToken")) {
            nextPageToken = j["nextPageToken"].get<std::string>();
        } else {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // gentle rate-limit
    }
    spdlog::info("search_video_ids: fetched {} ids", ids.size());
    return ids;
}

std::vector<nlohmann::json> fetch_comments(const std::string& video_id,
                                           const std::string& api_key,
                                           int max_results) {
    std::vector<nlohmann::json> out;
    if (max_results <= 0) return out;

    const int page_size = 100;
    std::string nextPageToken;

    while ((int)out.size() < max_results) {
        int remaining = max_results - (int)out.size();
        int req_max = remaining < page_size ? remaining : page_size;

        std::string url = "https://www.googleapis.com/youtube/v3/commentThreads"
                          "?part=snippet"
                          "&order=time"
                          "&maxResults=" + std::to_string(req_max) +
                          "&videoId=" + url_encode(video_id) +
                          "&key=" + url_encode(api_key);
        if (!nextPageToken.empty()) {
            url += "&pageToken=" + url_encode(nextPageToken);
        }

        std::string body;
        int attempt = 0;
        const int max_attempts = 5;
        while (attempt < max_attempts) {
            try {
                body = get_json(url);
                break;
            } catch (const std::exception& ex) {
                std::string msg;
                auto reason = parse_youtube_error_reason(ex.what(), &msg);
                if (should_retry_yt_error(reason)) {
                    int delay = (1 << attempt) * 1000; // exponential backoff in ms
                    spdlog::warn("YouTube API rate/quota error ({}): {}. Retrying in {} ms", static_cast<int>(reason), msg, delay);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                    attempt++;
                } else {
                    spdlog::error("YouTube API error ({}): {}", static_cast<int>(reason), msg);
                    break;
                }
            }
        }
        if (body.empty()) break;
        auto j = nlohmann::json::parse(body, nullptr, false);
        if (j.is_discarded()) {
            spdlog::warn("fetch_comments: JSON parse failed");
            break;
        }

        if (j.contains("items") && j["items"].is_array()) {
            for (const auto& it : j["items"]) {
                try {
                    const auto& s = it.at("snippet").at("topLevelComment").at("snippet");
                    nlohmann::json row;
                    row["commentId"]   = it.at("snippet").at("topLevelComment").at("id").get<std::string>();
                    row["text"]        = s.value("textOriginal", "");
                    row["author"]      = s.value("authorDisplayName", "");
                    row["likeCount"]   = s.value("likeCount", 0);
                    row["publishedAt"] = s.value("publishedAt", "");
                    out.push_back(row);
                } catch (...) { /* skip */ }
            }
        }
        if (j.contains("nextPageToken")) {
            nextPageToken = j["nextPageToken"].get<std::string>();
        } else {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // gentle rate-limit
    }
    spdlog::info("fetch_comments: fetched {} comments for video {}", out.size(), video_id);
    return out;
}

