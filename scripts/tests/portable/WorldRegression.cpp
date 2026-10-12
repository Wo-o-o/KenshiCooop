// Actual W1 publisher/receiver against deterministic world and network mocks.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <deque>
#include <vector>
#include "src/netproto/Wire.h"
#define _snprintf snprintf
struct GameWorld {};
struct RootObject {
    bool live, picked; float pos[3];
    RootObject() : live(true), picked(false) { pos[0] = pos[1] = pos[2] = 0; }
};
namespace coop {
static unsigned long clockMs = 1000;
static unsigned long nowMs() { return clockMs; }
static void logLine(const char*) {}
static bool isGearType(u32 type) { return type == 0 || type == 1 || type == 46; }
namespace engine {
struct WorldItemRaw { u32 hand[5]; char stringID[48]; u32 itemType; u16 quantity, quality; float x,y,z; u32 hash; };
struct ItemDropEdge { u32 ownerHand[5], itemHand[5]; char stringID[48]; u32 itemType; u16 quantity,quality; float x,y,z; };
static std::vector<WorldItemRaw> rows;
static std::vector<ItemDropEdge> drops;
static std::map<u32, RootObject> objects;
static unsigned spawned = 0, destroyed = 0;
static u32 nextHandle = 9000;
static unsigned captureWorldItems(GameWorld*, WorldItemRaw* out, unsigned cap, float) {
    unsigned n = (unsigned)rows.size(); if (n > cap) n = cap;
    for (unsigned i = 0; i < n; ++i) out[i] = rows[i]; return n;
}
static unsigned drainItemDrops(ItemDropEdge* out, unsigned cap) {
    unsigned n = (unsigned)drops.size(); if (n > cap) n = cap;
    for (unsigned i = 0; i < n; ++i) out[i] = drops[i]; drops.clear(); return n;
}
static RootObject* resolveObjectByHand(const unsigned int* hand) {
    std::map<u32, RootObject>::iterator it = objects.find(hand[3]);
    return it != objects.end() && it->second.live ? &it->second : 0;
}
static int groundObjectLiveness(RootObject* ro, float* pos, bool* picked = 0) {
    if (picked) *picked = ro && ro->live && ro->picked;
    if (!ro || !ro->live || ro->picked) return 0;
    if (pos) std::memcpy(pos, ro->pos, sizeof(ro->pos)); return 1;
}
static int groundItemLiveness(const unsigned int* hand, float* pos) { return groundObjectLiveness(resolveObjectByHand(hand), pos); }
static RootObject* spawnWorldItemProxy(GameWorld*, const char*, unsigned, int, float x, float y, float z, unsigned int* hand) {
    ++spawned; u32 id = nextHandle++; std::memset(hand, 0, 5*sizeof(u32)); hand[3] = id;
    RootObject& obj = objects[id]; obj.pos[0] = x; obj.pos[1] = y; obj.pos[2] = z; return &obj;
}
static void updateWorldItemProxy(RootObject* ro, float x,float y,float z) { ro->pos[0]=x; ro->pos[1]=y; ro->pos[2]=z; }
static bool removeWorldItemProxy(GameWorld*, RootObject* ro) { if (!ro || !ro->live) return false; ++destroyed; ro->live=false; return true; }
}
struct NetLink {
    std::vector<WorldItemEntry> sent; std::vector<u32> removed, claimed;
    void queueWorldItems(u32,const WorldItemEntry* rows,unsigned n) { sent.insert(sent.end(),rows,rows+n); }
    void queueWorldRemove(u32,const u32* ids,unsigned n) { removed.insert(removed.end(),ids,ids+n); }
    void queueWorldClaim(u32,u32,const u32* ids,unsigned n) { claimed.insert(claimed.end(),ids,ids+n); }
};
struct InboundWorldItems { u32 ownerId; std::vector<WorldItemEntry> items; };
struct InboundWorldRemove { u32 ownerId; std::vector<u32> netIds; };
struct Inbound {
    std::deque<InboundWorldItems> items; std::deque<InboundWorldRemove> removes;
    void drainWorldItems(std::deque<InboundWorldItems>& out) { out.swap(items); }
    void drainWorldRemove(std::deque<InboundWorldRemove>& out) { out.swap(removes); }
};
class Replicator {
public:
    struct Key {
        u32 t,c,cs,i,s;
        bool operator<(const Key& b) const {
            if(t!=b.t)return t<b.t; if(c!=b.c)return c<b.c; if(cs!=b.cs)return cs<b.cs;
            if(i!=b.i)return i<b.i; return s<b.s;
        }
    };
    struct WorldTrack { u32 netId,hash; unsigned long lastSendMs; float x,y,z; bool seen; char stringID[48]; u32 itemType; u16 quantity,quality; bool baseline; };
    struct WorldProxy { RootObject* obj; u32 hand[5]; float x,y,z; u32 hash; };
    struct PendingDrop { char stringID[48]; float x,y,z; unsigned long ms; };
    std::map<Key,WorldTrack> worldTrack_;
    std::map<std::pair<u32,u32>,WorldProxy> worldProxies_;
    std::vector<PendingDrop> pendingDrops_;
    std::set<std::pair<u32,u32> > consumedWorldItems_;
    u32 nextWorldNetId_; bool worldSeeded_;
    Replicator():nextWorldNetId_(1),worldSeeded_(false){}
    int ownerClassForHand(const unsigned int* hand) { return hand[0] == 2 ? 2 : 1; }
    RootObject* liveWorldProxy(const WorldProxy&);
    void publishWorldItems(GameWorld*,NetLink&,u32);
    void applyWorldItems(GameWorld*,Inbound&);
};
#include "World-under-test.inc"
}
using namespace coop;
static int total=0,failures=0;
#define CHECK(label,expr) do {++total;if(!(expr)){++failures;std::printf("FAIL %s\n",label);}}while(0)
static engine::WorldItemRaw row(u32 id) {
    engine::WorldItemRaw r; std::memset(&r,0,sizeof(r));r.hand[3]=id;
    std::strcpy(r.stringID,"food");r.itemType=2;r.quantity=3;r.quality=100;r.x=10;
    engine::objects[id].pos[0]=10; return r;
}
static engine::ItemDropEdge edge(u32 id) {
    engine::ItemDropEdge e; std::memset(&e,0,sizeof(e)); e.itemHand[3]=id;
    std::strcpy(e.stringID,"food");e.itemType=2;e.quantity=3;e.x=10;return e;
}
int main() {
    GameWorld world;NetLink net;Replicator repl;
    engine::rows.clear();repl.publishWorldItems(&world,net,0);
    CHECK("empty initial scan never emits",net.sent.empty());
    clockMs=1200;engine::rows.push_back(row(1));repl.publishWorldItems(&world,net,0);
    CHECK("late first native never streams a free duplicate",net.sent.empty());
    clockMs=5400000;engine::rows.push_back(row(2));repl.publishWorldItems(&world,net,0);
    CHECK("native loaded at 90 minutes remains native",net.sent.empty());
    engine::rows.push_back(row(3));engine::drops.push_back(edge(3));repl.publishWorldItems(&world,net,0);
    CHECK("real hook-authored drop still streams",net.sent.size()==1 && net.sent[0].quantity==3);
    engine::drops.push_back(edge(3));repl.publishWorldItems(&world,net,0);
    CHECK("duplicate drop edge cannot create a second track",net.sent.size()==1);
    engine::drops.push_back(edge(0));engine::rows.push_back(row(4));repl.publishWorldItems(&world,net,0);
    CHECK("unresolved hook drop is recovered by matching scan",net.sent.size()==2);
    engine::rows.push_back(row(5));engine::ItemDropEdge peer=edge(5);peer.ownerHand[0]=2;engine::drops.push_back(peer);
    repl.publishWorldItems(&world,net,0);
    CHECK("peer-owned drop never echoes",net.sent.size()==2);
    engine::objects[3].live=false;repl.publishWorldItems(&world,net,0);
    CHECK("real streamed item still culls",!net.removed.empty());
    Inbound in;InboundWorldItems incoming;incoming.ownerId=1;WorldItemEntry e;std::memset(&e,0,sizeof(e));
    e.netId=99;std::strcpy(e.stringID,"food");e.itemType=2;e.quantity=3;incoming.items.push_back(e);
    in.items.push_back(incoming);repl.applyWorldItems(&world,in);
    CHECK("normal remote drop creates one proxy",engine::spawned==1);
    in.items.push_back(incoming);repl.applyWorldItems(&world,in);
    CHECK("duplicate snapshot remains one proxy",engine::spawned==1);
    repl.worldProxies_[std::make_pair(1u,99u)].obj->picked=true;
    repl.publishWorldItems(&world,net,0);
    CHECK("pickup still sends one claim",net.claimed.size()==1);
    in.items.push_back(incoming);repl.applyWorldItems(&world,in);
    CHECK("A09 delayed snapshot after pickup cannot respawn item",engine::spawned==1);
    std::printf("world regressions: %d/%d PASS\n",total-failures,total);return failures?1:0;
}
