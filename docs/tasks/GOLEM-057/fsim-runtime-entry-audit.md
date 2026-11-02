# FSIM call overhead — source boundary audit

The shared runtime does not reload or relocate a VDPlet on each frame call.
The live consumer does repeat validation and setup within its own module.
These are source facts that help interpret the planned counters; they do not
establish which work causes the reported slowdown or select an optimization.

1. The rows-r1 resource receipt identifies firmware8d980025 and exported
   vdplet_runtime.h SHA bf760b5bc79977f49b6f5669c269d875692f9d9d5d41b3ca680e4fb906afeefa.
   The maintained file still matches that header. install() alone invokes
   vdpl::load() for package admission/allocation/segment copying/relocation and
   INIT. invoke() checks handle and the one-block, at-most32byte argument buffer,
   then call() dispatches directly to the existing loaded entry.
2. call() sets invocation ownership, issues memw/isync, selects the ABI service
   table and calls the loaded function. On return it closes remaining frame/
   buffer leases and clears invocation ownership. It does not re-run package
   admission, copy the whole code/data image or initialize all module data.
   Modules may keep permitted numeric state; pointers acquired through scoped
   leases must not survive their lifetime.
3. The FSIM module.c SHA f43c961da92ac8eda072771c160b0a601727d9460369d133cf06d3b3ac636567
   and presentation.h SHA be3a675f904d210b82ffc2154d077d6b5bc8682e58039a763fb5fafb46d765e6
   match the qualified consumer's source-pins.json. Each PREPARE and RENDER
   entry acquires its four arenas, calls pg_validate(), zeroes PR_NATIVE_BYTES,
   and calls pf_setup(). Cleanup zeroes that native scratch again and releases
   leases. PR_NATIVE_BYTES is8192, so the two successful calls request four
   clears,32768bytes/frame, before considering other renderer work. That is
   source-level requested traffic, not measured physical bandwidth or time.
4. Row BEGIN pauses/drains before validating/capturing mappings and checking
   every requested row. Per-row access checks lease/range/pointer and returns
   the captured mapping; it does not repeat the whole BEGIN drain on each row.
   The consumer also validates each returned descriptor before rasterization.
   Distinguish this shared row service from consumer validation, setup, depth
   clearing, rasterization and queued graphics/wire/foreground waits.
5. Continue the accepted counter-only tranche unchanged. It can identify broad
   costly stages but cannot assign time to these individual source operations.
   Do not remove admission/lifetime checks, add persistence or change module/
   firmware/ABI based on this static audit. Native telemetry or optimization
   needs the separately reviewed measurement/consumer scope already described.
6. Keep VSYNC mechanisms distinct. The pinned processNext() polls the context
   and runs buffered CALLBACK_VSYNC programs; processAllAvailable() explicitly
   avoids event processing. A synchronous RUN delays that foreground work.
   This is not proof that MOS's physical VBlank interrupt stops: the VGA
   controller also generates sync through its DMA/GPIO path, separately from
   buffered callback dispatch. The emulator's frontend-to-eZ80 interrupt gate
   adds another clock boundary. Preserve the already documented missed/coalesced
   interrupt and wall-calibration limits without equating these mechanisms or
   changing timing code on an unproved blocking hypothesis.
