// SPDX-License-Identifier: LGPL-2.1-or-later
// Owned native file/HTTP caching and actual TLS certificate/cipher consumers.
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_dialog.h>
#include <vlc_stream.h>
#include <vlc_tls.h>
#include "../../../lib/libvlc_internal.h"

static unsigned count, questions;
static void reject_question(void *opaque, vlc_dialog_id *id, const char *title,
    const char *text, vlc_dialog_question_type type, const char *cancel,
    const char *action1, const char *action2)
{
    (void)opaque; (void)title; (void)text; (void)type;
    (void)cancel; (void)action1; (void)action2;
    questions++;
    vlc_dialog_id_dismiss(id);
}
static void cancel_dialog(void *opaque, vlc_dialog_id *id)
{ (void)opaque; vlc_dialog_id_dismiss(id); }
#define CHECK(condition, description) do { \
    bool passed = (condition); count++; \
    printf("%s %u %s\n", passed ? "ok" : "not ok", count, description); \
    if (!passed) goto out; \
} while (0)

struct tls_result { bool credentials, tcp, session, payload; unsigned questions; };
static struct tls_result connect_tls(vlc_object_t *root, vlc_tls_client_t *existing,
                                    unsigned port, const char *host)
{
    struct tls_result result = {0};
    unsigned before = questions;
    vlc_tls_client_t *credentials = existing ? existing : vlc_tls_ClientCreate(root);
    if (!credentials) return result;
    result.credentials = true;
    vlc_tls_t *tcp = vlc_tls_SocketOpenTCP(root, "127.0.0.1", port);
    if (tcp) {
        result.tcp = true;
        vlc_tls_t *tls = vlc_tls_ClientSessionCreate(credentials, tcp, host,
            "owned-network-settings-fixture", NULL, NULL);
        if (tls) {
            result.session = true;
            char bytes[5];
            result.payload = vlc_tls_Write(tls, "PING\n", 5) == 5
                && vlc_tls_Read(tls, bytes, sizeof(bytes), true) == sizeof(bytes)
                && !memcmp(bytes, "PONG\n", sizeof(bytes));
            vlc_tls_Close(tls);
        } else vlc_tls_Close(tcp);
    }
    if (!existing) vlc_tls_ClientDelete(credentials);
    result.questions = questions - before;
    return result;
}
static struct tls_result attempt(vlc_object_t *root, unsigned port, bool system,
    const char *directory, const char *policy, const char *host)
{
    config_PutInt("gnutls-system-trust", system);
    config_PutPsz("gnutls-dir-trust", directory);
    config_PutPsz("gnutls-priorities", policy);
    return connect_tls(root, NULL, port, host);
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    // TLS port, owned CA directory, owned empty directory, file URI, HTTP URL.
    if (argc != 6) return 2;
    char *end; unsigned long port = strtoul(argv[1], &end, 10);
    if (*end || port < 1024 || port > UINT16_MAX || strncmp(argv[5], "http://127.0.0.1:", 17)) return 2;
    const char *options[] = {"--ignore-config", "--no-media-library", "--no-metadata-network-access",
        "--no-auto-preparse", "--extraintf=", "--control=", "--ipv4-timeout=2500", "--verbose=2"};
    libvlc_instance_t *instance = libvlc_new(ARRAY_SIZE(options), options);
    if (!instance) return 3;
    vlc_object_t *root = VLC_OBJECT(instance->p_libvlc_int);
    const vlc_dialog_cbs callbacks = {.pf_display_question=reject_question, .pf_cancel=cancel_dialog};
    vlc_dialog_provider_set_callbacks(root, &callbacks, NULL);
    config_PutPsz("http-proxy", "direct://");
    int result = 1;
    vlc_tls_client_t *cached = NULL;
    for (unsigned i = 0; i < 4; ++i) {
        const bool local = i < 2;
        const unsigned milliseconds = (i + 1) * 111;
        config_PutInt(local ? "file-caching" : "network-caching", milliseconds);
        stream_t *stream = vlc_stream_NewURL(root, argv[local ? 4 : 5]);
        CHECK(stream != NULL, "actual file/HTTP source opens after changing its buffering default");
        vlc_tick_t delay = VLC_TICK_INVALID;
        bool controlled = vlc_stream_Control(stream, STREAM_GET_PTS_DELAY, &delay) == VLC_SUCCESS;
        char bytes[64] = {0};
        ssize_t size = vlc_stream_Read(stream, bytes, sizeof(bytes));
        vlc_stream_Delete(stream);
        CHECK(controlled && delay == VLC_TICK_FROM_MS(milliseconds), "actual source reports the saved millisecond buffering value");
        static const char expected[] = "owned-network-cache-fixture\n";
        CHECK(size == sizeof(expected) - 1 && !memcmp(bytes, expected, sizeof(expected) - 1), "actual source preserves every owned payload byte");
    }
    struct tls_result tls;
#define EXPECT_TLS(system, directory, policy, host, expected, prompt) do { \
    tls = attempt(root, port, system, directory, policy, host); \
    CHECK(tls.credentials && tls.tcp, "actual TLS credentials and loopback TCP setup succeeded"); \
    CHECK(tls.session == expected, "actual TLS session respects certificate and cipher policy"); \
    if (expected) CHECK(tls.payload, "actual TLS session exchanges exact owned application bytes"); \
    if (prompt) CHECK(tls.questions > 0, "untrusted certificate reached the owned rejecting dialog callback"); \
} while (0)
    EXPECT_TLS(false, "", "NORMAL", "owned-network.invalid", false, true);
    EXPECT_TLS(true, "", "NORMAL", "owned-network.invalid", false, true);
    EXPECT_TLS(false, argv[2], "NORMAL", "owned-network.invalid", true, false);
    EXPECT_TLS(true, argv[2], "NORMAL", "owned-network.invalid", true, false);
    EXPECT_TLS(false, argv[2], "NORMAL", "wrong.owned.invalid", false, true);
    EXPECT_TLS(false, argv[3], "NORMAL", "owned-network.invalid", false, true);
    for (const char **policy = (const char *[]){"PERFORMANCE", "NORMAL", "SECURE128", "SECURE256", NULL}; *policy; ++policy)
        EXPECT_TLS(false, argv[2], *policy, "owned-network.invalid", true, false);
    EXPECT_TLS(false, argv[2], "OWNED-INVALID-PRIORITY", "owned-network.invalid", false, false);
    config_PutInt("gnutls-system-trust", false);
    config_PutPsz("gnutls-dir-trust", argv[2]); config_PutPsz("gnutls-priorities", "NORMAL");
    cached = vlc_tls_ClientCreate(root);
    config_PutPsz("gnutls-dir-trust", "");
    tls = connect_tls(root, cached, port, "owned-network.invalid");
    CHECK(cached && tls.credentials && tls.tcp, "cached TLS credentials remain usable after a preference change");
    CHECK(tls.session, "existing credentials retain their original owned trust snapshot");
    CHECK(tls.payload, "cached credentials still exchange exact owned application bytes");
    vlc_tls_ClientDelete(cached); cached = NULL;
    EXPECT_TLS(false, "", "NORMAL", "owned-network.invalid", false, true);
    result = 0;
out:
    if (cached) vlc_tls_ClientDelete(cached);
    vlc_dialog_provider_set_callbacks(root, NULL, NULL);
    libvlc_release(instance);
    return result;
}
