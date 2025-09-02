#include "youtube_errors.hpp"
#include <nlohmann/json.hpp>

YTErrorReason parse_youtube_error_reason(const std::string& body, std::string* message_out) {
auto j = nlohmann::json::parse(body, nullptr, false);
if (j.is_discarded()) return YTErrorReason::Unknown;
try {
if (!j.contains("error")) return YTErrorReason::Unknown;
const auto& e = j.at("error");
if (e.contains("message") && message_out) *message_out = e.value("message", "");
if (e.contains("errors") && e["errors"].is_array() && !e["errors"].empty()) {
const auto& first = e["errors"][0];
std::string reason = first.value("reason", "");
if (reason == "rateLimitExceeded") return YTErrorReason::RateLimitExceeded;
if (reason == "userRateLimitExceeded") return YTErrorReason::UserRateLimitExceeded;
if (reason == "quotaExceeded") return YTErrorReason::QuotaExceeded;
if (reason == "dailyLimitExceeded") return YTErrorReason::DailyLimitExceeded;
if (e.value("code", 0) == 403) return YTErrorReason::Forbidden;
}
if (e.value("code", 0) == 403) return YTErrorReason::Forbidden;
} catch (...) {}
return YTErrorReason::Unknown;
}
