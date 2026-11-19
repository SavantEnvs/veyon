// Leak detection is not the bug class fuzzed here; ASan/UBSan stay fully active.
extern "C" int __lsan_is_turned_off() { return 1; }
