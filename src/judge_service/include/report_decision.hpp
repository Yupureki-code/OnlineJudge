#pragma once

#include <cstdint>

namespace oj::judge
{

enum class ReportDecision { Ack, Retry, Reject };
enum class DeliveryAction { Ack, Retry, DeadLetter };

inline DeliveryAction ResolveDeliveryAction(ReportDecision decision, uint32_t retry_count)
{
    if (decision == ReportDecision::Ack) return DeliveryAction::Ack;
    if (decision == ReportDecision::Reject || retry_count >= 3) return DeliveryAction::DeadLetter;
    return DeliveryAction::Retry;
}

} // namespace oj::judge
