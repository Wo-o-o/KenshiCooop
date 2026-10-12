#include "InventoryMock.h"
namespace coop { namespace engine {
#include "Inventory-under-test.inc"
} }
using namespace coop;
using namespace coop::engine;
static int total = 0, failures = 0;
#define CHECK(label, expr) do { ++total; if (!(expr)) { ++failures; std::printf("FAIL %s\n", label); } } while (0)
struct Fixture {
    GameData bagData, foodData, otherBagData;
    Inventory top, bag, otherBag;
    InventorySection first, second, otherSection;
    Item parent, otherParent;
    std::vector<Item*> allocated;
    Fixture() : bagData("bag", 46), foodData("food", 2), otherBagData("other_bag", 46),
                parent(&bagData), otherParent(&otherBagData) {
        parent.inv = &bag; otherParent.inv = &otherBag;
        top._allItems.push_back(&parent); root.inv = &top;
        bag.sections.push_back(&first); bag.sections.push_back(&second);
        otherBag.sections.push_back(&otherSection);
    }
    ~Fixture() { for (size_t i = 0; i < allocated.size(); ++i) delete allocated[i]; }
    Item* add(Inventory& inv, InventorySection* section, int quantity = 1) {
        Item* it = new Item(&foodData); it->quantity = quantity; allocated.push_back(it);
        inv._allItems.push_back(it);
        if (section) section->items.push_back(InventorySection::SectionItem(it));
        return it;
    }
};
static void captureTests() {
    {
        Fixture f; f.add(f.bag, &f.first);
        for (unsigned i = 1; i < INV_ITEMS_MAX; ++i) f.add(f.top, 0);
        InvItemEntry out[INV_ITEMS_MAX]; bool truncated = false;
        unsigned n = readInvItems(&f.top, out, 0, INV_ITEMS_MAX, &truncated, true);
        CHECK("A02 top exactly fills real 64-entry cap", n == INV_ITEMS_MAX);
        CHECK("A02 omitted child after full top marks truncation", truncated);
    }
    {
        Fixture f;
        for (unsigned i = 1; i < INV_ITEMS_MAX; ++i) f.add(f.bag, &f.first);
        f.add(f.bag, &f.second);
        InvItemEntry out[INV_ITEMS_MAX]; bool truncated = false;
        unsigned n = readInvItems(&f.top, out, 0, INV_ITEMS_MAX, &truncated, true);
        CHECK("A02 first nested section fills cap", n == INV_ITEMS_MAX);
        CHECK("A02 unseen next section marks truncation", truncated);
        CHECK("parent reference survives boundary capture", out[1].parentIdx == 1);
    }
    {
        Fixture f; f.top._allItems.push_back(&f.otherParent);
        for (unsigned i = 2; i < INV_ITEMS_MAX; ++i) f.add(f.bag, &f.first);
        f.add(f.otherBag, &f.otherSection);
        InvItemEntry out[INV_ITEMS_MAX]; bool truncated = false;
        readInvItems(&f.top, out, 0, INV_ITEMS_MAX, &truncated, true);
        CHECK("A02 unseen next parent marks truncation", truncated);
    }
    {
        Fixture f;
        for (unsigned i = 1; i < INV_ITEMS_MAX; ++i) f.add(f.bag, &f.first);
        InvItemEntry out[INV_ITEMS_MAX]; bool truncated = true;
        unsigned n = readInvItems(&f.top, out, 0, INV_ITEMS_MAX, &truncated, true);
        CHECK("exact full complete snapshot is not falsely truncated", n == INV_ITEMS_MAX && !truncated);
        readInvItems(&f.top, out, 0, INV_ITEMS_MAX, &truncated, false);
        CHECK("top-only readers retain existing meaning", !truncated && out[0].parentIdx == 0);
    }
    {
        Fixture f; f.add(f.bag, &f.first)->fault = true;
        InvItemEntry out[INV_ITEMS_MAX]; bool truncated = false;
        readInvItems(&f.top, out, 0, INV_ITEMS_MAX, &truncated, true);
        CHECK("nested read fault remains incomplete", truncated);
    }
    {
        Fixture f; f.add(f.bag, &f.first, 7); f.add(f.bag, &f.second, 3);
        InvItemEntry desired[2]; bool truncated = false;
        readInvItems(&f.top, desired, 0, 2, &truncated, true);
        GameWorld world; unsigned hand[5] = {0};
        applyContainerContents(&world, hand, desired, 2, truncated);
        CHECK("incomplete desired cannot delete remaining nested units", f.bag.removed == 0);
        CHECK("nested identity and quantity remain intact", f.parent.inv == &f.bag && f.bag._allItems.size() == 2);
    }
    {
        Fixture f; f.add(f.bag, &f.first); f.bag.missingSections = true;
        InvItemEntry out[INV_ITEMS_MAX]; bool truncated = false;
        readInvItems(&f.top, out, 0, INV_ITEMS_MAX, &truncated, true);
        CHECK("unreadable nested section list is incomplete", truncated);
        f.bag.missingSections = false;
        g_getSectionsFn = 0;
        readInvItems(&f.top, out, 0, INV_ITEMS_MAX, &truncated, true);
        CHECK("unavailable section reader is incomplete", truncated);
        g_getSectionsFn = getSections;
    }
}
static void emptyTests() {
    {
        Fixture f; f.add(f.bag, &f.first, 7);
        InvItemEntry desired[1]; readInvItems(&f.top, desired, 0, 1, 0, false);
        GameWorld world; unsigned hand[5] = {0};
        applyContainerContents(&world, hand, desired, 1, false);
        CHECK("A01 complete empty bag clears stale seven units", f.bag.removed == 7 && f.bag._allItems.empty());
        CHECK("A01 preserves backpack object and parent inventory", f.top._allItems.size() == 1 && f.parent.inv == &f.bag);
        applyContainerContents(&world, hand, desired, 1, false);
        CHECK("A01 duplicate snapshot is idempotent", f.bag.removed == 7);
    }
    {
        Fixture f; f.add(f.bag, &f.first, 7);
        InvItemEntry desired[1]; readInvItems(&f.top, desired, 0, 1, 0, false);
        GameWorld world; unsigned hand[5] = {0};
        applyContainerContents(&world, hand, desired, 1, true);
        CHECK("A01 absent children in truncated snapshot never means empty", f.bag.removed == 0);
    }
    {
        Fixture f; f.add(f.bag, &f.first, 7); f.top._allItems.push_back(&f.otherParent);
        f.add(f.otherBag, &f.otherSection, 3);
        InvItemEntry desired[3]; readInvItems(&f.top, desired, 0, 2, 0, false);
        std::memset(&desired[2], 0, sizeof(desired[2])); std::strcpy(desired[2].stringID, "food");
        desired[2].itemType = 2; desired[2].quantity = 3; desired[2].parentIdx = 2;
        GameWorld world; unsigned hand[5] = {0};
        applyContainerContents(&world, hand, desired, 3, false);
        CHECK("A01 empty first bag clears while second remains populated", f.bag.removed == 7 && f.otherBag.removed == 0);
    }
    {
        Fixture f;
        for (unsigned i = 0; i <= INV_ITEMS_MAX; ++i) f.add(f.bag, &f.first);
        InvItemEntry desired[1]; readInvItems(&f.top, desired, 0, 1, 0, false);
        GameWorld world; unsigned hand[5] = {0};
        applyContainerContents(&world, hand, desired, 1, false);
        CHECK("A01 locally truncated read never deletes", f.bag.removed == 0);
    }
    {
        Fixture f; f.add(f.bag, &f.first, 7);
        f.otherParent.data = &f.bagData; f.top._allItems.push_back(&f.otherParent);
        f.add(f.otherBag, &f.otherSection, 3);
        InvItemEntry desired[2]; readInvItems(&f.top, desired, 0, 2, 0, false);
        GameWorld world; unsigned hand[5] = {0};
        applyContainerContents(&world, hand, desired, 2, false);
        CHECK("A01 ambiguous identical bags are never arbitrarily cleared", f.bag.removed == 0 && f.otherBag.removed == 0);
    }
    {
        Fixture f; f.add(f.bag, &f.first, 7);
        for (unsigned i = 1; i < INV_ITEMS_MAX; ++i) f.add(f.top, 0);
        f.otherParent.data = &f.bagData; f.top._allItems.push_back(&f.otherParent);
        InvItemEntry desired[1]; readInvItems(&f.top, desired, 0, 1, 0, false);
        GameWorld world; unsigned hand[5] = {0};
        applyContainerContents(&world, hand, desired, 1, false);
        CHECK("A01 incomplete parent enumeration cannot authorize empty clearing", f.bag.removed == 0);
    }
    {
        Fixture f; f.add(f.bag, &f.first, 7);
        InvItemEntry desired[2]; readInvItems(&f.top, desired, 0, 1, 0, false);
        std::memset(&desired[1], 0, sizeof(desired[1]));
        desired[1].parentIdx = 3; // absent parent
        GameWorld world; unsigned hand[5] = {0};
        applyContainerContents(&world, hand, desired, 2, false);
        CHECK("A01 invalid parent graph performs no destructive writes", f.bag.removed == 0 && f.top.removed == 0);
    }
}
int main(int argc, char** argv) {
    captureTests();
    if (argc < 2 || std::strcmp(argv[1], "capture") != 0) emptyTests();
    std::printf("inventory regressions: %d/%d PASS\n", total - failures, total);
    return failures ? 1 : 0;
}
