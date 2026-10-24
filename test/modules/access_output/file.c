// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_block.h>
#include <vlc_fs.h>
#include <vlc_modules.h>
#include <vlc_sout.h>
#include <fcntl.h>
#include "../../libvlc/test.h"
#include "../../../lib/libvlc_internal.h"

static unsigned failures;

static void check(const char *name, bool success)
{
    printf("%s: %s\n", success ? "PASS" : "FAIL", name);
    failures += !success;
}

static block_t *bytes(const char *text)
{
    size_t size = strlen(text);
    block_t *block = block_Alloc(size);
    assert(block);
    if (size)
        memcpy(block->p_buffer, text, size);
    return block;
}

static sout_access_out_t *open_fd(libvlc_instance_t *vlc, int fd)
{
    char number[32];
    snprintf(number, sizeof(number), "%d", fd);
    sout_access_out_t *access = sout_AccessOutNew(vlc->p_libvlc_int, "fd", number);
    assert(access && strcmp(module_get_object(access->p_module), "file") == 0);
    return access;
}

int main(void)
{
    if (getenv("VLC_PLUGIN_PATH") == NULL)
        test_init();
    const char *args[] = {"--ignore-config", "--no-media-library", "--quiet"};
    libvlc_instance_t *vlc = libvlc_new(ARRAY_SIZE(args), args);
    assert(vlc);

    /* A relative template keeps native runs in their owned working directory.
     * The fd shortcut loads the real file module and duplicates this regular
     * file descriptor, avoiding a close/reopen race in temporary storage. */
    char path[] = "vlc-sout-file-XXXXXX";
    int fd = vlc_mkstemp(path);
    assert(fd >= 0);
    sout_access_out_t *access = open_fd(vlc, fd);

    check("NULL chain", sout_AccessOutWrite(access, NULL) == 0);
    check("single empty block", sout_AccessOutWrite(access, bytes("")) == 0);
    block_t *chain = bytes("");
    block_ChainAppend(&chain, bytes(""));
    block_ChainAppend(&chain, bytes(""));
    check("all-empty chain", sout_AccessOutWrite(access, chain) == 0);
    check("data after empty writes", sout_AccessOutWrite(access, bytes("AB")) == 2);

    chain = bytes("");
    block_ChainAppend(&chain, bytes("C"));
    block_ChainAppend(&chain, bytes(""));
    block_ChainAppend(&chain, bytes("DE"));
    block_ChainAppend(&chain, bytes(""));
    check("leading/interleaved/trailing empty blocks",
          sout_AccessOutWrite(access, chain) == 3);

    chain = bytes("F");
    block_ChainAppend(&chain, bytes(""));
    block_ChainAppend(&chain, bytes("G"));
    check("empty block must not discard later payload",
          sout_AccessOutWrite(access, chain) == 2);
    sout_AccessOutDelete(access);

    char output[16] = {0};
    assert(lseek(fd, 0, SEEK_SET) == 0);
    ssize_t count = read(fd, output, sizeof(output));
    check("exact complete output and caller-owned descriptor survives close",
          count == 7 && memcmp(output, "ABCDEFG", 7) == 0);
    vlc_close(fd);

    fd = vlc_open(path, O_RDONLY);
    assert(fd >= 0);
    access = open_fd(vlc, fd);
    chain = bytes("H");
    block_ChainAppend(&chain, bytes("I"));
    check("genuine write failure remains an error",
          sout_AccessOutWrite(access, chain) == -1);
    sout_AccessOutDelete(access);
    vlc_close(fd);
    assert(vlc_unlink(path) == 0);
    libvlc_release(vlc);
    printf("RESULT: 8 checks, %u failures\n", failures);
    return failures ? 1 : 0;
}
