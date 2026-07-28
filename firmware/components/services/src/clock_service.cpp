#include "services/clock_service.hpp"

namespace nova {
namespace services {

ClockService::ClockService(core::StateStore& store, board::IBoard& board, Config config)
    : store_(store), board_(board), config_(config) {}

utils::Status ClockService::start() {
    if (!has_valid_offset()) {
        return utils::Status::kInvalidArg;
    }

    const uint64_t rtc_time_s = board_.rtc_unix_time_s();
    if (!is_plausible_utc(rtc_time_s)) {
        // O estado inicial ja e indisponivel. Nao ha erro de boot: o painel
        // segue operacional e a sincronizacao NTP futura podera corrigi-lo.
        return utils::Status::kOk;
    }
    publish_clock(rtc_time_s, 0, models::ClockSource::kRtc);
    return utils::Status::kOk;
}

void ClockService::tick(uint64_t now_ms) {
    if (!has_time_ || now_ms < base_monotonic_ms_) {
        return;
    }
    const uint64_t utc_time_s = base_utc_time_s_ + (now_ms - base_monotonic_ms_) / 1000ULL;
    const uint64_t minute = utc_time_s / 60ULL;
    if (minute != last_published_minute_) {
        publish_clock(utc_time_s, now_ms, source_);
    }
}

utils::Status ClockService::accept_ntp_time(models::UtcTime utc_time, uint64_t now_ms) {
    if (!is_plausible_utc(utc_time.unix_time_s)) {
        return utils::Status::kStale;
    }
    publish_clock(utc_time.unix_time_s, now_ms, models::ClockSource::kNtp);
    return utils::Status::kOk;
}

bool ClockService::is_plausible_utc(uint64_t unix_time_s) {
    return unix_time_s >= kMinPlausibleUtcS && unix_time_s < kMaxPlausibleUtcS;
}

bool ClockService::has_valid_offset() const {
    constexpr int16_t kMinOffsetMinutes = -14 * 60;
    constexpr int16_t kMaxOffsetMinutes = 14 * 60;
    return config_.utc_offset_minutes >= kMinOffsetMinutes &&
           config_.utc_offset_minutes <= kMaxOffsetMinutes;
}

void ClockService::publish_clock(uint64_t utc_time_s, uint64_t now_ms,
                                 models::ClockSource source) {
    const int64_t local_time_s = static_cast<int64_t>(utc_time_s) +
                                 static_cast<int64_t>(config_.utc_offset_minutes) * 60;
    const int64_t local_minute = local_time_s / 60;
    const int64_t minute_of_day = ((local_minute % (24 * 60)) + (24 * 60)) % (24 * 60);

    models::ClockState clock;
    clock.last_update_ms_ = now_ms;
    clock.hour_ = static_cast<uint8_t>(minute_of_day / 60);
    clock.minute_ = static_cast<uint8_t>(minute_of_day % 60);
    clock.source_ = source;
    clock.valid_ = true;
    clock.stale_ = false;
    store_.set_clock(clock);

    base_utc_time_s_ = utc_time_s;
    base_monotonic_ms_ = now_ms;
    last_published_minute_ = utc_time_s / 60ULL;
    source_ = source;
    has_time_ = true;
}

}  // namespace services
}  // namespace nova
