/* Layout companion for the opt-in Python LibVLC frame-feedback regression. */
#include <stddef.h>
#include <stdio.h>
#include <vlc/vlc.h>

int main(void)
{
    printf("{\"version\":0,\"pointerSize\":%llu,\"callbackStructSize\":%llu,"
           "\"versionOffset\":%llu,\"nextFrameOffset\":%llu,"
           "\"previousFrameOffset\":%llu}\n",
           (unsigned long long) sizeof(void *),
           (unsigned long long) sizeof(struct libvlc_media_player_cbs),
           (unsigned long long) offsetof(struct libvlc_media_player_cbs, version),
           (unsigned long long) offsetof(struct libvlc_media_player_cbs,
                                        on_next_frame_status),
           (unsigned long long) offsetof(struct libvlc_media_player_cbs,
                                        on_prev_frame_status));
    return 0;
}
