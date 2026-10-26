/* SPDX-License-Identifier: MIT */
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include "zmk_bt_recovery.h"
LOG_MODULE_REGISTER(zmk_bt_recovery, CONFIG_ZMK_BT_RECOVERY_LOG_LEVEL);
static struct k_spinlock lock;
static struct recovery_policy policy = {
    .state = SUSPENDED,
    .peripheral = !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL),
    .limit = CONFIG_ZMK_BT_RECOVERY_MAX_RETRIES,
    .initial_ms = CONFIG_ZMK_BT_RECOVERY_RETRY_DELAY_MS,
    .cap_ms = CONFIG_ZMK_BT_RECOVERY_MAX_DELAY_MS,
    .grace_until = CONFIG_ZMK_BT_RECOVERY_GRACE_MS,
};
static void retry_work_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(retry_work, retry_work_cb);
/* Called with lock held. Work is on sysworkq, also used by ZMK sleep entry.
 * No additional thread, timer polling, connection reference, or heap allocation. */
static void schedule_locked(void) {
    if (policy.state == RETRY_PENDING)
        k_work_reschedule(&retry_work, K_MSEC(MAX(1, policy.due - k_uptime_get())));
    else
        k_work_cancel_delayable(&retry_work);
}
void zmk_bt_recovery_snapshot(struct recovery_policy *out) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    *out = policy;
    k_spin_unlock(&lock, key);
}
void zmk_bt_recovery_enabled(bool enabled) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    recovery_enable(&policy, enabled);
    schedule_locked();
    k_spin_unlock(&lock, key);
}
void zmk_bt_recovery_result(int err) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    enum recovery_state previous = policy.state;
    recovery_result(&policy, err, k_uptime_get());
    enum recovery_state next = policy.state;
    unsigned attempt = policy.attempts;
    schedule_locked();
    k_spin_unlock(&lock, key);
    if (IS_ENABLED(CONFIG_ZMK_BT_RECOVERY_DIAGNOSTICS))
        LOG_INF("role=%s start=%d state=%d->%d attempts=%u",
                IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL) ? "central-scan" : "peripheral-adv",
                err, previous, next, attempt);
}
void zmk_bt_recovery_link(int index, bool connected, uint8_t reason) {
    if (index < 0 || index >= CONFIG_ZMK_BT_RECOVERY_LINK_COUNT) return;
    k_spinlock_key_t key = k_spin_lock(&lock);
    recovery_link(&policy, index, connected, k_uptime_get());
    k_spin_unlock(&lock, key);
    if (IS_ENABLED(CONFIG_ZMK_BT_RECOVERY_DIAGNOSTICS))
        LOG_INF("split peer=%d connected=%d reason=0x%02x", index, connected, reason);
}
void zmk_bt_recovery_traffic(int index) {
    if (index < 0 || index >= CONFIG_ZMK_BT_RECOVERY_LINK_COUNT) return;
    k_spinlock_key_t key = k_spin_lock(&lock);
    policy.last_traffic = k_uptime_get();
    k_spin_unlock(&lock, key);
}
static void retry_work_cb(struct k_work *work) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    bool run = recovery_due(&policy, k_uptime_get());
    k_spin_unlock(&lock, key);
    if (!run) return;
    int err = zmk_bt_recovery_retry();
    if (err == -ECANCELED) {
        key = k_spin_lock(&lock);
        if (policy.state == RECOVERING)
            policy.state = policy.links ? CONNECTED : DISCONNECTED;
        k_spin_unlock(&lock, key);
    }
    /* Real start results arrive through the bridge exactly once. */
}
static int activity_listener(const zmk_event_t *eh) {
    const struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    k_spinlock_key_t key = k_spin_lock(&lock);
    if (ev->state == ZMK_ACTIVITY_SLEEP || policy.suspended) {
        recovery_suspend(&policy, ev->state == ZMK_ACTIVITY_SLEEP,
                         k_uptime_get(), CONFIG_ZMK_BT_RECOVERY_GRACE_MS);
        schedule_locked();
    }
    k_spin_unlock(&lock, key);
    return ZMK_EV_EVENT_BUBBLE;
}
ZMK_LISTENER(bt_recovery_activity, activity_listener);
ZMK_SUBSCRIPTION(bt_recovery_activity, zmk_activity_state_changed);
