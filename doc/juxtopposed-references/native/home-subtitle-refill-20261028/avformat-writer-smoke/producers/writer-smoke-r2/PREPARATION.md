# Real writer smoke R2

R1 remains frozen with its failed source-exact MP4 assertion and all guards.
Both R1 DLLs produced identical59-frame decoded video from60-source-frame input,
while preserving60 compressed video packets. The candidate retains115712 decoded
AAC samples versus113664 control and116736 source, all source PCM prefixes.

R2 records that precise existing MP4 limitation as a matched compatibility smoke.
It requires the already observed59/60 split,60 video packets, identical decoded
video across both DLLs, a source-exact audio prefix missing exactly1024 samples,
and no audio regression against control. It does not claim source-exact MP4.
Other formats still require source-exact candidate video; short lossless Matroska
also requires exact audio. All frozen-source/runtime/fixture guards remain.

Fixtures are the same immutable287823-byte generated R1 set. No regeneration,
production edits,builds or dependency installations occur in this lane.
