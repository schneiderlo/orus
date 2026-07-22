#ifndef ORUS_CONTRACTS_RESOURCE_MONITOR_H_
#define ORUS_CONTRACTS_RESOURCE_MONITOR_H_

#include "orus/contracts/contracts.h"

#include <cstdint>

namespace orus::contracts::internal {

std::uint64_t MonotonicNowNs();
std::uint64_t CurrentProcessRssBytes();
ResourceUsage ObserveResourceUsage(ResourceUsage minimum, std::uint64_t started_ns);

}  // namespace orus::contracts::internal

#endif  // ORUS_CONTRACTS_RESOURCE_MONITOR_H_
