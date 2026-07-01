/*
 * pawn_host_mem.c
 *
 * Fully in-memory replacement for pawn_host.c:
 *   - Source code supplied as a const char* (no .p file needed)
 *   - Intermediate assembly stays in a heap buffer (no temp file)
 *   - Compiled .amx stays in a heap buffer (no .amx file written)
 *
 * Public API (declare in your header or extern directly):
 *   void        pawn_mem_set_source(const char *virtual_name, const char *source)
 *   const void *pawn_mem_get_amx   (size_t *out_size)
 *   void        pawn_mem_free      (void)
 *
 * Usage:
 *   pawn_mem_set_source("script.p", my_pawn_source_text);
 *   char *argv[] = { (char*)"pawncc", (char*)"script.p" };
 *   pc_compile(2, argv);
 *   // Do NOT pass -o as two separate argv tokens: pawncc's option_value()
 *   // reads the value from the same string, so -o <sep> leaves oname="" and
 *   // the next token is inserted as a second source file → fatal error 100.
 *   // Either omit -o (pawncc derives .amx from the .p name, pc_openbin
 *   // ignores it anyway) or concatenate: "-oscript.amx".
 *   size_t size; const void *amx = pawn_mem_get_amx(&size);
 *   // pass amx to pawn_mem_init_amx() -- see mini_pawn.h
 *
 * Note: #include files (e.g. default.inc) still fall back to FILE* so they
 *       can be found on disk.  If pawnc can't find default.inc it just skips
 *       it -- that is fine for self-contained scripts.
 *
 * Link EITHER pawn_host.c OR pawn_host_mem.c -- never both (duplicate symbols).
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ==========================================================================
   Growable heap buffer — used for assembly (rw) and binary output (rw).
   wpos: current write position (advances on write; seekable via pc_resetbin)
   rpos: current read  position (used by pc_readasm after pc_resetasm rewinds)
   used: high-water mark of written bytes (= real content length)
   ========================================================================== */

typedef struct {
    char  *data;
    size_t cap;
    size_t used;
    size_t wpos;
    size_t rpos;
} hbuf_t;

static hbuf_t *hbuf_new(void)
{
    hbuf_t *b = (hbuf_t *)calloc(1, sizeof *b);
    b->cap  = 4096;
    b->data = (char *)malloc(b->cap);
    return b;
}

static void hbuf_free(hbuf_t *b)
{
    if (b) { free(b->data); free(b); }
}

static void hbuf_grow(hbuf_t *b, size_t need)
{
    while (b->wpos + need > b->cap) b->cap *= 2;
    b->data = (char *)realloc(b->data, b->cap);
}

static void hbuf_write(hbuf_t *b, const void *src, size_t n)
{
    if (n == 0) return;
    hbuf_grow(b, n);
    memcpy(b->data + b->wpos, src, n);
    b->wpos += n;
    if (b->wpos > b->used) b->used = b->wpos;
}

/* fgets-style line reader advancing rpos */
static char *hbuf_getline(hbuf_t *b, char *dst, int max)
{
    int i;
    if (b->rpos >= b->used) return NULL;
    for (i = 0; i < max - 1 && b->rpos < b->used; ) {
        char c = b->data[b->rpos++];
        dst[i++] = c;
        if (c == '\n') break;
    }
    dst[i] = '\0';
    return dst;
}

/* ==========================================================================
   In-memory source — read-only view of a caller-owned string.
   ========================================================================== */

typedef struct {
    const char *data;
    size_t      size;
    size_t      pos;
} memsrc_t;

/* Position slots: tagged union so we can hold either a byte offset (memory
   source) or an fpos_t (real FILE* for #include files).                   */
typedef struct {
    int is_mem;
    union {
        size_t mem;
        fpos_t fp;
    } u;
} srcpos_t;

#define MAX_SRCPOS 4
static srcpos_t      s_pos[MAX_SRCPOS];
static unsigned char s_pos_used[MAX_SRCPOS];

/* ==========================================================================
   Module-level state
   ========================================================================== */

static const char *s_virt_name = NULL; /* virtual filename for the source */
static memsrc_t    s_src;              /* in-memory source (not a pointer) */
static hbuf_t     *s_asm      = NULL;  /* assembly intermediate            */
static hbuf_t     *s_bin      = NULL;  /* compiled AMX output              */
static int         s_sink;             /* discard sentinel (pc_createsrc)  */

/* Handle discrimination helpers */
static int is_virt(void *h) { return h == &s_src;  }
static int is_sink(void *h) { return h == &s_sink; }

/* ==========================================================================
   Public API
   ========================================================================== */

void pawn_mem_set_source(const char *virtual_name, const char *source)
{
    s_virt_name = virtual_name;
    s_src.data  = source;
    s_src.size  = strlen(source);
    s_src.pos   = 0;
}

/* Returns the compiled AMX bytes (heap-owned; valid until next pc_compile
   call or pawn_mem_free).  Returns NULL if compilation was never run or
   failed with a fatal error.                                               */
const void *pawn_mem_get_amx(size_t *out_size)
{
    if (!s_bin) { if (out_size) *out_size = 0; return NULL; }
    if (out_size) *out_size = s_bin->used;
    return s_bin->data;
}

/* Release all internal buffers.  Call this when you no longer need the AMX
   bytes (e.g. after loading them into an AMX struct).                      */
void pawn_mem_free(void)
{
    hbuf_free(s_asm); s_asm = NULL;
    hbuf_free(s_bin); s_bin = NULL;
    s_virt_name = NULL;
    s_src.data  = NULL;
    s_src.size  = 0;
    s_src.pos   = 0;
}

/* ==========================================================================
   Console / error output (identical to pawn_host.c)
   ========================================================================== */

int pc_printf(const char *message, ...)
{
    int ret;
    va_list ap;
    va_start(ap, message);
    ret = vprintf(message, ap);
    va_end(ap);
    fflush(stdout);
    return ret;
}

int pc_error(int number, const char *message, const char *filename,
             int firstline, int lastline, va_list argptr)
{
    static const char *prefix[3] = { "error", "fatal error", "warning" };
    if (number != 0) {
        const char *pre = prefix[number / 100];
        if (firstline >= 0)
            fprintf(stderr, "%s(%d -- %d) : %s %03d: ",
                    filename, firstline, lastline, pre, number);
        else
            fprintf(stderr, "%s(%d) : %s %03d: ",
                    filename, lastline, pre, number);
    }
    vfprintf(stderr, message, argptr);
    fflush(stderr);
    return 0;
}

/* ==========================================================================
   Source file I/O
   ========================================================================== */

void *pc_opensrc(const char *filename)
{
    /* Virtual source: return address of our memsrc_t */
    if (s_virt_name && strcmp(filename, s_virt_name) == 0) {
        s_src.pos = 0;
        return &s_src;
    }
    /* #include files (e.g. default.inc): fall back to disk */
    return fopen(filename, "r");
}

/* pc_createsrc: used for -r (cross-reference) output; discard silently */
void *pc_createsrc(const char *filename) { (void)filename; return &s_sink; }

void pc_closesrc(void *handle)
{
    if (is_virt(handle) || is_sink(handle)) return;
    fclose((FILE *)handle);
}

char *pc_readsrc(void *handle, unsigned char *target, int maxchars)
{
    if (is_virt(handle)) {
        int i;
        memsrc_t *ms = (memsrc_t *)handle;
        if (ms->pos >= ms->size) return NULL;
        for (i = 0; i < maxchars - 1 && ms->pos < ms->size; ) {
            char c = ms->data[ms->pos++];
            ((char *)target)[i++] = c;
            if (c == '\n') break;
        }
        ((char *)target)[i] = '\0';
        return (char *)target;
    }
    if (is_sink(handle)) return NULL;
    return fgets((char *)target, maxchars, (FILE *)handle);
}

int pc_writesrc(void *handle, const unsigned char *source)
{
    /* Only called for -r output; discard for sink, ignore for everything else */
    (void)handle; (void)source;
    return 1;
}

int pc_eofsrc(void *handle)
{
    if (is_virt(handle)) { memsrc_t *ms = (memsrc_t *)handle; return (int)(ms->pos >= ms->size); }
    if (is_sink(handle)) return 1;
    return feof((FILE *)handle);
}

void pc_clearpossrc(void)
{
    memset(s_pos,      0, sizeof s_pos);
    memset(s_pos_used, 0, sizeof s_pos_used);
}

void *pc_getpossrc(void *handle, void *position)
{
    srcpos_t *p;
    if (position == NULL) {
        int i;
        for (i = 0; i < MAX_SRCPOS && s_pos_used[i]; i++)
            ;
        assert(i < MAX_SRCPOS);
        if (i >= MAX_SRCPOS) return NULL;
        position       = &s_pos[i];
        s_pos_used[i]  = 1;
    }
    p = (srcpos_t *)position;
    if (is_virt(handle)) {
        p->is_mem   = 1;
        p->u.mem    = ((memsrc_t *)handle)->pos;
    } else {
        p->is_mem   = 0;
        fgetpos((FILE *)handle, &p->u.fp);
    }
    return position;
}

void pc_resetsrc(void *handle, void *position)
{
    srcpos_t *p;
    assert(handle != NULL && position != NULL);
    p = (srcpos_t *)position;
    if (is_virt(handle))
        ((memsrc_t *)handle)->pos = p->u.mem;
    else
        fsetpos((FILE *)handle, &p->u.fp);
}

/* ==========================================================================
   Intermediate assembly file I/O — always in-memory
   ========================================================================== */

void *pc_openasm(const char *filename)
{
    (void)filename;
    hbuf_free(s_asm);
    s_asm = hbuf_new();
    return s_asm;
}

void pc_closeasm(void *handle, int deletefile)
{
    (void)handle;
    if (deletefile) { hbuf_free(s_asm); s_asm = NULL; }
    /* If not deleting, keep s_asm alive for the read pass that follows */
}

/* pc_resetasm: rewind read cursor so the assembler can read back what it wrote */
void pc_resetasm(void *handle)
{
    ((hbuf_t *)handle)->rpos = 0;
}

int pc_writeasm(void *handle, const char *string)
{
    hbuf_write((hbuf_t *)handle, string, strlen(string));
    return 1;
}

char *pc_readasm(void *handle, char *string, int maxchars)
{
    return hbuf_getline((hbuf_t *)handle, string, maxchars);
}

/* ==========================================================================
   Binary output — always in-memory
   ========================================================================== */

void *pc_openbin(const char *filename)
{
    (void)filename;
    hbuf_free(s_bin);
    s_bin = hbuf_new();
    return s_bin;
}

void pc_closebin(void *handle, int deletefile)
{
    (void)handle;
    if (deletefile) { hbuf_free(s_bin); s_bin = NULL; }
    /* If not deleting, keep s_bin alive so pawn_mem_get_amx() can retrieve it */
}

/* pc_resetbin: random-access seek for write cursor (the assembler patches
   earlier fields, e.g. total size, after writing the whole binary)        */
void pc_resetbin(void *handle, long offset)
{
    ((hbuf_t *)handle)->wpos = (size_t)offset;
}

int pc_writebin(void *handle, const void *buffer, int size)
{
    hbuf_write((hbuf_t *)handle, buffer, (size_t)size);
    return 1;
}

/* pc_lengthbin: return current write position (used by assembler to know
   how far ahead to patch jump targets)                                    */
long pc_lengthbin(void *handle)
{
    return (long)((hbuf_t *)handle)->wpos;
}
