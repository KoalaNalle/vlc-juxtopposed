# Setup clarification for the repaired-Core role receipt

The frozen [role receipt](repaired-core-subtitle-roles-20261029/README.md) describes
R3 as using full two-track initialization instead of an FPS-only fresh setup.
Both R2 and R3 actually perform the same two-track initialization. The
`VLC_TEST_FIRST_FPS_FRESH` variable guards eligibility of particular first-FPS
test methods; it does not bypass `initTestCase`.

R2's estimator rows failed at their first explicit check for the missing
`VLC_TRACKS_SOURCE_DIR`. R3 supplies the frozen production pages through that
variable. Removing the unrelated first-FPS eligibility flag does not repair
initialization. The recorded 24 speed rows and 16 cancellation rows still pass
with the exact source/binary/input identities in the receipt. This clarification
corrects the explanation without changing the frozen results or their scope.
