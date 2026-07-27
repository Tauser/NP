// Fonte controlável para testes de service. Não faz I/O nem representa NTP.
#pragma once

#include "providers/i_time_provider.hpp"

namespace nova {
namespace providers {

class MockTimeProvider final : public ITimeProvider {
public:
    explicit MockTimeProvider(models::UtcTime time = {});

    void set_time(models::UtcTime time);
    void set_failure(utils::Status failure);
    utils::Result<models::UtcTime> fetch_utc_time() override;

private:
    models::UtcTime time_;
    utils::Status failure_ = utils::Status::kOk;
};

}  // namespace providers
}  // namespace nova
