# Focused preimage R2: old production setter, stronger cache prerequisites

This is diagnostic evidence, not a passing worker qualification. Only the two
busyStateAndCapturedMode data rows run, plus setup/cleanup. The actual initial
model must first load3, and a fresh opposite model must load1 before the switched
original model is tested. The production setter is still unchanged. Preserve
all failure output; do not remove or relax either query prerequisite.

The same reviewed exclusive-lock, compiler-dependency,384MiB peak and23:35
stop-bound guards apply. No GUI/Core rebuild, new dependencies or production
changes are made by these producers. A separate fresh full run follows any
reviewed production repair.
