# Typed drawing syntax proposals

The Author authorized GOLEM-011 / SYN-001 after the drawing inventory review.
EX-030 through EX-036 now propose independently initialized examples for all
five Rally drawing families: general PLOT, filled quadrilaterals, bitmap drawing,
viewport selection and scrolling. Named point/line and four scroll operations
avoid numeric operation codes for common drawing. General plot remains available.
Supporting contracts and coverage live in docs/tasks/GOLEM-011/drawing-syntax.md.

The bitmap proposal uses full 16-bit buffer IDs with stock resource selection,
fitting build-assigned AGNB resource IDs. Assembly still installs/registers the
bitmap and owns graphics setup, input updates, invocation and pacing. Optional
affine drawing explicitly initializes and rebuilds its matrix each invocation.
The examples state pixel/origin setup and viewport/scroll preconditions rather
than implying hidden mode changes or a new error protocol.

Quad source review clarified a detail in the inventory: Rally's PLOT 4,4,85,85
sequence shares its middle vertex pair between two triangles. Friendly
fill_quad accepts convex perimeter order A,B,C,D; lowering would use A,B,D,C
so the shared pair is a diagonal. This needs only a fixed operand permutation,
not a sophisticated optimizer or runtime geometry repair.

The earlier EX-001 through EX-029 source blocks remain unchanged. Reviewed the
new examples for declarations, initializers, argument types, resource setup and
coverage. No compiler implementation, emulator run or upstream modification was
made; proposed syntax has not been compiled or visually qualified. DRW-001's
four existing passing host regressions remain inventory evidence, not validation
of these new spellings. SYN-001 is checked complete; stop for Author review.
WIR-001 installation/invocation traffic analysis is next. This tranche is
uncommitted.

The Author subsequently approved SYN-001's syntax direction and asked when
visible drawing would be available. Acceptance is recorded in the task register
and syntax document. WIR-001 remains the next review subtask; compiler support
and an Oryx pixel demonstration have not yet been implemented or authorized.
