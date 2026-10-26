/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include "recovery_policy.h"
static struct recovery_policy fresh(void) {
    struct recovery_policy p = {.limit=5, .initial_ms=1000, .cap_ms=16000};
    recovery_enable(&p, true);
    return p;
}
int main(void) {
    struct recovery_policy p=fresh();
    /* Normal connect/disconnect is observation only; no polling/retry. */
    assert(recovery_link(&p, 0, true, 100)); assert(p.state==CONNECTED);
    assert(recovery_link(&p, 0, false, 200)); assert(!recovery_due(&p, 999999));
    assert(!recovery_link(&p,-22,false,0)); assert(!recovery_link(&p,234,false,0));
    /* Two peers: a connection must not cancel recovery for the missing peer. */
    recovery_result(&p,-EPERM,1000); assert(p.due==2000);
    recovery_link(&p,1,true,1100); assert(p.state==RETRY_PENDING);
    recovery_result(&p,-EPERM,1500); assert(p.due==2000);
    int64_t now=2000;
    const int delays[]={2000,4000,8000,16000};
    for(int i=0;i<5;i++) {
        assert(!recovery_due(&p,now-1)); assert(recovery_due(&p,now));
        recovery_result(&p,-EBUSY,now);
        if(i<4) { assert(p.due==now+delays[i]); now=p.due; }
    }
    assert(p.state==RECOVERY_FAILED && p.attempts==5);
    recovery_result(&p,-EPERM,now+100); assert(p.state==RECOVERY_FAILED);
    assert(!recovery_due(&p,now+999999));
    /* Standard ZMK success still works after exhausted module budget. */
    recovery_result(&p,0,now); assert(p.state==SCANNING && p.attempts==0);
    /* Advertising start failure uses same bounded policy, different owner. */
    p.peripheral=true; recovery_result(&p,-ENOMEM,now);
    assert(recovery_due(&p,now+1000)); recovery_result(&p,0,now+1000);
    assert(p.state==ADVERTISING);
    recovery_result(&p,-EPERM,now+2000);
    recovery_suspend(&p,true,now+2100,5000);
    assert(!recovery_due(&p,now+99999));
    recovery_result(&p,-EPERM,now+2200); assert(p.state==SUSPENDED);
    recovery_suspend(&p,false,now+3000,5000); assert(!p.due);
    recovery_result(&p,-EPERM,now+3000); assert(p.due==now+8000);
    recovery_enable(&p,false); assert(!recovery_due(&p,now+99999));
    recovery_result(&p,-EPERM,now+99999); assert(p.state==SUSPENDED);
    recovery_enable(&p,true); assert(p.attempts==0);
    p.initial_ms=60000; p.cap_ms=300000; p.attempts=10;
    assert(recovery_delay(&p)==300000);
    puts("PASS: policy normal links, failure budgets, duplicates, multi-peer, invalid index, suspend/grace, disabled");
}
