#pragma once

extern "C" {
    #include <amx.h>      // AMX runtime VM
    #include <amxaux.h>   // aux_LoadProgram / aux_FreeProgram
    #include <sc.h>       // pc_compile  (requires pawn_host_mem.c OR pawn_host.c linked)

#ifdef PAWN_HOST_MEMORY
    /* pawn_host_mem.c public API — only available when PAWN_HOST_MEMORY is set */
    void        pawn_mem_set_source(const char *virtual_name, const char *source);
    const void *pawn_mem_get_amx   (size_t *out_size);
    void        pawn_mem_free      (void);
#endif
}

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mini_native_functions.h"

// ---------------------------------------------------------------------------
// Native: print(const str[])
// ---------------------------------------------------------------------------
static cell AMX_NATIVE_CALL pawn_native_print(AMX *amx, const cell *params)
{
    /* Pawn 4 passes string args via OP_PUSHR_PRI: params[1] is the physical
       host address of the string in the AMX data segment.  Strings may be
       packed (8 chars per 64-bit cell, MSB-first) or unpacked (1 char per
       cell).  amx_GetString handles both formats via the UNPACKEDMAX test. */
    (void)amx;
    const cell *cstr = reinterpret_cast<const cell *>(static_cast<intptr_t>(params[1]));
    char buf[1024];
    amx_GetString(buf, cstr, 0, sizeof buf);
    printf("%s", buf);
    return 0;
}

// ---------------------------------------------------------------------------
// Native: intAdd(a, b)  — delegates to nativeC_intAdd
// ---------------------------------------------------------------------------
static cell AMX_NATIVE_CALL pawn_native_intAdd(AMX *amx, const cell *params)
{
    (void)amx;
    return (cell)nativeC_intAdd((int)params[1], (int)params[2]);
}

// ---------------------------------------------------------------------------
// Native: Float:floatAdd(Float:a, Float:b)  — delegates to nativeC_floatAdd
// Pawn stores Float-tagged values as IEEE 754 bits inside a cell.
// memcpy is used for the cell↔float conversion to avoid strict-aliasing UB
// and to sidestep the REAL typedef which is an implementation detail of the
// amx_ctof/amx_ftoc macros and may not be exposed as a standalone name.
// ---------------------------------------------------------------------------
static cell AMX_NATIVE_CALL pawn_native_floatAdd(AMX *amx, const cell *params)
{
    (void)amx;
    /* With PAWN_CELL_SIZE==64, amx_ctof/amx_ftoc use double, not float.
       Cast to float for nativeC_floatAdd, then widen back to double before
       storing into the cell.  Use memcpy to avoid strict-aliasing UB. */
    float a = static_cast<float>(amx_ctof(params[1]));
    float b = static_cast<float>(amx_ctof(params[2]));
    float r = nativeC_floatAdd(a, b);
#if PAWN_CELL_SIZE == 64
    double rd = static_cast<double>(r);
    cell rc;
    memcpy(&rc, &rd, sizeof rc);
    return rc;
#else
    return amx_ftoc(r);
#endif
}

// ---------------------------------------------------------------------------
// Native: print_int(n)    — prints an integer from a Pawn script
// Native: print_float(f)  — prints a float   from a Pawn script
// ---------------------------------------------------------------------------
static cell AMX_NATIVE_CALL pawn_native_print_int(AMX *amx, const cell *params)
{
    (void)amx;
    printf("%d\n", (int)params[1]);
    return 0;
}

static cell AMX_NATIVE_CALL pawn_native_print_float(AMX *amx, const cell *params)
{
    (void)amx;
    /* amx_ctof returns double (64-bit cells) or float (32-bit cells);
       cast to double in either case for printf's %g. */
    printf("%g\n", (double)amx_ctof(params[1]));
    return 0;
}

static const AMX_NATIVE_INFO pawn_natives[] = {
    { "print",       pawn_native_print       },
    { "intAdd",      pawn_native_intAdd      },
    { "floatAdd",    pawn_native_floatAdd    },
    { "print_int",   pawn_native_print_int   },
    { "print_float", pawn_native_print_float },
    { nullptr,       nullptr                 }
};

// ---------------------------------------------------------------------------
#ifdef PAWN_HOST_MEMORY
// ---------------------------------------------------------------------------
// pawn_mem_init_amx()
//
// Loads the compiled AMX from pawn_mem_get_amx() into an AMX struct without
// touching the filesystem.  The header's stp field gives the total memory
// required (code + data + stack + heap); we allocate that, copy the file
// bytes into it, and call amx_Init().
// Call aux_FreeProgram(&amx) to release the buffer when done.
// ---------------------------------------------------------------------------
inline int pawn_mem_init_amx(AMX *amx)
{
    size_t file_size = 0;
    const void *file_data = pawn_mem_get_amx(&file_size);
    if (!file_data || file_size < sizeof(AMX_HEADER)) return AMX_ERR_FORMAT;

    const AMX_HEADER *hdr = reinterpret_cast<const AMX_HEADER *>(file_data);
    size_t total = static_cast<size_t>(hdr->stp);  /* stack top = total allocation */
    if (total < file_size) total = file_size;       /* guard against malformed header */

    void *prog = malloc(total);
    if (!prog) return AMX_ERR_MEMORY;
    memset(prog, 0, total);
    memcpy(prog, file_data, file_size);

    memset(amx, 0, sizeof *amx);
    int err = amx_Init(amx, prog);
    if (err != AMX_ERR_NONE) { free(prog); return err; }
    return AMX_ERR_NONE;
}

// ---------------------------------------------------------------------------
// mini_pawn_example_mem()
//
// Compiles a Pawn script and executes it entirely in memory:
//   - Source string  → in-memory read (no .p  file written)
//   - Assembly pass  → in-memory buffer (no .asm file written)
//   - AMX binary     → in-memory buffer (no .amx file written)
// Requires pawn_host_mem.c linked (PAWN_HOST_MEMORY=ON).
// ---------------------------------------------------------------------------
inline void mini_pawn_example_mem()
{
    printf( "\n\t--- mem-based Pawn!!! ---\n" );

    const char *virt_name = "mem_script.p";

    const char *script =
        "#pragma rational Float\n"
        "native intAdd(a, b);\n"
        "native Float:floatAdd(Float:a, Float:b);\n"
        "native print(const str[]);\n"
        "native print_int(n);\n"
        "native print_float(Float:f);\n"
        "\n"
        "main()\n"
        "{\n"
        "    new Float:r2 = floatAdd(1.5, 2.25);\n"
        "    print(\"[pawn-mem] floatAdd(1.5, 2.25) = \");\n"
        "    print_float(r2);\n"
        "\n"
        "    new r1 = intAdd(7, 5);\n"
        "    print(\"[pawn-mem] intAdd(7, 5) = \");\n"
        "    print_int(r1);\n"
        "\n"
        "    print(\"\\n\");\n"
        "    print(\"pawn>> float ret val = \");\n"
        "    print_float(r2);\n"
        "    print(\"pawn>> int ret val = \");\n"
        "    print_int(r1);\n"
        "    return r1;\n"
        "}\n";

    // --- 1. Register the source string with the in-memory host ---
    pawn_mem_set_source(virt_name, script);

    // --- 2. Compile (source read from memory; asm and amx stay on heap) ---
    // No -o flag: pawncc derives "mem_script.amx" from the input name, and
    // pc_openbin ignores the filename anyway (all output stays in-memory).
    // NOTE: -o <file> as two separate args is NOT supported by pawncc — it
    // uses option_value() which reads the value from the same string, so the
    // form must be -ofile.amx (concatenated). Omitting -o avoids the issue.
    char *argv[] = { (char *)"pawncc", (char *)"-C64", (char *)virt_name };
    if (pc_compile(3, argv) != 0) {
        fprintf(stderr, "[pawn-mem] Compilation failed\n");
        pawn_mem_free();
        return;
    }

    // --- 3. Move compiled AMX from heap buffer into AMX struct ---
    AMX amx = {};
    int err = pawn_mem_init_amx(&amx);
    pawn_mem_free();   /* amx_Init copied bytes into amx.base; buffer no longer needed */

    if (err != AMX_ERR_NONE) {
        fprintf(stderr, "[pawn-mem] Load error: %s\n", aux_StrError(err));
        return;
    }

    // --- 4. Register natives and run main() ---
    amx_Register(&amx, pawn_natives, -1);

    cell retval = 0;
    err = amx_Exec(&amx, &retval, AMX_EXEC_MAIN);
    if (err != AMX_ERR_NONE)
        fprintf(stderr, "[pawn-mem] Exec error: %s\n", aux_StrError(err));
    else
        printf("[pawn-mem] main() returned: %lld\n", (long long)retval);

    aux_FreeProgram(&amx);
}

// ---------------------------------------------------------------------------
#else  /* PAWN_HOST_MEMORY not set → file-based host (pawn_host.c) */
// ---------------------------------------------------------------------------
// mini_pawn_example()
//
// Compiles a Pawn script via temp files and executes it:
//   pawn_sample.p   written to working directory, then removed
//   pawn_sample.amx written to working directory, then removed
// Requires pawn_host.c linked (PAWN_HOST_MEMORY=OFF).
// ---------------------------------------------------------------------------
inline void mini_pawn_example()
{
    printf( "\n\t--- file-based Pawn!!! ---\n" );

    const char *src_path = "pawn_sample.p";
    const char *amx_path = "pawn_sample.amx";

    const char *script =
        "#pragma rational Float\n"
        "native intAdd(a, b);\n"
        "native Float:floatAdd(Float:a, Float:b);\n"
        "native print(const str[]);\n"
        "native print_int(n);\n"
        "native print_float(Float:f);\n"
        "\n"
        "main()\n"
        "{\n"
        "    new Float:r2 = floatAdd(1.5, 2.25);\n"
        "    print(\"[pawn-file] floatAdd(1.5, 2.25) = \");\n"
        "    print_float(r2);\n"
        "\n"
        "    new r1 = intAdd(7, 5);\n"
        "    print(\"[pawn-file] intAdd(7, 5) = \");\n"
        "    print_int(r1);\n"
        "\n"
        "    print(\"\\n\");\n"
        "    print(\"pawn>> float ret val = \");\n"
        "    print_float(r2);\n"
        "    print(\"pawn>> int ret val = \");\n"
        "    print_int(r1);\n"
        "    return r1;\n"
        "}\n";

    // --- 1. Write source to a temp file ---
    FILE *f = fopen(src_path, "w");
    if (!f) { fprintf(stderr, "[pawn-file] Cannot write source file\n"); return; }
    fputs(script, f);
    fclose(f);

    // --- 2. Compile ---
    // pawncc option_value() reads the value from the SAME string as the flag,
    // so -o and the filename must be concatenated: "-opawn_sample.amx", NOT "-o" "file".
    char o_flag[256];
    snprintf(o_flag, sizeof o_flag, "-o%s", amx_path);
    char *argv[] = { (char *)"pawncc", (char *)"-C64", (char *)src_path, o_flag };
    if (pc_compile(4, argv) != 0) {
        fprintf(stderr, "[pawn-file] Compilation failed\n");
        remove(src_path);
        return;
    }

    // --- 3. Load .amx (aux_LoadProgram allocates stack+heap space beyond file size) ---
    AMX amx = {};
    int err = aux_LoadProgram(&amx, (char *)amx_path, nullptr);
    if (err != AMX_ERR_NONE) {
        fprintf(stderr, "[pawn-file] Load error: %s\n", aux_StrError(err));
        remove(src_path); remove(amx_path);
        return;
    }

    // --- 4. Register natives and run main() ---
    amx_Register(&amx, pawn_natives, -1);

    cell retval = 0;
    err = amx_Exec(&amx, &retval, AMX_EXEC_MAIN);
    if (err != AMX_ERR_NONE)
        fprintf(stderr, "[pawn-file] Exec error: %s\n", aux_StrError(err));
    else
        printf("[pawn-file] main() returned: %lld\n", (long long)retval);

    aux_FreeProgram(&amx);
    remove(src_path);
    remove(amx_path);
}

#endif /* PAWN_HOST_MEMORY */
