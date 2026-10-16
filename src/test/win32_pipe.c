// SPDX-License-Identifier: LGPL-2.1-or-later
/* Windows anonymous-pipe emulation must work in simultaneous processes. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#undef NDEBUG
#include <assert.h>

#include <vlc_common.h>
#include <vlc_fs.h>
#include <windows.h>

static int open_pipes(int fds[2][2])
{
    for (unsigned i = 0; i < 2; ++i)
    {
        if (vlc_pipe(fds[i]) != 0)
        {
            fprintf(stderr, "pipe %u creation failed: Windows error %lu\n",
                    i, (unsigned long) GetLastError());
            while (i > 0)
            {
                --i;
                vlc_close(fds[i][0]);
                vlc_close(fds[i][1]);
            }
            return 1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int fds[2][2];
    if (open_pipes(fds) != 0)
        return 1;

    if (argc == 1)
    {
        WCHAR executable[MAX_PATH];
        DWORD length = GetModuleFileNameW(NULL, executable, ARRAY_SIZE(executable));
        assert(length > 0 && length < ARRAY_SIZE(executable));
        WCHAR command[] = L"test_win32_pipe.exe --child";
        STARTUPINFOW startup = { .cb = sizeof(startup) };
        PROCESS_INFORMATION child;
        assert(CreateProcessW(executable, command, NULL, NULL, FALSE,
                              CREATE_NO_WINDOW, NULL, NULL, &startup, &child));
        assert(WaitForSingleObject(child.hProcess, 5000) == WAIT_OBJECT_0);
        DWORD status;
        assert(GetExitCodeProcess(child.hProcess, &status));
        CloseHandle(child.hProcess);
        CloseHandle(child.hThread);
        assert(status == 0);
    }
    else
        assert(argc == 2 && strcmp(argv[1], "--child") == 0);

    for (unsigned i = 0; i < 2; ++i)
    {
        vlc_close(fds[i][0]);
        vlc_close(fds[i][1]);
    }
    puts("Concurrent Windows processes create independent pipes.");
    return 0;
}
