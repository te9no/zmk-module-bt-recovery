/* SPDX-License-Identifier: MIT */
#include "recovery_policy.h"
uint32_t recovery_delay(const struct recovery_policy *p) {
    uint32_t delay = p->initial_ms;
    for (unsigned i = 0; i < p->attempts && delay < p->cap_ms; i++)
        delay = delay > p->cap_ms / 2 ? p->cap_ms : delay * 2;
    return delay > p->cap_ms ? p->cap_ms : delay;
}
void recovery_enable(struct recovery_policy *p, bool enabled) {
    if (p->enabled == enabled) return;
    p->enabled = enabled;
    p->attempts = 0;
    p->due = 0;
    p->state = !enabled || p->suspended ? SUSPENDED :
               p->links ? CONNECTED : DISCONNECTED;
}
void recovery_suspend(struct recovery_policy *p, bool suspended, int64_t now, uint32_t grace) {
    p->suspended = suspended;
    p->due = 0;
    p->state = suspended || !p->enabled ? SUSPENDED :
               p->links ? CONNECTED : DISCONNECTED;
    p->grace_until = now + grace;
    /* Resume does not itself start scanning or replenish an exhausted budget.
     * The ZMK owner must report a new operation result. */
}
void recovery_result(struct recovery_policy *p, int err, int64_t now) {
    p->last_error = err;
    if (!p->enabled || p->suspended) return;
    if (!err) {
        p->attempts = 0;
        p->due = 0;
        p->state = p->peripheral ? ADVERTISING : SCANNING;
        return;
    }
    if (p->state == RETRY_PENDING) return; /* duplicates cannot postpone the deadline */
    if (p->attempts >= p->limit) {
        p->state = RECOVERY_FAILED;
        p->due = 0;
        return;
    }
    p->due = now + recovery_delay(p);
    if (p->due < p->grace_until) p->due = p->grace_until;
    p->state = RETRY_PENDING;
}
bool recovery_due(struct recovery_policy *p, int64_t now) {
    if (!p->enabled || p->suspended || p->state != RETRY_PENDING || now < p->due)
        return false;
    p->state = RECOVERING;
    p->due = 0;
    p->attempts++;
    return true;
}
bool recovery_link(struct recovery_policy *p, int index, bool connected, int64_t now) {
    if (index < 0 || index >= 32) return false;
    if (connected) {
        p->links |= UINT32_C(1) << index;
        p->last_connected = now;
        /* Another missing peripheral's scan failure must keep its retry. */
        if (p->state != RETRY_PENDING && p->state != RECOVERING &&
            p->state != RECOVERY_FAILED && !p->suspended && p->enabled)
            p->state = CONNECTED;
    } else {
        p->links &= ~(UINT32_C(1) << index);
        if (p->state == CONNECTED && !p->links) p->state = DISCONNECTED;
    }
    return true;
}
