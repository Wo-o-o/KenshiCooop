#include "src/netproto/InventorySnapshot.h"
#include <cstdio>
#include <cstring>
using namespace coop;
static int total = 0, failures = 0;
#define CHECK(label, expr) do { ++total; if (!(expr)) { ++failures; std::printf("FAIL %s\n", label); } } while (0)
static InvItemEntry entry(const char* sid, unsigned quantity, unsigned parent = 0) {
    InvItemEntry e; std::memset(&e, 0, sizeof(e)); std::strcpy(e.stringID, sid);
    e.quantity = (u16)quantity; e.parentIdx = (u8)parent; return e;
}
int main() {
    std::vector<InvItemEntry> a;
    a.push_back(entry("removed", 0)); a.push_back(entry("bag", 1)); a.push_back(entry("child", 7, 2));
    CHECK("A03 compaction succeeds", compactInventorySnapshot(a));
    CHECK("A03 child points to shifted bag", a.size() == 2 && a[1].parentIdx == 1 && a[1].quantity == 7);
    CHECK("A03 repeated compaction idempotent", compactInventorySnapshot(a) && a[1].parentIdx == 1);
    a.clear(); a.push_back(entry("bag", 0)); a.push_back(entry("child", 7, 1)); a.push_back(entry("other bag", 1)); a.push_back(entry("other child", 3, 3));
    CHECK("removed parent removes only its described children", compactInventorySnapshot(a) && a.size() == 2 && a[1].parentIdx == 1 && a[1].quantity == 3);
    a.clear(); a.push_back(entry("child before parent", 2, 2)); a.push_back(entry("bag", 1));
    CHECK("valid reordered rows preserve graph", compactInventorySnapshot(a) && a[0].parentIdx == 2);
    a.clear(); a.push_back(entry("orphan", 1, 2));
    CHECK("orphan rejected without mutation", !compactInventorySnapshot(a) && a.size() == 1 && a[0].parentIdx == 2);
    a.clear(); a.push_back(entry("self", 1, 1));
    CHECK("self-parent rejected", !compactInventorySnapshot(a));
    a.clear(); a.push_back(entry("a", 1, 2)); a.push_back(entry("b", 1, 1));
    CHECK("cycles rejected", !compactInventorySnapshot(a));
    a.clear(); a.push_back(entry("top", 1)); a.push_back(entry("child", 1, 1)); a.push_back(entry("grandchild", 1, 2));
    CHECK("unsupported deep nesting rejected", !compactInventorySnapshot(a));
    a.assign(256, entry("row", 1));
    CHECK("index overflow rejected", !compactInventorySnapshot(a));
    std::printf("snapshot regressions: %d/%d PASS\n", total - failures, total);
    return failures ? 1 : 0;
}
