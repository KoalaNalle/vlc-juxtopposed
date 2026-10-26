# Local subtitle attachments - 26 October 2026

The ordinary Add Subtitles dialog now has file, folder and saved-playlist scope,
local file/folder pickers, manual pairs, pointer drag, unambiguous Match names,
checked persistence and ordered default selection. It uses the original four
[Figma references](../../figma/20261026/subtitle-attachments/README.md) and artwork.
The online search/download design remains unfinished.

Twenty-two native functions pass, including five attachment functions covering
scope/restart, ambiguity, standard picker format discovery, blocked-write retry,
corrupt storage and actual paused native attachment/default/fresh-input restore.
The component checks separately exercise production QML with supplied state,
actual pointer drag, accessible choice names, short-window errors and 130 host
checks. They do not establish ordinary desktop or native behavior.

Ordinary runs save two local SRTs and render the chosen external caption without
advancing the paused clock. Restart restores choices. Folder matching leaves
ambiguous language variants unselected; manual pairs and the next episode work.
Actual native playlist creation and Add and play establish saved playlist origin,
both loaded media rows, partial-edit preservation and file > playlist > folder
precedence. Cold reopening retains the playlist-specific choice. All four runs
exit normally with unchanged guards, 602 modules and no reported diagnostics.
The dialog moves by its header; connector visibility and choice names were fixed.

[checkpoint.json](checkpoint.json) binds original compressed artifacts and exact
source/runtime identities. It records limits, including pending asynchronous
decode-error feedback, preset interaction, the initial cold presentation recheck,
and loaded-member scope. No full surface, visual, final-package or beta gate is
closed. Continue using [STATUS](../../../juxtopposed/STATUS.md).
