#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Returns a list of YouTube video IDs that match the query since the given ISO8601 timestamp (UTC, with 'Z').
std::vector<std::string> search_video_ids(const std::string& query,
                                          const std::string& published_after_iso,
                                          int max_results,
                                          const std::string& api_key);

// Returns minimal comment objects for a given video ID (up to max_results).
// Each item should include: commentId, text, author, likeCount, publishedAt.
std::vector<nlohmann::json> fetch_comments(const std::string& video_id,
                                           const std::string& api_key,
                                           int max_results);
