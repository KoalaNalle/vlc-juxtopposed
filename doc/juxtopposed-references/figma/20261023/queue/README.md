# Original Queue reference

Figma MCP returned full context and a valid400×946 screenshot for
[Queue463:831](https://www.figma.com/design/zdNUYRVOH9lEtIxMYvh8DV/?node-id=463-831)
on22 October. [Context](design-context.tsx), [reference facts](reference.json)
and [original screenshot](queue-463-831.png) are preserved here. This is reference
material; the TSX is not application code or a runtime dependency. Temporary
asset URLs in the context are not used at runtime. Existing original SVGs and
Inter font remain unchanged.

The design uses Inter12 for tabs/summary, Inter14 for rows,50px tabs,32px rows
with12px separation,20px body side insets and the original24px move icon in a
32px slot. Panel tint is95% #0E0E0E with background blur (Figma radius50,
generated CSS25px). Actual background blur remains unimplemented. The current
shortcut repair changes event handling only; it does not establish whole-surface
pixel, focus, screen-reader or physical Windows DPI acceptance.
