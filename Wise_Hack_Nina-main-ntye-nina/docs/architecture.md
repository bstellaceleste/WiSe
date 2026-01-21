# Architecture Documentation
## Adaptive Binary Instrumentation Engine

### Table of Contents
1. [Overview](#overview)
2. [Design Principles](#design-principles)
3. [Component Details](#component-details)
4. [Data Flow](#data-flow)
5. [Algorithms](#algorithms)
6. [Integration Points](#integration-points)

---

## Overview

The Adaptive Binary Instrumentation Engine implements a two-phase approach to dynamic program optimization:

1. **Phase 1: Profiling** - Collect runtime metrics with minimal overhead
2. **Phase 2: Adaptation** - Detect hotspots and apply optimizations dynamically

### Key Design Goals

- **Low Overhead**: <5% performance impact during profiling
- **Adaptive**: No hardcoded thresholds, works on any binary
- **Safe**: Atomic patching with rollback capability
- **Portable**: Clean separation between instrumentation and analysis

---

## Design Principles

### 1. Statistical Rigor

All detection thresholds are computed from observed data distribution:

```
threshold = mean + k × standard_deviation
```

Where `k` is derived from configurable sensitivity:
- High sensitivity (0.9) → k=1.2 → more hotspots detected
- Low sensitivity (0.1) → k=2.8 → fewer, high-confidence hotspots

### 2. Separation of Concerns

```
┌──────────────┐     ┌───────────────────┐     ┌──────────────┐
│   Metrics    │────▶│ Hotspot Detection │────▶│   Patching   │
│  Collection  │     │   (Statistical)   │     │ (Trampolines)│
└──────────────┘     └───────────────────┘     └──────────────┘
```

Each component is independent and testable.

### 3. Efficiency First

- Hash table for O(1) profile lookup
- Minimal instrumentation (only CALL and memory ops)
- Statistical computation amortized over time

---

## Component Details

### Config Module (`config.c/.h`)

**Responsibility:** Centralized configuration management

**Key Structures:**
```c
typedef struct {
    uint64_t call_threshold;      // Adaptive
    uint64_t memory_threshold;    // Adaptive
    float sensitivity;            // User-configurable
    char strategy[32];            // Detection strategy
    // ...
} EngineConfig;
```

**Design Choice:** All runtime behavior controlled via configuration, enabling easy experimentation.

### Metrics Module (`metrics.c/.h`)

**Responsibility:** Efficient storage and retrieval of runtime metrics

**Key Structures:**
```c
typedef struct {
    void *function_addr;
    uint64_t call_count;
    uint64_t last_call_timestamp;
    bool is_hotspot;
    // ...
} FunctionProfile;

typedef struct {
    FunctionProfile *func_profiles;
    MemoryProfile *mem_profiles;
    size_t capacity;
    // ...
} ProfileTable;
```

**Design Choice:**
- Hash table with linear probing for collision resolution
- Separate arrays for function and memory profiles (cache locality)
- O(1) average-case lookup critical for low overhead

**Hash Function:**
```c
size_t hash = (addr * 2654435761UL) % capacity;
```
Using Knuth's multiplicative hash for good distribution.

### Profiler Module (`profiler.c/.h`)

**Responsibility:** Intercept and record function calls and memory accesses

**Key Operations:**
1. `profiler_on_function_call()` - Called on every function entry
2. `profiler_on_memory_access()` - Called on memory operations
3. `profiler_is_stack_access()` - Distinguish stack from heap

**Stack Detection Algorithm:**
```c
bool is_stack_access(void *addr, void *rsp, void *rbp) {
    // Check if address is within stack frame range
    uintptr_t stack_min = min(rsp, rbp);
    uintptr_t stack_max = max(rsp, rbp) + STACK_BUFFER;
    return (addr >= stack_min && addr <= stack_max);
}
```

**Design Choice:** Conservative stack detection with 1MB buffer to avoid false negatives.

### Hotspot Detector Module (`hotspot_detector.c/.h`)

**Responsibility:** Identify performance-critical functions using statistical analysis

**Detection Algorithm:**
```
1. Compute statistics from all function profiles:
   - Mean call count
   - Standard deviation
   - 95th and 99th percentiles

2. Calculate adaptive thresholds:
   call_threshold = mean + k × stddev
   memory_threshold = mem_mean + k × mem_stddev

3. For each function:
   IF (calls > call_threshold OR memory_ops > memory_threshold)
      AND recently_active (< time_window):
      MARK as hotspot
```

**Strategy Variations:**
- **Conservative:** Use P99 threshold
- **Balanced:** Use mean + 2×stddev
- **Aggressive:** Use P95 threshold

**Design Choice:** Multiple detection criteria (call frequency OR memory intensity) catches different types of hotspots.

### Patcher Module (`patcher.c/.h`)

**Responsibility:** Install and manage code patches via trampolines

**Trampoline Installation:**
```
Step 1: mprotect(addr, PROT_READ|WRITE|EXEC)
Step 2: memcpy(saved_bytes, original, 16)
Step 3: Write JMP instruction:
        [0xE9] [32-bit offset]
Step 4: __builtin___clear_cache()
Step 5: mprotect(addr, PROT_READ|EXEC)
```

**Safety Mechanisms:**
1. Save original bytes for rollback
2. Verify jump offset fits in 32-bit
3. Atomic memcpy (5 bytes)
4. Cache flush to ensure coherency

**Design Choice:**
- 5-byte trampoline (JMP rel32) chosen for:
  - Minimal code modification
  - x86_64 compatibility
  - Reversibility

---

## Data Flow

### Phase 1: Profiling

```
Binary Execution
    │
    ├──▶ Function Call Detected
    │       └──▶ profiler_on_function_call()
    │              └──▶ profile_record_call()
    │                     └──▶ Update hash table
    │
    └──▶ Memory Access Detected
            └──▶ profiler_on_memory_access()
                   └──▶ profile_record_memory()
                          └──▶ Update memory profile
```

### Phase 2: Detection and Patching

```
End of Profiling
    │
    ├──▶ Compute Statistics
    │       └──▶ profile_compute_statistics()
    │              ├──▶ Calculate mean, stddev
    │              └──▶ Calculate percentiles
    │
    ├──▶ Update Detection Criteria
    │       └──▶ hotspot_update_criteria()
    │              └──▶ threshold = mean + k×stddev
    │
    ├──▶ Detect Hotspots
    │       └──▶ hotspot_detect_all()
    │              └──▶ Mark functions as hotspots
    │
    └──▶ Install Patches
            └──▶ For each hotspot:
                   └──▶ patcher_install_trampoline()
```

---

## Algorithms

### Statistical Computation

**Mean:**
```c
mean = sum(all_call_counts) / num_functions
```

**Standard Deviation:**
```c
variance = sum((count - mean)²) / num_functions
stddev = sqrt(variance)
```

**Percentile (Simple Sorting):**
```c
1. Sort all call counts
2. index = num_functions × percentile
3. return sorted_array[index]
```

**Optimization:** For large datasets, use quickselect O(n) instead of full sort O(n log n).

### Adaptive Threshold Calculation

```c
float sensitivity = config->sensitivity;  // 0.0 to 1.0

// Map sensitivity to statistical multiplier
// sensitivity=0.0 → k=3.0 (very conservative)
// sensitivity=0.5 → k=2.0 (balanced)
// sensitivity=1.0 → k=1.0 (very aggressive)
float k = 3.0 - (2.0 * sensitivity);

uint64_t threshold = (uint64_t)(mean + k * stddev);

// Ensure minimum threshold
if (threshold < MIN_THRESHOLD) {
    threshold = MIN_THRESHOLD;
}
```

**Rationale:**
- k=2.0 (95.4% confidence in normal distribution)
- k=3.0 (99.7% confidence)
- Adjustable for different use cases

---

## Integration Points

### DynamoRIO Integration (Future)

```c
// In DynamoRIO client
dr_emit_flags_t event_basic_block(void *drcontext, void *tag,
                                  instrlist_t *bb, ...) {
    instr_t *instr;
    for (instr = instrlist_first(bb); instr; instr = instr_get_next(instr)) {

        if (instr_is_call(instr)) {
            // Inject profiler callback
            dr_insert_clean_call(drcontext, bb, instr,
                                (void*)profiler_on_function_call,
                                false, 2,
                                OPND_CREATE_INTPTR(instr_get_app_pc(instr)),
                                OPND_CREATE_INTPTR(instr_get_return_target(instr)));
        }

        if (instr_reads_memory(instr) || instr_writes_memory(instr)) {
            // Inject memory tracker
            dr_insert_clean_call(drcontext, bb, instr,
                                (void*)profiler_on_memory_access,
                                false, 4,
                                OPND_CREATE_INTPTR(instr_get_app_pc(instr)),
                                opnd_create_reg(DR_REG_XAX),
                                OPND_CREATE_INT32(instr_reads_memory(instr)),
                                opnd_create_reg(DR_REG_XSP));
        }
    }

    return DR_EMIT_DEFAULT;
}
```

### Intel Pin Integration (Alternative)

```cpp
VOID Instruction(INS ins, VOID *v) {
    if (INS_IsCall(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE,
                      (AFUNPTR)profiler_on_function_call,
                      IARG_INST_PTR,
                      IARG_BRANCH_TARGET_ADDR,
                      IARG_END);
    }

    if (INS_IsMemoryRead(ins) || INS_IsMemoryWrite(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE,
                      (AFUNPTR)profiler_on_memory_access,
                      IARG_INST_PTR,
                      IARG_MEMORYREAD_EA,
                      IARG_BOOL, INS_IsMemoryRead(ins),
                      IARG_REG_VALUE, REG_STACK_PTR,
                      IARG_END);
    }
}
```

---

## Performance Considerations

### Overhead Analysis

**Sources of Overhead:**
1. Hash table lookup: O(1) average
2. Counter increment: O(1)
3. Timestamp: O(1) syscall
4. Memory classification: O(1) comparison

**Estimated Overhead:**
- Per function call: ~50-100 CPU cycles
- Per memory op: ~30-50 CPU cycles
- Typical overhead: 2-5% of execution time

### Optimization Techniques

1. **Sampling:** Instrument only every Nth call/access
2. **Batch Updates:** Buffer updates and flush periodically
3. **Lock-free Structures:** Use atomic operations for multi-threading
4. **Inline Assembly:** Critical path optimization

---

## Testing Strategy

### Unit Tests
- Test each module independently
- Mock dependencies
- Validate edge cases

### Integration Tests
- Full pipeline with simulated data
- Verify JSON output format
- Check statistical accuracy

### System Tests
- Run on real binaries
- Measure actual overhead
- Verify generalization

---

## Conclusion

This architecture prioritizes:
1. **Correctness:** Statistical rigor in detection
2. **Performance:** Minimal overhead design
3. **Adaptability:** Zero hardcoded assumptions
4. **Safety:** Atomic operations and rollback

The modular design enables easy extension (e.g., adding new metrics, different patching strategies) while maintaining a clean separation of concerns.
