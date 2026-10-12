// Internal snapshot editing policy. No wire-layout or protocol-version change.
#ifndef COOP_INVENTORY_SNAPSHOT_H
#define COOP_INVENTORY_SNAPSHOT_H
#include "Wire.h"
#include <vector>
namespace coop {
// A zero-quantity row is a latch tombstone. Removing a parent also removes its
// description's children; remaining children must name the parent's NEW index.
// Build a replacement first so invalid input cannot leave a half-edited graph.
inline bool compactInventorySnapshot(std::vector<InvItemEntry>& entries) {
    const unsigned int count = (unsigned int)entries.size();
    if (count > 255) return false; // parentIdx has one byte
    std::vector<unsigned int> index(count, count);
    std::vector<InvItemEntry> result;
    for (unsigned int i = 0; i < count; ++i) {
        unsigned int p = entries[i].parentIdx;
        if (p && (p > count || p == i + 1 || entries[p - 1].parentIdx)) return false;
        if (!entries[i].quantity || (p && !entries[p - 1].quantity)) continue;
        index[i] = (unsigned int)result.size();
        result.push_back(entries[i]);
    }
    for (unsigned int i = 0; i < result.size(); ++i) {
        unsigned int p = result[i].parentIdx;
        if (p) result[i].parentIdx = (u8)(index[p - 1] + 1);
    }
    entries.swap(result);
    return true;
}
} // namespace coop
#endif
