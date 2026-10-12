// Single-threaded queue tests only. This shim cannot validate Win32 locking,
// worker races, calling conventions or game memory access.
#ifndef KENSHICOOP_TEST_WINDOWS_H
#define KENSHICOOP_TEST_WINDOWS_H
struct CRITICAL_SECTION { int unused; };
inline void InitializeCriticalSection(CRITICAL_SECTION*) {}
inline void DeleteCriticalSection(CRITICAL_SECTION*) {}
inline void EnterCriticalSection(CRITICAL_SECTION*) {}
inline void LeaveCriticalSection(CRITICAL_SECTION*) {}
#endif
