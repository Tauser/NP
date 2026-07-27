#include "providers/mock_time_provider.hpp"

namespace nova {
namespace providers {

MockTimeProvider::MockTimeProvider(models::UtcTime time) : time_(time) {}

void MockTimeProvider::set_time(models::UtcTime time) { time_ = time; }

void MockTimeProvider::set_failure(utils::Status failure) { failure_ = failure; }

utils::Result<models::UtcTime> MockTimeProvider::fetch_utc_time() {
    return failure_ == utils::Status::kOk ? utils::Result<models::UtcTime>::ok(time_)
                                           : utils::Result<models::UtcTime>::fail(failure_);
}

}  // namespace providers
}  // namespace nova
