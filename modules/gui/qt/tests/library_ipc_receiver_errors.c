/* SPDX-License-Identifier: LGPL-2.1-or-later
 * Standalone regression source. Calls the extracted production LibraryOpenFiles
 * body with deterministic playlist/input failures; does not emulate Windows IPC.
 * test_library_ipc_errors.py extracts the current helper before compilation.
 */
#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VLC_SUCCESS 0
#define VLC_EINVAL (-22)
#define VLC_ENOMEM (-12)
#define VLC_INPUT_OPTION_TRUSTED 2
typedef intptr_t LRESULT;
typedef struct { uintptr_t dwData; uint32_t cbData; void *lpData; } COPYDATASTRUCT;
typedef struct input_item_t { char *uri; unsigned references; int options; } input_item_t;
typedef struct vlc_playlist_t {
    input_item_t *items[8]; size_t count; int current; bool locked;
} vlc_playlist_t;
typedef struct intf_thread_t { vlc_playlist_t playlist; } intf_thread_t;
enum fault { NONE, ARGS_ALLOC, ITEMS_ALLOC, SECOND_ITEM, OPTIONS,
             INSERT, GOTO, START };
static enum fault inject;
static int live_items, alloc_calls, new_calls, insert_calls, goto_calls;
static int start_calls, player_changes, logs, logged_error;
static int cases;

static char *duplicate(const char *text)
{
    char *out = malloc(strlen(text) + 1); assert(out);
    memcpy(out, text, strlen(text) + 1); return out;
}
static void *vlc_alloc(size_t count, size_t size)
{
    ++alloc_calls;
    if ((inject == ARGS_ALLOC && alloc_calls == 1) ||
        (inject == ITEMS_ALLOC && alloc_calls == 2)) return NULL;
    return calloc(count, size);
}
static char *vlc_path2uri(const char *filename, const char *scheme)
{
    (void)scheme; return duplicate(filename);
}
static input_item_t *input_item_New(const char *uri, const char *name)
{
    (void)name;
    if (++new_calls == 2 && inject == SECOND_ITEM) return NULL;
    input_item_t *item = calloc(1, sizeof(*item)); assert(item);
    item->uri = duplicate(uri); item->references = 1; ++live_items;
    return item;
}
static int input_item_AddOptions(input_item_t *item, int count,
                                const char *const *options, unsigned flags)
{
    assert(flags == VLC_INPUT_OPTION_TRUSTED);
    if (inject == OPTIONS) return -45;
    if (count) { assert(count == 1); assert(!strcmp(options[0], ":start-time=0")); }
    item->options = count; return VLC_SUCCESS;
}
static void input_item_Release(input_item_t *item)
{
    assert(item && item->references);
    if (--item->references == 0) { free(item->uri); free(item); --live_items; }
}
static vlc_playlist_t *vlc_intf_GetMainPlaylist(intf_thread_t *intf)
{ return &intf->playlist; }
static void vlc_playlist_Lock(vlc_playlist_t *playlist)
{ assert(!playlist->locked); playlist->locked = true; }
static void vlc_playlist_Unlock(vlc_playlist_t *playlist)
{ assert(playlist->locked); playlist->locked = false; }
static size_t vlc_playlist_Count(vlc_playlist_t *playlist)
{ assert(playlist->locked); return playlist->count; }
static int vlc_playlist_Insert(vlc_playlist_t *playlist, size_t index,
                               input_item_t *const *items, size_t count)
{
    assert(playlist->locked); ++insert_calls;
    if (inject == INSERT) return VLC_ENOMEM;
    assert(index <= playlist->count && playlist->count + count <= 8);
    memmove(playlist->items + index + count, playlist->items + index,
            (playlist->count - index) * sizeof(*items));
    for (size_t i = 0; i < count; ++i) {
        playlist->items[index + i] = items[i]; ++items[i]->references;
    }
    if (playlist->current >= (int)index) playlist->current += (int)count;
    playlist->count += count; return VLC_SUCCESS;
}
static void vlc_playlist_Remove(vlc_playlist_t *playlist, size_t index, size_t count)
{
    assert(playlist->locked && index + count <= playlist->count);
    for (size_t i = index; i < index + count; ++i) input_item_Release(playlist->items[i]);
    memmove(playlist->items + index, playlist->items + index + count,
            (playlist->count - index - count) * sizeof(*playlist->items));
    if (playlist->current >= (int)index) playlist->current = -1;
    playlist->count -= count;
}
static int vlc_playlist_GoTo(vlc_playlist_t *playlist, int index)
{
    assert(playlist->locked && index == 0); ++goto_calls; ++player_changes;
    /* Exercise a failure after player state changes, like SetCurrentMedia can. */
    if (inject == GOTO) return -71;
    playlist->current = index; return VLC_SUCCESS;
}
static int vlc_playlist_Start(vlc_playlist_t *playlist)
{
    assert(playlist->locked); ++start_calls;
    return inject == START ? -72 : VLC_SUCCESS;
}
static void msg_Err(intf_thread_t *intf, const char *format, ...)
{
    (void)intf; assert(strstr(format, "failed (%d)")); ++logs;
    va_list ap; va_start(ap, format); logged_error = va_arg(ap, int); va_end(ap);
}

#include "receiver-under-test.inc"

static void setup(intf_thread_t *intf, enum fault failure)
{
    assert(live_items == 0); memset(intf, 0, sizeof(*intf)); inject = NONE;
    new_calls = 0;
    intf->playlist.items[0] = input_item_New("file:///old-a.wav", NULL);
    intf->playlist.items[1] = input_item_New("file:///old-b.wav", NULL);
    intf->playlist.count = 2; intf->playlist.current = 1;
    inject = failure;
    alloc_calls = new_calls = insert_calls = goto_calls = start_calls = 0;
    player_changes = logs = logged_error = 0;
}
static void cleanup(intf_thread_t *intf)
{
    assert(!intf->playlist.locked);
    for (size_t i = 0; i < intf->playlist.count; ++i) input_item_Release(intf->playlist.items[i]);
    assert(live_items == 0); ++cases;
}
static COPYDATASTRUCT payload(int mode, bool invalid)
{
    const char *args[] = { "file:///new-a.wav", ":start-time=0", "file:///new-b.wav" };
    size_t size = sizeof(vlc_ipc_data_t);
    for (size_t i = 0; i < 3; ++i) size += sizeof(size_t) + strlen(args[i]) + 1;
    vlc_ipc_data_t *data = calloc(1, size + invalid); assert(data);
    data->argc = 3; data->enqueue = mode;
    size_t at = 0;
    for (size_t i = 0; i < 3; ++i) {
        size_t length = strlen(args[i]) + 1;
        memcpy(data->data + at, &length, sizeof(length)); at += sizeof(length);
        memcpy(data->data + at, args[i], length); at += length;
    }
    return (COPYDATASTRUCT){0, (uint32_t)(size + invalid), data};
}
static void unchanged(const intf_thread_t *intf)
{
    assert(intf->playlist.count == 2 && intf->playlist.current == 1);
    assert(!strcmp(intf->playlist.items[0]->uri, "file:///old-a.wav"));
    assert(!strcmp(intf->playlist.items[1]->uri, "file:///old-b.wav"));
    assert(!goto_calls && !start_calls && !player_changes);
    assert(live_items == 2);
}
static void replaced(const intf_thread_t *intf)
{
    assert(intf->playlist.count == 2 && insert_calls == 1);
    assert(!strcmp(intf->playlist.items[0]->uri, "file:///new-a.wav"));
    assert(!strcmp(intf->playlist.items[1]->uri, "file:///new-b.wav"));
    assert(intf->playlist.items[0]->options == 1 && intf->playlist.items[1]->options == 0);
    assert(live_items == 2);
}
int main(void)
{
    intf_thread_t intf;
    for (int mode = 2; mode <= 3; ++mode) {
        for (enum fault f = ARGS_ALLOC; f <= INSERT; ++f) {
            setup(&intf, f); COPYDATASTRUCT message = payload(mode, false);
            assert(LibraryOpenFiles(&intf, &message) == LIBRARY_IPC_REJECTED);
            unchanged(&intf); free(message.lpData); cleanup(&intf);
        }
        setup(&intf, NONE); COPYDATASTRUCT invalid = payload(mode, true);
        assert(LibraryOpenFiles(&intf, &invalid) == LIBRARY_IPC_REJECTED);
        unchanged(&intf); assert(!insert_calls); free(invalid.lpData); cleanup(&intf);
    }
    for (enum fault f = NONE; ; f = f == NONE ? GOTO : START) {
        setup(&intf, f); COPYDATASTRUCT message = payload(2, false);
        const LRESULT expected = f == NONE ? LIBRARY_IPC_ACCEPTED : LIBRARY_IPC_ACCEPTED_PLAYBACK_FAILED;
        assert(LibraryOpenFiles(&intf, &message) == expected); replaced(&intf);
        assert(goto_calls == 1 && player_changes == 1);
        assert(start_calls == (f == GOTO ? 0 : 1));
        if (f == NONE) assert(!logs && intf.playlist.current == 0);
        else { assert(logs == 1); assert(logged_error == (f == GOTO ? -71 : -72)); }
        free(message.lpData); cleanup(&intf); if (f == START) break;
    }
    /* Append preserves selection and never invokes GoTo/Start, even if those
     * operations would fail. The complete prior two-item queue stays intact. */
    setup(&intf, GOTO); COPYDATASTRUCT append = payload(3, false);
    assert(LibraryOpenFiles(&intf, &append) == LIBRARY_IPC_ACCEPTED);
    assert(intf.playlist.count == 4 && intf.playlist.current == 1);
    assert(!strcmp(intf.playlist.items[0]->uri, "file:///old-a.wav"));
    assert(!strcmp(intf.playlist.items[1]->uri, "file:///old-b.wav"));
    assert(!strcmp(intf.playlist.items[2]->uri, "file:///new-a.wav"));
    assert(!strcmp(intf.playlist.items[3]->uri, "file:///new-b.wav"));
    assert(!goto_calls && !start_calls && !player_changes && !logs && insert_calls == 1);
    free(append.lpData); cleanup(&intf);
    printf("receiver branch cases: %d\n", cases); return 0;
}
