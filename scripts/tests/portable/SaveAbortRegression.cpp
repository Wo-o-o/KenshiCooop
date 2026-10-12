// Original sender-abort/receiver-DONE functions with file operations observed,
// not executed. These tests must never touch real saves or runtime dependencies.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "src/netproto/Wire.h"
#include "src/netproto/ContentHash.h"
#define _snprintf snprintf
typedef unsigned int DWORD;
const DWORD INVALID_FILE_ATTRIBUTES = ~0u;
const int MOVEFILE_WRITE_THROUGH = 1;
static unsigned moved = 0, priorDeleted = 0, stagingDeleted = 0;
static DWORD GetTickCount() { return 1000; }
static DWORD GetFileAttributesA(const char*) { return INVALID_FILE_ATTRIBUTES; }
static bool MoveFileExA(const char*, const char*, int) { ++moved; return true; }
namespace coop {
static void logLine(const char*) {}
static void logErrLine(const char*) {}
struct NetLink {
    std::vector<SaveDoneHeader> done;
    void queueSaveDone(const SaveDoneHeader& header, const u32*, unsigned count) {
        if (count) throw 1; done.push_back(header);
    }
};
namespace savexfer {
static u32 g_sendXferId = 17;
static bool g_sendActive = true;
static std::vector<int> g_sendFiles(2, 1);
static std::vector<u32> g_sendCrcs(2, 1);
static unsigned senderClosed = 0, receiverClosed = 0;
static void sendCloseFile() { ++senderClosed; }
static void recvCloseFile() { ++receiverClosed; }
static bool g_recvActive = true;
static u32 g_recvXferId = 17;
static u16 g_recvFileCount = 2;
static unsigned long long g_recvBytes = 3 * 1024 * 1024;
static std::string g_recvName = "existing", g_recvStaging = "incoming";
static unsigned long g_recvStartTick = 0;
static std::vector<u8> g_recvSeen(2, 1);
static std::vector<u32> g_recvCrcs(2, 1);
static int g_lastCommitResult = -1;
static u32 g_commitSeq = 0;
static std::string saveFolderFor(const std::string& name) { return name; }
static void removeTree(const std::string& path, int) {
    if (path == g_recvStaging) ++stagingDeleted; else ++priorDeleted;
}
#include "Save-abort-under-test.inc"
} }
using namespace coop;
using namespace coop::savexfer;
static int total = 0, failures = 0;
#define CHECK(label, expr) do { ++total; if (!(expr)) { ++failures; std::printf("FAIL %s\n", label); } } while (0)
int main() {
    NetLink net; sendAbortAndNotify(net, 0, "read failed err=32 fixture");
    CHECK("sender abort closes IO and stops pumping", !g_sendActive && senderClosed == 1 && g_sendFiles.empty());
    CHECK("abort marker uses existing protocol-55 DONE", net.done.size() == 1 && net.done[0].type == PKT_SAVE_DONE && net.done[0].fileCount == 0 && net.done[0].xferId == 17);
    u16 files = 99; unsigned long long bytes = 99;
    int ok = onSaveDone(net.done[0], 0, &files, &bytes);
    CHECK("partial 3-MB transfer fails instead of committing", ok == 0 && files == 0 && bytes == 0);
    CHECK("receiver leaves waiting state", !g_recvActive && g_lastCommitResult == 0 && g_commitSeq == 1);
    CHECK("previous save never moved or deleted on abort", moved == 0 && priorDeleted == 0);
    CHECK("only partial staging discarded on explicit sender abort", stagingDeleted == 1);
    onSaveDone(net.done[0], 0, &files, &bytes);
    CHECK("duplicated abort marker is harmless", g_commitSeq == 1 && priorDeleted == 0);
    g_recvActive = true; g_recvXferId = 18;
    onSaveDone(net.done[0], 0, &files, &bytes);
    CHECK("delayed old abort cannot terminate newer transfer", g_recvActive && g_commitSeq == 1);
    abortReceive();
    CHECK("disconnect closes receiver without committing", !g_recvActive && g_commitSeq == 2 && receiverClosed >= 2);
    CHECK("disconnect preserves both prior save and diagnostic staging", priorDeleted == 0 && stagingDeleted == 1);
    abortReceive();
    CHECK("repeated teardown idempotent", g_commitSeq == 2);
    std::printf("save abort regressions: %d/%d PASS\n", total - failures, total); return failures ? 1 : 0;
}
