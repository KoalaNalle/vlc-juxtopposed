// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright (C) 2026 VLC authors and VideoLAN
.pragma library

function mediaCriteria(criteria) {
    return criteria === "activity" ? "" : criteria
}

function mediaOrder(criteria, order) {
    return criteria ? order : Qt.DescendingOrder
}

function activityEntries(entries, pattern, criteria, order) {
    const search = pattern.toLocaleLowerCase()
    let result = entries.filter(entry => !search || entry.title.toLocaleLowerCase().includes(search))
    if (criteria === "title")
        result.sort((a, b) => a.title.localeCompare(b.title) * (order === Qt.AscendingOrder ? 1 : -1))
    else if (criteria && order === Qt.AscendingOrder)
        result.reverse()
    return result
}

// Recent visits use their actual timestamp, independently of frequency ranking.
function recentActivityEntries(entries, pattern, criteria, order) {
    const result = activityEntries(entries, pattern, "", order)
    if (criteria === "title")
        result.sort((a, b) => a.title.localeCompare(b.title) * (order === Qt.AscendingOrder ? 1 : -1))
    else {
        result.sort((a, b) => b.lastActivity - a.lastActivity || a.title.localeCompare(b.title))
        if (criteria && order === Qt.AscendingOrder)
            result.reverse()
    }
    return result
}
