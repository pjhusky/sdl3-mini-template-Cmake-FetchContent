/*
 * pawn_host.c
 *
 * pawnc is compiled with NO_MAIN, which strips the default FILE*-based
 * implementations of every pc_* callback from sc1.c.  The host application
 * must supply them.  These implementations mirror what the standalone pawncc
 * executable uses, except we cache filenames ourselves so we never need to
 * reach back into the DLL for outfname / binfname.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* --- console / error output ----------------------------------------------- */

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

/* --- source file I/O ------------------------------------------------------- */

void *pc_opensrc(const char *filename)   { return fopen(filename, "r"); }
void *pc_createsrc(const char *filename) { return fopen(filename, "w"); }
void  pc_closesrc(void *handle)          { fclose((FILE *)handle); }

char *pc_readsrc(void *handle, unsigned char *target, int maxchars)
{
    return fgets((char *)target, maxchars, (FILE *)handle);
}

int pc_writesrc(void *handle, const unsigned char *source)
{
    return fputs((char *)source, (FILE *)handle) >= 0;
}

int pc_eofsrc(void *handle) { return feof((FILE *)handle); }

/* pawncc supports up to 4 simultaneously saved source positions */
#define PAWN_MAX_SRCPOS 4
static fpos_t        s_src_pos[PAWN_MAX_SRCPOS];
static unsigned char s_src_pos_used[PAWN_MAX_SRCPOS];

void pc_clearpossrc(void)
{
    memset(s_src_pos,      0, sizeof s_src_pos);
    memset(s_src_pos_used, 0, sizeof s_src_pos_used);
}

void *pc_getpossrc(void *handle, void *position)
{
    if (position == NULL) {
        int i;
        for (i = 0; i < PAWN_MAX_SRCPOS && s_src_pos_used[i]; i++)
            ;
        assert(i < PAWN_MAX_SRCPOS);
        if (i >= PAWN_MAX_SRCPOS) return NULL;
        position = &s_src_pos[i];
        s_src_pos_used[i] = 1;
    }
    fgetpos((FILE *)handle, (fpos_t *)position);
    return position;
}

void pc_resetsrc(void *handle, void *position)
{
    assert(handle != NULL && position != NULL);
    fsetpos((FILE *)handle, (fpos_t *)position);
}

/* --- intermediate assembly file I/O --------------------------------------- */

static char s_asm_path[512];

void *pc_openasm(const char *filename)
{
    strncpy(s_asm_path, filename, sizeof s_asm_path - 1);
    s_asm_path[sizeof s_asm_path - 1] = '\0';
    return fopen(filename, "w+");
}

void pc_closeasm(void *handle, int deletefile)
{
    if (handle != NULL) fclose((FILE *)handle);
    if (deletefile)     remove(s_asm_path);
}

void pc_resetasm(void *handle)
{
    fflush((FILE *)handle);
    fseek((FILE *)handle, 0, SEEK_SET);
}

int   pc_writeasm(void *handle, const char *string)             { return fputs(string, (FILE *)handle) >= 0; }
char *pc_readasm(void *handle, char *string, int maxchars)      { return fgets(string, maxchars, (FILE *)handle); }

/* --- binary output file I/O ----------------------------------------------- */

static char s_bin_path[512];

void *pc_openbin(const char *filename)
{
    strncpy(s_bin_path, filename, sizeof s_bin_path - 1);
    s_bin_path[sizeof s_bin_path - 1] = '\0';
    return fopen(filename, "wb");
}

void pc_closebin(void *handle, int deletefile)
{
    fclose((FILE *)handle);
    if (deletefile) remove(s_bin_path);
}

void pc_resetbin(void *handle, long offset)
{
    fflush((FILE *)handle);
    fseek((FILE *)handle, offset, SEEK_SET);
}

int  pc_writebin(void *handle, const void *buffer, int size) { return (int)fwrite(buffer, 1, size, (FILE *)handle) == size; }
long pc_lengthbin(void *handle)                              { return ftell((FILE *)handle); }
