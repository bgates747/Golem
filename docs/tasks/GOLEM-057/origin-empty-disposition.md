# Empty origin control: retain failure, reuse the valid observation

The first control captured no unmatched reads and completed the intended mode,
pixel and fresh-frame observation. A later exact executable-heap equality
assertion failed by2,064 bytes. That is a real failed acceptance condition in
the frozen run. It does not invalidate the independently finalized zero-read
metadata or require another empty run to observe the loaded case.

1. Preserve generation23, strictpassed=false/usable_observation=false and the
   unexecuted LOADED suffix exactly. Metadata is finalized, reconciled to zero
   original counters and has no collector error. Firmware commands reached
   restore/empty-slot; the exact-heap condition failed afterward.
2. Stock mode transitions destructively recreate controller, row/descriptor,
   Canvas and palette resources. The source uses overlapping internal8/DMA/
   executable/32-bit pools and different allocation orders. Same mode number
   does not itself promise identical allocation placement or free totals. This
   is not proof that this residual is benign or attributable to one resource.
   In particular the palette plane is INTERNAL|8BIT, not proof of an IRAM leak.
3. The diagnostic question is the first counted read's actual origin. Separate
   completed cleanup commands, empty module slot, exact heap recovery, observer
   validity and clean execution. Keep exact recovery false where it is false;
   do not hide residuals behind tolerance or relabel the prior run passing.
4. Release a new frozen LOADED-only observation preparation with identical
   original firmware/module/prefix and collector. Reference the retained EMPTY
   zero metadata as comparison context without changing its strict result.
   No repeat of the already completed EMPTY observation is needed.
5. The new contract records heap deltas separately and keeps strictpass false
   if exact recovery is a desired strict condition. A bounded finalized usable
   diagnostic can still report a PC despite missing cleanup at its deadline.
   Any unexpected prefix/backend/transport/schema/finalization failure stops.
   New source/profile freeze and fresh central grant precede execution.

This changes no firmware, allocator, emulator memory mapping or read behavior.
It does not establish physical memory behavior, absence of leakage, a loaded
origin or acceptance of the original full-colour desktop.
