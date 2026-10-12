// A test-only engine surface. Production function bodies are extracted at build time.
// This models enumeration and removal; it does not model Kenshi ABI or equipment AI.
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include "src/netproto/Wire.h"
#include "src/netproto/ContentHash.h"
#define _snprintf snprintf
template<class T> struct lektor : std::vector<T> {};
namespace Ogre { template<class T> struct vector { typedef std::vector<T> type; }; }
struct Inventory;
struct GameWorld {};
struct RootObject {
    Inventory* inv;
    RootObject() : inv(0) {}
    Inventory* getInventory() { return inv; }
};
struct GameData {
    std::string stringID; int type;
    GameData(const char* s, int t) : stringID(s), type(t) {}
};
struct LockedArmour { void* lock; };
struct Item : RootObject {
    GameData* data; int quantity; float quality; bool isEquipped, fault;
    int slotType; unsigned char grade;
    Item(GameData* d) : data(d), quantity(1), quality(1), isEquipped(false),
                        fault(false), slotType(0), grade(95) {}
    GameData* getGameData() { if (fault) throw 1; return data; }
    LockedArmour* isLockedArmour() { return 0; }
};
struct InventorySection {
    struct SectionItem { Item* item; SectionItem(Item* i) : item(i) {} };
    bool isAnEquippedItemSection; int limitedSlot; std::string name;
    Ogre::vector<SectionItem>::type items;
    InventorySection() : isAnEquippedItemSection(false), limitedSlot(0), name("main") {}
};
struct Inventory {
    lektor<Item*> _allItems; lektor<InventorySection*> sections;
    bool fault, missingSections; int removed;
    Inventory() : fault(false), missingSections(false), removed(0) {}
    bool removeItemAutoDestroy(Item* it, int take) {
        if (!it || take < 1 || take > it->quantity) return false;
        it->quantity -= take; removed += take;
        if (!it->quantity) {
            _allItems.erase(std::remove(_allItems.begin(), _allItems.end(), it), _allItems.end());
            for (size_t s = 0; s < sections.size(); ++s) {
                std::vector<InventorySection::SectionItem>& a = sections[s]->items;
                for (size_t i = 0; i < a.size(); )
                    if (a[i].item == it) a.erase(a.begin() + i); else ++i;
            }
        }
        return true;
    }
};
namespace coop {
static void logLine(const char*) {}
namespace engine {
static unsigned short qualityBucketOf(float q) { return (unsigned short)(q * 100); }
static unsigned char gradeLevelOf(Item* it) { return it->grade; }
static void fillItemProvenance(Item*, InvItemEntry&) {}
static lektor<InventorySection*>* getSections(Inventory* inv) {
    if (inv->fault) throw 1;
    if (inv->missingSections) return 0;
    return &inv->sections;
}
static lektor<InventorySection*>* (*g_getSectionsFn)(Inventory*) = getSections;
static Item* (*g_getPrimaryWeaponFn)(Inventory*) = 0;
static Item* (*g_getSecondaryWeaponFn)(Inventory*) = 0;
static RootObject root;
static RootObject* resolveObjectByHand(const unsigned int*) { return &root; }
static Inventory* invOf(RootObject* ro) { return ro->inv; }
unsigned int readInvItems(Inventory*, InvItemEntry*, Item**, unsigned int,
                         bool* = 0, bool = false);
unsigned int captureContainerContents(GameWorld*, const unsigned int*, InvItemEntry*,
                                      unsigned int, unsigned int*, bool* = 0, bool = false);
// Movement/factory paths are deliberately outside these tests. Removal below is real
// removeByKey + applyToInventory, with only Inventory's engine operation simulated.
static int equipExisting(Inventory*, Item*) { return 0; }
static int unequipToLoose(Inventory*, Item*) { return 0; }
static int correctWeaponSlot(Inventory*, const char*, unsigned int, unsigned short) { return 0; }
static bool isContainerItemType(unsigned int t) { return t == 46; }
static bool createItemAndAdd(GameWorld*, Inventory*, const char*, unsigned int,
                             int, int, bool, const char*, const char*, unsigned char) { return false; }
} }
