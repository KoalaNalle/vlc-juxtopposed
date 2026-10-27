# Sparse subtitles and recording start - 27 October 2026

A subtitle packet queued near20seconds advanced the common recording start past
the first video keyframe. The next keyframe also fell before that start, leaving
short recordings without usable video and longer recordings misaligned.

The ordinary recording module now aligns audio/video using those streams when
present. Subtitle-only output retains its previous calculation. The direct
production regression fails three cases before and passes all eleven after.

A28.3-second ordinary desktop capture now decodes about19.5seconds of aligned
audio/video. Three native recordings also decode aligned AV; their starts differ
by about21milliseconds. The long-GOP source still needs a usable keyframe, so
this is not frame-exact recording from the clicked time. Original missing-video
and misalignment controls are retained separately.

See the [checkpoint](checkpoint.json) and sanitized
[timestamp observations](timestamp-observations.json) for exact identities and
limitations. This is a scoped recording repair, not complete beta acceptance.
Continue from [STATUS](../../../juxtopposed/STATUS.md).
