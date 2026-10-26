/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>
enum recovery_state { DISCONNECTED, CONNECTED, SCANNING, ADVERTISING,
                      RETRY_PENDING, RECOVERING, SUSPENDED, RECOVERY_FAILED };
struct recovery_policy {
    enum recovery_state state;
    bool enabled, suspended, peripheral;
    uint8_t attempts, limit;
    uint32_t links, initial_ms, cap_ms;
    int64_t due, grace_until, last_connected, last_traffic;
    int last_error;
};
void recovery_enable(struct recovery_policy *p, bool enabled);
void recovery_suspend(struct recovery_policy *p, bool suspended, int64_t now, uint32_t grace);
void recovery_result(struct recovery_policy *p, int err, int64_t now);
bool recovery_due(struct recovery_policy *p, int64_t now);
bool recovery_link(struct recovery_policy *p, int index, bool connected, int64_t now);
uint32_t recovery_delay(const struct recovery_policy *p);
