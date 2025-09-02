#pragma once
#include <string>
#include <nlohmann/json.hpp>

enum class YTErrorReason {
None,
RateLimitExceeded, // reason: rateLimitExceeded
UserRateLimitExceeded, // reason: userRateLimitExceeded
QuotaExceeded, // reason: quotaExceeded
DailyLimitExceeded, // reason: dailyLimitExceeded
Forbidden, // generic 403 without specific reason
Unknown
};

// Try to extract the primary error reason from a YouTube error payload.
YTErrorReason parse_youtube_error_reason(const std::string& body, std::string* message_out = nullptr);
