/* SPDX-License-Identifier: MIT */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "recovery_policy.h"
/* Explicit integration ABI; only the pinned ZMK owner may call these hooks. */
void zmk_bt_recovery_enabled(bool enabled);
void zmk_bt_recovery_result(int err);
void zmk_bt_recovery_link(int index, bool connected, uint8_t reason);
void zmk_bt_recovery_traffic(int index);
void zmk_bt_recovery_snapshot(struct recovery_policy *out);
/* Implemented by the pinned core bridge, never by symbol interposition.
 * -ECANCELED: no longer appropriate. A real start reports its own result. */
int zmk_bt_recovery_retry(void);
