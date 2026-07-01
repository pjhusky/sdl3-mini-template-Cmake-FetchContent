# Pawn 4 Integration Fixes — Foreign C Function Calls on 64-bit Windows

A record of everything that had to be changed to get the in-memory Pawn scripting integration (`PAWN_HOST_MEMORY` mode) working correctly with 64-bit cells on MSVC/Windows.

---

## Fix 1 — `-o` flag syntax

**File:** `src/mini_pawn.h`

`pawncc`'s internal `option_value()` reads the output filename from the **same string** as the flag.
Passing `-o` and the filename as separate `argv` tokens leaves `oname = ""` and inserts the filename
as a second source file → `fatal error 100: cannot read from file`.

**File-based version** — concatenate into one token:
```cpp
// WRONG
char *argv[] = { "pawncc", src_path, "-o", amx_path };

// CORRECT
char o_flag[256];
snprintf(o_flag, sizeof o_flag, "-o%s", amx_path);
char *argv[] = { "pawncc", src_path, o_flag };
```

**Memory-based version** — omit `-o` entirely. `pc_openbin` ignores the filename; the output stays
in the `hbuf_t` heap buffer regardless.

---

## Fix 2 — `#pragma rational Float`

**File:** `src/mini_pawn.h` (script string)

Pawn 4 requires an explicit pragma before float literals are accepted.
Without it: `error 070: rational number support was not enabled`.

```pawn
// Add as the very first line of every script that uses Float:
#pragma rational Float
```

---

## Fix 3 — Force `PAWN_CELL_SIZE=64` on the libraries

**File:** `external/scripting/pawn/CMakeLists.txt`

`amx.h` auto-detects cell size via `__SIZEOF_POINTER__`, but **MSVC does not define that macro**.
Without an explicit override both `amx` and `pawnc` default to `PAWN_CELL_SIZE=32` (32-bit cells),
while the compiler with `-C64` writes magic `0xf1e1` (64-bit) but the runtime expects `0xf1e0`
(32-bit) → `AMX_ERR_FORMAT` on `amx_Init`.

Add after `add_subdirectory(...)`:
```cmake
# MSVC does not define __SIZEOF_POINTER__, so amx.h defaults to PAWN_CELL_SIZE=32.
# Force 64-bit cells on both libraries. PUBLIC propagates through pawn_bundle to
# the main executable (mini_pawn.h, pawn_host_mem.c).
if(TARGET amx)
    target_compile_definitions(amx PUBLIC PAWN_CELL_SIZE=64)
endif()
if(TARGET pawnc)
    target_compile_definitions(pawnc PUBLIC PAWN_CELL_SIZE=64)
endif()
```

The `PUBLIC` visibility is important: it propagates `PAWN_CELL_SIZE=64` through the `pawn_bundle`
interface target to every consumer (main executable, host files, `mini_pawn.h`).

---

## Fix 4 — Pass `-C64` to the Pawn compiler

**File:** `src/mini_pawn.h`

The Pawn compiler has its own **runtime variable** `pc_cellsize` (in `scvars.c`) that defaults to `4`
(32-bit), independent of the `PAWN_CELL_SIZE` C macro.  Without `-C64`, the compiler emits 32-bit
bytecode even when the AMX runtime is built for 64-bit cells.  Consequences:

| Symptom | Root cause |
|---|---|
| `AMX_ERR_FORMAT` from `amx_Init` | Compiler writes magic `0xf1e0` (32-bit); runtime expects `0xf1e1` (64-bit) |
| Stack corruption / crash in native calls | `modstk` in `sc4.c` uses `pc_cellsize=4` to clean the stack, but each push consumed 8 bytes → 4-byte leak per argument per call |
| Truncated host pointer in `params[1]` | `OP_PUSHR_PRI` stores a 64-bit host address but only 32-bit cells could hold it |

The `-C64` argument must be **concatenated** in a single argv token (same `option_value()` rule as `-o`):

```cpp
// Memory-based
char *argv[] = { (char*)"pawncc", (char*)"-C64", (char*)virt_name };
pc_compile(3, argv);

// File-based
char *argv[] = { (char*)"pawncc", (char*)"-C64", (char*)src_path, o_flag };
pc_compile(4, argv);
```

---

## Fix 5 — Packed string decoding in the `print` native

**File:** `src/mini_pawn.h`

Pawn 4 with 64-bit cells stores string literals **packed**: 8 characters per 64-bit cell, in
**MSB-first** order within each cell.  A manual `cstr[i] & 0xFF` loop reads only the
least-significant byte of each cell (the *last* character of each 8-char group) and produces
garbage output.

The canonical detection is via the `UNPACKEDMAX` sentinel already in `amx.h`:
```c
#define UNPACKEDMAX  (((cell)1 << (sizeof(cell)-1)*8) - 1)
// = 0x00FFFFFFFFFFFFFF for 64-bit cells
// First cell of a packed string has a char in its MSB → value > UNPACKEDMAX → packed
// First cell of an unpacked string holds one ASCII char  → value ≤ UNPACKEDMAX → unpacked
```

Use `amx_GetString` (part of the AMX runtime, declared in `amx.h`) which handles both formats:

```cpp
// WRONG — only works for unpacked strings
while (cstr[i]) buf[i] = (char)(cstr[i++] & 0xFF);

// CORRECT — handles packed and unpacked transparently
static cell AMX_NATIVE_CALL pawn_native_print(AMX *amx, const cell *params)
{
    (void)amx;
    // params[1] is the physical host address of the string (pushed via OP_PUSHR_PRI)
    const cell *cstr = reinterpret_cast<const cell *>(static_cast<intptr_t>(params[1]));
    char buf[1024];
    amx_GetString(buf, cstr, 0, sizeof buf);
    printf("%s", buf);
    return 0;
}
```
