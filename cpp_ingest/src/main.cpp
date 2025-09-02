#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "youtube_client.hpp"
#include "writer.hpp"
#include "youtube_errors.hpp"

// quick CSV split
static std::vector<std::string> split_csv(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ',') {
            if (!cur.empty()) { out.push_back(cur); cur.clear(); }
        } else if (!isspace((unsigned char)c)) {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

static std::string iso8601_utc_days_ago(int days) {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto past = now - hours(days * 24);
    std::time_t t = system_clock::to_time_t(past);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

int main(int argc, char** argv) {
    std::string brand = "verizon";
    std::string keywords_csv = "verizon,5g,coverage";
    int days = 14;
    int limit_videos = 20;
    std::string out_path = "data/raw/output.ndjson";

    // simple flag parser
    for (int i=1; i<argc; ++i) {
        std::string a = argv[i];
        auto next = [&](std::string& dst){ if (i+1<argc) dst = argv[++i]; };
        if (a == "--brand") next(brand);
        else if (a == "--keywords") next(keywords_csv);
        else if (a == "--days") { if (i+1<argc) days = std::stoi(argv[++i]); }
        else if (a == "--limit_videos") { if (i+1<argc) limit_videos = std::stoi(argv[++i]); }
        else if (a == "--out") next(out_path);
    }

    const char* key = std::getenv("YOUTUBE_API_KEY");
    if (!key || std::string(key).empty()) {
        spdlog::error("YOUTUBE_API_KEY env var is not set. Aborting.");
        return 1;
    }
    std::string api_key = key;

    auto keywords = split_csv(keywords_csv);
    // For MVP: join keywords into a single space-separated query
    std::string query;
    for (size_t i=0;i<keywords.size();++i) {
        if (i) query += " ";
        query += keywords[i];
    }

    std::string published_after = iso8601_utc_days_ago(days);
    spdlog::info("Searching videos for '{}', since {}", query, published_after);

    auto video_ids = search_video_ids(query, published_after, limit_videos, api_key);
    if (video_ids.empty()) {
        spdlog::warn("No videos found or API request failed. Exiting.");
        return 0;
    }

    int total_comments = 0;
    for (const auto& vid : video_ids) {
        auto comments = fetch_comments(vid, api_key, 500);
        if (comments.empty()) {
            spdlog::warn("No comments fetched for video {}. Possible API error or rate limit.", vid);
            continue;
        }
        for (const auto& c : comments) {
            nlohmann::json row;
            row["post_id"]      = std::string("YOUTUBE_") + c.value("commentId", "");
            row["source"]       = "youtube";
            row["brand"]        = brand;
            row["keywords"]     = keywords;
            row["text"]         = c.value("text", "");
            row["created_utc"]  = c.value("publishedAt", "");
            row["author"]       = c.value("author", "");
            row["like_count"]   = c.value("likeCount", 0);
            row["video_id"]     = vid;
            row["url"]          = std::string("https://www.youtube.com/watch?v=") + vid;

            // fetched_at = now UTC
            using namespace std::chrono;
            auto now = system_clock::now();
            std::time_t t = system_clock::to_time_t(now);
            std::tm tm{};
        #ifdef _WIN32
            gmtime_s(&tm, &t);
        #else
            gmtime_r(&t, &tm);
        #endif
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
            row["fetched_at"] = oss.str();

            append_ndjson(row, out_path);
            total_comments++;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    spdlog::info("Done. Wrote {} comments to {}", total_comments, out_path);
    if (total_comments == 0) {
        spdlog::warn("No comments were written. Check if API quota/rate limits caused data gaps.");
    }
    return 0;
}
