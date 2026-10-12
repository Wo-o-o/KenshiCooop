#include <map>
#include <set>
#include <deque>
#include "src/netproto/InventorySnapshot.h"
namespace coop {
static unsigned long mockNow = 1000;
static unsigned long nowMs() { return mockNow; }
struct InboundInvXferAck { InvXferAckPacket pkt; };
struct Inbound {
    std::deque<InboundInvXferAck> acks;
    void drainInvXferAcks(std::deque<InboundInvXferAck>& out) { out.swap(acks); }
};
namespace engine {
static void dumpInventory(GameWorld*, const unsigned int*) {}
}
class Replicator {
public:
    struct Key {
        u32 t, c, cs, i, s;
        Key() : t(0), c(0), cs(0), i(0), s(0) {}
        bool operator<(const Key& b) const {
            if (t != b.t) return t < b.t; if (c != b.c) return c < b.c;
            if (cs != b.cs) return cs < b.cs; if (i != b.i) return i < b.i;
            return s < b.s;
        }
    };
    typedef std::pair<std::string, u32> XKey;
    struct InvRecv {
        u32 ownerId; bool dirty, truncated; std::vector<InvItemEntry> items;
        InvRecv() : ownerId(0), dirty(true), truncated(false) {}
    };
    struct XferLatch {
        int delta; unsigned long deadlineMs;
        XferLatch() : delta(0), deadlineMs(11000) {}
    };
    std::map<Key, InvRecv> invRecv_;
    std::set<Key> ownedContainers_, ownHands_;
    std::map<Key, bool> xferSeeded_;
    std::map<Key, std::map<XKey, int> > xferBase_;
    std::map<Key, unsigned long> xferDefer_;
    std::map<Key, std::map<XKey, XferLatch> > xferLatch_;
    struct XferOut {
        Key src, dst; XKey key; int qty; bool srcPeer, dstPeer; unsigned long sentMs;
        XferOut() : qty(1), srcPeer(true), dstPeer(false), sentMs(1000) {}
    };
    std::map<u32, XferOut> xferOut_;
    unsigned long xferScanMs_;
    Replicator() : xferScanMs_(0) {}
    void handForContainerKey(const Key& k, unsigned int out[5]) {
        out[0] = k.t; out[1] = k.c; out[2] = k.cs; out[3] = k.i; out[4] = k.s;
    }
    void xferRebase(GameWorld*, const Key&) {}
    void applyInventories(GameWorld*);
    void applyXferAcks(GameWorld*, Inbound&, u32);
    void releaseXferLatch(const Key&, const XKey&, int);
};
#include "Replicator-under-test.inc"
}
