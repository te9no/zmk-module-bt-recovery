"""Compile actual pinned ZMK bridge functions against deterministic BLE mocks."""
import pathlib, re, subprocess, sys, tempfile
tree = pathlib.Path(sys.argv[1])
def function(text, name):
    match = re.search(r"(?:static )?(?:int|void) " + name + r"\([^;]*?\)\s*\{", text)
    assert match, name
    depth = 1; end = match.end()
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[match.start():end]
central = (tree / "app/src/split/bluetooth/central.c").read_text()
prefix = r"""
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <stddef.h>
#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_PERIPHERALS 2
#define CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING 1
#define CONFIG_ZMK_INPUT_SPLIT 0
#define IS_ENABLED(x) (x)
#define BT_ADDR_LE_STR_LEN 32
#define BT_CONN_ROLE_CENTRAL 0
#define BT_LE_SCAN_PASSIVE 0
#define K_NO_WAIT 0
#define ZMK_SPLIT_TRANSPORT_PERIPHERAL_EVENT_TYPE_BATTERY_EVENT 3
#define LOG_DBG(...) ((void)0)
#define LOG_ERR(...) ((void)0)
#define LOG_WRN(...) ((void)0)
#define BT_RECOVERY(x) x
struct bt_conn { int slot, role; };
struct bt_conn_info { int role; };
struct { struct bt_conn *conn; } peripherals[2];
struct peripheral_event_wrapper {
 int source;
 struct { int type; struct { struct {int level;} battery_event; } data; } event;
};
static bool is_enabled=true, is_scanning;
static int start_rc, starts, result, reports, releases, messages, links;
static int peripheral_event_msgq, peripheral_event_work, notify_status_work;
static void *split_central_device_found;
static int bt_le_scan_start(int a, void *b) {starts++; return start_rc;}
static void zmk_bt_recovery_result(int e) {reports++;result=e;}
static void zmk_bt_recovery_link(int i,bool c,uint8_t r) {assert(i>=0&&i<2);links++;}
static int peripheral_slot_index_for_conn(struct bt_conn *c){return c->slot;}
static void *bt_conn_get_dst(struct bt_conn *c){return c;}
static void bt_addr_le_to_str(void*a,char*b,int n){}
static void bt_conn_get_info(struct bt_conn*c,struct bt_conn_info*i){i->role=c->role;}
static void release_peripheral_slot_for_conn_unused(void){}
static int release_peripheral_slot_for_conn(struct bt_conn*c){releases++;return 0;}
static void confirm_peripheral_slot_conn(struct bt_conn*c){}
static void split_central_process_connection(struct bt_conn*c){}
static int k_work_submit(void*w){return 0;}
static int k_msgq_put(void*q,struct peripheral_event_wrapper*e,int t){
 assert(e->source>=0&&e->source<2);messages++;return 0;
}
"""
body = "\n".join(function(central,n) for n in
                 ["start_scanning","split_central_connected","split_central_disconnected"])
main = r"""
int main(void) {
 start_rc=-EPERM; assert(start_scanning()==-EPERM);assert(!is_scanning&&reports==1);
 start_rc=0; assert(start_scanning()==0);assert(is_scanning&&result==0);
 int before=starts;start_scanning();assert(starts==before);
 struct bt_conn host={-22,1}, mouse={-22,0}, peer={1,0};
 split_central_connected(&host,0);split_central_connected(&mouse,2);
 split_central_disconnected(&host,19);split_central_disconnected(&mouse,19);
 assert(!links&&!releases&&!messages);
 split_central_connected(&peer,0);split_central_disconnected(&peer,8);
 assert(links==2&&releases==1&&messages==1);
 is_scanning=false;is_enabled=false;before=starts;start_scanning();assert(starts==before);
 is_enabled=true;peripherals[0].conn=&peer;peripherals[1].conn=&peer;
 start_scanning();assert(starts==before);
 return 0;
}
"""
peripheral=(tree/"app/src/split/bluetooth/peripheral.c").read_text()
adv=r"""
#include <assert.h>
#include <stdbool.h>
#include <errno.h>
#define BT_RECOVERY(x) x
#define LOG_ERR(...) ((void)0)
struct k_work {int unused;};
static bool enabled, is_connected, low_duty_advertising;
static int calls, rc, reported;
static int start_advertising(bool low) { calls++;return rc; }
static void zmk_bt_recovery_result(int err) {reported=err;}
""" + function(peripheral,"advertising_cb") + r"""
int main(void) {
 advertising_cb(0);assert(calls==0);
 enabled=true;is_connected=true;advertising_cb(0);assert(calls==0);
 is_connected=false;rc=-ENOMEM;advertising_cb(0);assert(calls==1&&reported==-ENOMEM);
 rc=-EALREADY;advertising_cb(0);assert(reported==0);
 rc=0;advertising_cb(0);assert(reported==0);return 0;
}
"""
with tempfile.TemporaryDirectory() as tmp:
    for name, src in [("central",prefix+body+main),("peripheral",adv)]:
        p=pathlib.Path(tmp)/name
        p.with_suffix(".c").write_text(src)
        subprocess.run(["cc","-std=c11","-fsanitize=address,undefined",str(p.with_suffix(".c")),"-o",str(p)],check=True)
        subprocess.run([str(p)],check=True)
print("PASS: actual scan/adv start functions, host/third-party rejection, split battery slot, two peers")
