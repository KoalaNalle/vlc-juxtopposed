/*****************************************************************************
 * file.c: configuration file handling
 *****************************************************************************
 * Copyright (C) 2001-2007 VLC authors and VideoLAN
 *
 * Authors: Gildas Bazin <gbazin@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <errno.h>                                                  /* errno */
#include <assert.h>
#include <limits.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifdef __APPLE__
#   include <xlocale.h>
#elif defined(HAVE_USELOCALE)
#include <locale.h>
#endif
#include <unistd.h>

#include <vlc_common.h>
#include "../libvlc.h"
#include <vlc_charset.h>
#include <vlc_fs.h>
#include <vlc_actions.h>
#ifdef _WIN32
# include <windows.h>
# include <io.h>
#endif
#include <vlc_modules.h>
#include <vlc_plugin.h>

#include "configuration.h"
#include "modules/modules.h"
#include "misc/rcu.h"

/**
 * Get the user's configuration file
 */
static char *config_GetConfigFile( libvlc_int_t *obj )
{
    char *psz_file = var_InheritString( obj, "config" );
    if( psz_file == NULL )
    {
        char *psz_dir = config_GetUserDir( VLC_CONFIG_DIR );
        if (psz_dir == NULL)
            return NULL;

        if( asprintf( &psz_file, "%s" DIR_SEP CONFIG_FILE, psz_dir ) == -1 )
            psz_file = NULL;
        free( psz_dir );
    }
    return psz_file;
}

static FILE *config_OpenForMerge(const char *path, bool *missing);

static FILE *config_OpenConfigFile( libvlc_int_t *p_obj )
{
    char *psz_filename = config_GetConfigFile( p_obj );
    if( psz_filename == NULL )
        return NULL;

    msg_Dbg( p_obj, "opening config file (%s)", psz_filename );

    bool missing;
    FILE *p_stream = config_OpenForMerge(psz_filename, &missing);
    if( p_stream == NULL && !missing )
    {
        msg_Err( p_obj, "cannot open config file (%s): %s",
                 psz_filename, vlc_strerror_c(errno) );

    }
#if !( defined(_WIN32) || defined(__APPLE__) || defined(__OS2__) )
    else if( p_stream == NULL && missing )
    {
        /* This is the fallback for pre XDG Base Directory
         * Specification configs */
        char *home = config_GetUserDir(VLC_HOME_DIR);
        char *psz_old;

        if( home != NULL
         && asprintf( &psz_old, "%s/.vlc/" CONFIG_FILE,
                      home ) != -1 )
        {
            p_stream = vlc_fopen( psz_old, "rt" );
            if( p_stream )
            {
                /* Old config file found. We want to write it at the
                 * new location now. */
                msg_Info( p_obj, "Found old config file at %s. "
                          "now using %s.", psz_old, psz_filename );
                char *psz_readme;
                if( asprintf(&psz_readme,"%s/.vlc/README",
                             home ) != -1 )
                {
                    FILE *p_readme = vlc_fopen( psz_readme, "wt" );
                    if( p_readme )
                    {
                        fprintf( p_readme, "The VLC media player "
                                 "configuration folder has moved to comply\n"
                                 "with the XDG Base Directory Specification "
                                 "version 0.6. Your\nconfiguration has been "
                                 "copied to the new location:\n%s\nYou can "
                                 "delete this directory and all its contents.",
                                  psz_filename);
                        fclose( p_readme );
                    }
                    free( psz_readme );
                }
                /* Remove the old configuration file so that --reset-config
                 * can work properly. Fortunately, Linux allows removing
                 * open files - with most filesystems. */
                unlink( psz_old );
            }
            free( psz_old );
        }
        free( home );
    }
#endif
    free( psz_filename );
    return p_stream;
}

static int64_t vlc_strtoi (const char *str)
{
    char *end;
    long long l;

    errno = 0;
    l = strtoll (str, &end, 0);

    if (!errno)
    {
#if (LLONG_MAX > 0x7fffffffffffffffLL)
        if (l > 0x7fffffffffffffffLL
         || l < -0x8000000000000000LL)
            errno = ERANGE;
#endif
        if (*end)
            errno = EINVAL;
    }
    return l;
}

/*****************************************************************************
 * config_LoadConfigFile: loads the configuration file.
 *****************************************************************************
 * This function is called to load the config options stored in the config
 * file.
 *****************************************************************************/
int config_LoadConfigFile( libvlc_int_t *p_this )
{
    FILE *file;

    file = config_OpenConfigFile (p_this);
    if (file == NULL)
        return VLC_EGENERIC;

    /* Skip UTF-8 Byte Order Mark if present */
    char bom[3];
    if (fread (bom, 1, 3, file) != 3 || memcmp (bom, "\xEF\xBB\xBF", 3))
        rewind (file); /* no BOM, rewind */

    char *line = NULL;
    size_t bufsize;
    ssize_t linelen;

    /* Ensure consistent number formatting... */
    locale_t loc = newlocale (LC_NUMERIC_MASK, "C", NULL);
    locale_t baseloc = uselocale (loc);

    config_Lock();
    while ((linelen = getline (&line, &bufsize, file)) != -1)
    {
        /* The last line need not end in a newline. */
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r'))
            line[--linelen] = '\0';

        /* Ignore comments, section and empty lines */
        if (memchr ("#[", line[0], 3) != NULL)
            continue;

        /* look for option name */
        const char *psz_option_name = line;

        char *ptr = strchr (line, '=');
        if (ptr == NULL)
            continue; /* syntax error */
        *ptr = '\0';

        struct vlc_param *param = vlc_param_Find(psz_option_name);
        if (param == NULL)
            continue;

        /* Reject values of options that are unsaveable */
        if (param->unsaved)
            continue;
        /* Ignore options that are obsolete */
        if (param->obsolete)
            continue;

        module_config_t *item = &param->item;
        const char *psz_option_value = ptr + 1;
        switch (CONFIG_CLASS(item->i_type))
        {
            case CONFIG_ITEM_BOOL:
            case CONFIG_ITEM_INTEGER:
            {
                int64_t l;

                errno = 0;
                l = vlc_strtoi (psz_option_value);
                if ((l > item->max.i) || (l < item->min.i))
                    errno = ERANGE;
                if (errno)
                    msg_Warn (p_this, "Integer value (%s) for %s: %s",
                              psz_option_value, psz_option_name,
                              vlc_strerror_c(errno));
                else
                {
                    atomic_store_explicit(&param->value.i, l,
                                          memory_order_relaxed);
                    item->value.i = l;
                }
                break;
            }

            case CONFIG_ITEM_FLOAT:
            {
                if (!*psz_option_value)
                    break;                    /* ignore empty option */

                float f = (float)atof(psz_option_value);
                atomic_store_explicit(&param->value.f, f,
                                      memory_order_relaxed);
                item->value.f = f;
                break;
            }

            default:
                vlc_param_SetString(param, psz_option_value);
                break;
        }
    }
    config_Unlock();
    free (line);

    if (ferror (file))
    {
        msg_Err (p_this, "error reading configuration: %s",
                 vlc_strerror_c(errno));
        clearerr (file);
    }
    fclose (file);

    if (loc != (locale_t)0)
    {
        uselocale (baseloc);
        freelocale (loc);
    }
    return 0;
}

/*****************************************************************************
 * config_CreateDir: Create configuration directory if it doesn't exist.
 *****************************************************************************/
static int config_CreateDir( libvlc_int_t *p_this, char *psz_dirname )
{
    if (vlc_mkdir_parent(psz_dirname, 0700) == 0)
        return 0;
    msg_Warn( p_this, "could not create %s: %s", psz_dirname,
              vlc_strerror_c(errno) );
    return -1;
}

VLC_FORMAT(6, 7) static int
config_Write (FILE *file, const char *desc, const char *type,
              bool comment, const char *name, const char *fmt, ...)
{
    va_list ap;
    int ret;

    if (desc == NULL)
        desc = "?";

    if (fprintf (file, "# %s (%s)\n%s%s=", desc, vlc_gettext (type),
                 comment ? "#" : "", name) < 0)
        return -1;

    va_start (ap, fmt);
    ret = vfprintf (file, fmt, ap);
    va_end (ap);
    if (ret < 0)
        return -1;

    if (fputs ("\n\n", file) == EOF)
        return -1;
    return 0;
}


static int config_PrepareDir (libvlc_int_t *obj)
{
    char *psz_configdir = config_GetUserDir (VLC_CONFIG_DIR);
    if (psz_configdir == NULL)
        return -1;

    int ret = config_CreateDir (obj, psz_configdir);
    free (psz_configdir);
    return ret;
}

/* Preserve the existing Windows destination when replacement fails. The
 * generic vlc_rename() currently removes its destination before retrying. */
#ifdef _WIN32
static void config_ReplacementDiagnostic(libvlc_int_t *obj, const wchar_t *path,
                                          const char *label)
{
    DWORD attributes = GetFileAttributesW(path);
    DWORD attribute_error = attributes == INVALID_FILE_ATTRIBUTES ? GetLastError() : 0;
    HANDLE handle = CreateFileW(path, FILE_READ_ATTRIBUTES,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    DWORD open_error = handle == INVALID_HANDLE_VALUE ? GetLastError() : 0;
    FILE_STANDARD_INFO info = {0};
    DWORD info_error = 0;
    if (handle != INVALID_HANDLE_VALUE)
    {
        if (!GetFileInformationByHandleEx(handle, FileStandardInfo, &info, sizeof(info)))
            info_error = GetLastError();
        CloseHandle(handle);
    }
    msg_Dbg(obj, "configuration replacement %s: attributes=%lu attribute-error=%lu open-error=%lu info-error=%lu delete-pending=%u",
            label, (unsigned long)attributes, (unsigned long)attribute_error,
            (unsigned long)open_error, (unsigned long)info_error, (unsigned)info.DeletePending);
}

static void config_LockDiagnostic(libvlc_int_t *obj, HANDLE handle, const char *stage)
{
    BY_HANDLE_FILE_INFORMATION info = {0};
    LARGE_INTEGER tick = {0};
    GetFileInformationByHandle(handle, &info);
    QueryPerformanceCounter(&tick);
    msg_Dbg(obj, "configuration writer %s: pid=%lu tick=%lld lock=%lu:%lu:%lu",
            stage, (unsigned long)GetCurrentProcessId(), (long long)tick.QuadPart,
            (unsigned long)info.dwVolumeSerialNumber,
            (unsigned long)info.nFileIndexHigh, (unsigned long)info.nFileIndexLow);
}
#endif

static int config_ReplaceFile(libvlc_int_t *obj, const char *temporary,
                              const char *permanent)
{
#ifdef _WIN32
    wchar_t *source = ToWide(temporary), *destination = ToWide(permanent);
    if (source == NULL || destination == NULL)
    {
        free(source);
        free(destination);
        return -1;
    }
    BOOL success;
    DWORD error;
    unsigned retries = 0;
    /* Windows can transiently deny replacement even after our handles close.
     * Keep the interprocess lock, immutable temporary and pending edits through
     * a bounded retry; persistent denial still fails and rolls back normally. */
    for (;;)
    {
        success = MoveFileExW(source, destination,
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        error = success ? ERROR_SUCCESS : GetLastError();
        if (success || retries == 10
         || (error != ERROR_ACCESS_DENIED && error != ERROR_SHARING_VIOLATION))
            break;
        retries++;
        Sleep(10);
    }
    if (!success)
    {
        msg_Err(obj, "cannot replace configuration file (Windows error %lu)",
                (unsigned long)error);
        config_ReplacementDiagnostic(obj, source, "source");
        config_ReplacementDiagnostic(obj, destination, "destination");
        free(source);
        free(destination);
        return -1;
    }
    if (retries)
        msg_Dbg(obj, "configuration replacement recovered after %u retries", retries);
    free(source);
    free(destination);
    return 0;
#else
    VLC_UNUSED(obj);
    return vlc_rename(temporary, permanent);
#endif
}

static void config_WriteSnapshot(FILE *file)
{
    fprintf( file,
        "\xEF\xBB\xBF###\n"
        "###  "PACKAGE_NAME" "PACKAGE_VERSION"\n"
        "###\n"
        "\n"
        "###\n"
        "### lines beginning with a '#' character are comments\n"
        "###\n"
        "\n" );

    /* Look for the selected module, if NULL then save everything */
    for (vlc_plugin_t *p = vlc_plugins; p != NULL; p = p->next)
    {
        module_t *p_parser = p->module;

        if (p->conf.count == 0)
            continue;

        fprintf( file, "[%s]", module_get_object (p_parser) );
        if( p_parser->psz_longname )
            fprintf( file, " # %s\n\n", p_parser->psz_longname );
        else
            fprintf( file, "\n\n" );

        for (struct vlc_param *param = p->conf.params,
                              *end = param + p->conf.size;
             param < end;
             param++)
        {
            module_config_t *p_item = &param->item;

            if (!CONFIG_ITEM(p_item->i_type)   /* ignore hint */
             || param->obsolete                /* ignore deprecated option */
             || param->unsaved)                /* ignore volatile option */
                continue;

            if (IsConfigIntegerType (p_item->i_type))
            {
                int64_t val = atomic_load_explicit(&param->value.i,
                                                   memory_order_relaxed);
                config_Write (file, p_item->psz_text,
                             (CONFIG_CLASS(p_item->i_type) == CONFIG_ITEM_BOOL)
                                  ? N_("boolean") : N_("integer"),
                              val == p_item->orig.i,
                              p_item->psz_name, "%"PRId64, val);
            }
            else
            if (IsConfigFloatType (p_item->i_type))
            {
                float val = atomic_load_explicit(&param->value.f,
                                                 memory_order_relaxed);
                config_Write (file, p_item->psz_text, N_("float"),
                              val == p_item->orig.f,
                              p_item->psz_name, "%f", val);
            }
            else
            {
                const char *val = atomic_load_explicit(&param->value.str,
                                                       memory_order_relaxed);
                const char *orig = p_item->orig.psz;

                if (val == NULL)
                    val = "";
                if (orig == NULL)
                    orig = "";

                assert(IsConfigStringType(p_item->i_type));
                config_Write(file, p_item->psz_text, N_("string"),
                             strcmp(val, orig) == 0, p_item->psz_name, "%s",
                             val);
            }
        }
    }
}

static bool config_Savable(const struct vlc_param *param)
{
    return CONFIG_ITEM(param->item.i_type) && !param->obsolete && !param->unsaved;
}

static void config_WritePending(FILE *file, const struct vlc_param *param,
                                const char *ending)
{
    const module_config_t *item = &param->item;
    if (IsConfigIntegerType(item->i_type))
    {
        int64_t value = atomic_load_explicit(&param->value.i, memory_order_relaxed);
        fprintf(file, "%s%s=%"PRId64"%s", value == item->orig.i ? "#" : "",
                item->psz_name, value, ending);
    }
    else if (IsConfigFloatType(item->i_type))
    {
        float value = atomic_load_explicit(&param->value.f, memory_order_relaxed);
        fprintf(file, "%s%s=%f%s", value == item->orig.f ? "#" : "",
                item->psz_name, value, ending);
    }
    else
    {
        const char *value = atomic_load_explicit(&param->value.str, memory_order_relaxed);
        const char *original = item->orig.psz;
        if (value == NULL) value = "";
        if (original == NULL) original = "";
        fprintf(file, "%s%s=%s%s", strcmp(value, original) == 0 ? "#" : "",
                item->psz_name, value, ending);
    }
}

/* Preserve every untouched line verbatim, including unknown/newer options.
 * Only intentional pending keys may replace lines in another process's file. */
static int config_Merge(FILE *input, FILE *output)
{
    struct pending_line { struct vlc_param *param; bool emitted; };
    size_t count = 0;
    for (vlc_plugin_t *p = vlc_plugins; p != NULL; p = p->next)
        for (size_t i = 0; i < p->conf.size; ++i)
            if (config_Savable(p->conf.params + i) && p->conf.params[i].pending)
                count++;
    struct pending_line *pending = count ? vlc_alloc(count, sizeof(*pending)) : NULL;
    if (count && pending == NULL)
        return -1;
    size_t index = 0;
    for (vlc_plugin_t *p = vlc_plugins; p != NULL; p = p->next)
        for (size_t i = 0; i < p->conf.size; ++i)
            if (config_Savable(p->conf.params + i) && p->conf.params[i].pending)
                pending[index++] = (struct pending_line){p->conf.params + i, false};

    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    bool first = true, terminated = true;
    const char *ending = "\n";
    int ret = -1;
    while ((length = getline(&line, &capacity, input)) != -1)
    {
        size_t bom = first && length >= 3 && !memcmp(line, "\xef\xbb\xbf", 3) ? 3 : 0;
        if (first && length >= 2 && line[length - 2] == '\r' && line[length - 1] == '\n')
            ending = "\r\n";
        first = false;
        char *name = line + bom;
        if (*name == '#') name++;
        char *equals = strchr(name, '=');
        struct vlc_param *param = NULL;
        if (equals)
        {
            *equals = '\0';
            param = vlc_param_Find(name);
            *equals = '=';
        }
        size_t match = count;
        if (param)
            for (size_t i = 0; i < count; ++i)
                if (pending[i].param == param) { match = i; break; }
        if (match == count)
        {
            if (fwrite(line, 1, length, output) != (size_t)length)
                goto out;
            terminated = length == 0 || line[length - 1] == '\n';
        }
        else
        {
            if (bom && fwrite(line, 1, bom, output) != bom)
                goto out;
            if (!pending[match].emitted)
            {
                const char *line_ending = length >= 2 && line[length - 2] == '\r'
                                      && line[length - 1] == '\n' ? "\r\n" : ending;
                config_WritePending(output, param, line_ending);
                pending[match].emitted = true;
                terminated = true;
            }
            /* Remove later active/default duplicates so none can override it. */
        }
    }
    if (ferror(input) || !feof(input))
        goto out;
    for (size_t i = 0; i < count; ++i)
        if (!pending[i].emitted)
        {
            if (!terminated) fputs(ending, output);
            config_WritePending(output, pending[i].param, ending);
            terminated = true;
        }
    ret = ferror(output) ? -1 : 0;
out:
    free(line);
    free(pending);
    return ret;
}

/* The sidecar is persistent: replacing/unlinking a lock file would allow two
 * waiting processes to lock different files for the same destination. */
static intptr_t config_FileLock(const char *permanent)
{
    char *path;
    if (asprintf(&path, "%s.vlc-lock", permanent) == -1)
        return -1;
#ifdef _WIN32
    /* vlc_open(O_CREAT) uses CREATE_ALWAYS on Windows. A lock must be opened
     * without truncation, even while another process owns its byte range. */
    wchar_t *wide = ToWide(path);
    free(path);
    if (wide == NULL)
        return -1;
    HANDLE handle = CreateFileW(wide, GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                               OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    free(wide);
    if (handle == INVALID_HANDLE_VALUE)
        return -1;
    OVERLAPPED overlap = {0};
    if (!LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &overlap))
    {
        CloseHandle(handle);
        return -1;
    }
    return (intptr_t)handle;
#else
    int fd = vlc_open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    free(path);
    if (fd == -1)
        return -1;
    struct flock lock = {.l_type = F_WRLCK, .l_whence = SEEK_SET, .l_start = 0, .l_len = 1};
    int result;
    do result = fcntl(fd, F_SETLKW, &lock); while (result < 0 && errno == EINTR);
    if (result < 0)
    {
        vlc_close(fd);
        return -1;
    }
    return fd; /* Closing the descriptor releases the byte lock. */
#endif
}

static FILE *config_OpenForMerge(const char *path, bool *missing)
{
    *missing = false;
#ifdef _WIN32
    /* vlc_open's Windows failure path does not reliably translate GetLastError
     * into errno. Distinguish real absence from read denial before considering
     * a new-file snapshot; an unreadable existing file must never be replaced. */
    wchar_t *wide = ToWide(path);
    if (wide == NULL)
        return NULL;
    HANDLE handle = CreateFileW(wide, GENERIC_READ,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD error = GetLastError();
    free(wide);
    if (handle == INVALID_HANDLE_VALUE)
    {
        *missing = error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
        errno = *missing ? ENOENT
              : error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION
                || error == ERROR_LOCK_VIOLATION ? EACCES : EIO;
        return NULL;
    }
    int fd = _open_osfhandle((intptr_t)handle, O_RDONLY | O_BINARY | O_NOINHERIT);
    if (fd == -1)
    {
        CloseHandle(handle);
        return NULL;
    }
    FILE *file = fdopen(fd, "rb");
    if (file == NULL) vlc_close(fd);
    return file;
#else
    FILE *file = vlc_fopen(path, "rb");
    *missing = file == NULL && errno == ENOENT;
    return file;
#endif
}

int config_SaveConfigFileLocked(libvlc_int_t *obj)
{
    if (config_PrepareDir(obj))
        return -1;
    char *permanent = config_GetConfigFile(obj), *temporary = NULL;
    if (permanent == NULL)
        return -1;
    if (asprintf(&temporary, "%s.%"PRIu32, permanent, (uint32_t)getpid()) == -1)
    {
        free(permanent);
        return -1;
    }
    /* Lock order: configuration -> process temporary -> interprocess sidecar. */
    static vlc_mutex_t lock = VLC_STATIC_MUTEX;
    vlc_mutex_lock(&lock);
    intptr_t lockfd = config_FileLock(permanent);
    int ret = -1;
    const char *stage = "sidecar lock";
    bool reported = false;
    FILE *input = NULL, *file = NULL;
    if (lockfd == -1)
        goto out;
#ifdef _WIN32
    config_LockDiagnostic(obj, (HANDLE)lockfd, "acquired");
#endif
    struct stat st;
    if (stat(permanent, &st) == 0 && !(st.st_mode & S_IWUSR))
    {
        msg_Err(obj, "configuration file is read-only");
        reported = true;
        goto out;
    }
    bool missing;
    stage = "destination read";
    input = config_OpenForMerge(permanent, &missing);
    if (input == NULL && !missing)
    {
        msg_Err(obj, "cannot read configuration file: %s", vlc_strerror_c(errno));
        reported = true;
        goto out;
    }
    bool pending = false;
    for (vlc_plugin_t *p = vlc_plugins; p != NULL && !pending; p = p->next)
        for (size_t i = 0; i < p->conf.size && !pending; ++i)
            pending = config_Savable(p->conf.params + i) && p->conf.params[i].pending;
    if (input != NULL && !pending)
    {
        ret = 0; /* No stale snapshot write on explicit no-op saves or exit. */
        goto out;
    }
    stage = "temporary open";
    int fd = vlc_open(temporary, O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd == -1)
        goto out;
    stage = "temporary stream";
    file = fdopen(fd, "wb");
    if (file == NULL)
    {
        vlc_close(fd);
        goto out;
    }
    stage = "numeric locale";
    locale_t loc = newlocale(LC_NUMERIC_MASK, "C", NULL);
#ifdef HAVE_NEWLOCALE
    if (loc == (locale_t)0)
        goto out;
#endif
    locale_t previous = uselocale(loc);
    stage = "configuration merge";
    if (input == NULL)
    {
        config_WriteSnapshot(file);
        ret = ferror(file) ? -1 : 0;
    }
    else
        ret = config_Merge(input, file);
    uselocale(previous);
    if (loc != (locale_t)0) freelocale(loc);
    if (input != NULL)
    {
        if (fclose(input)) { ret = -1; stage = "destination close"; }
        input = NULL; /* Release the destination before replacing on Windows. */
    }
    if (ret)
        goto out;
    stage = "temporary flush";
    if (fflush(file) || ferror(file))
    {
        ret = -1;
        goto out;
    }
    stage = "temporary disk sync";
    if (fdatasync(fd))
    {
        ret = -1;
        goto out;
    }
    stage = "temporary close";
    ret = fclose(file);
    file = NULL;
    if (ret)
        goto out;
#ifdef __OS2__
    vlc_unlink(permanent);
#endif
    stage = "replacement";
    ret = config_ReplaceFile(obj, temporary, permanent);
#ifdef _WIN32
    if (ret) reported = true; /* config_ReplaceFile logged the native error. */
#endif
out:
    if (ret && !reported)
    {
#ifdef _WIN32
        DWORD error = GetLastError();
        msg_Err(obj, "configuration save failed during %s (errno %d, Windows error %lu)",
                stage, errno, (unsigned long)error);
#else
        msg_Err(obj, "configuration save failed during %s: %s", stage, vlc_strerror_c(errno));
#endif
    }
    if (input != NULL) fclose(input);
    if (file != NULL) fclose(file);
    if (ret) vlc_unlink(temporary);
    else config_AcknowledgeSave();
    if (lockfd != -1)
    {
#ifdef _WIN32
        config_LockDiagnostic(obj, (HANDLE)lockfd, "releasing");
        CloseHandle((HANDLE)lockfd);
#else
        vlc_close(lockfd);
#endif
    }
    vlc_mutex_unlock(&lock);
    free(temporary);
    free(permanent);
    return ret ? -1 : 0;
}

int (config_SaveConfigFile)(libvlc_int_t *obj)
{
    config_Lock();
    int ret = config_SaveConfigFileLocked(obj);
    config_Unlock();
    return ret;
}
