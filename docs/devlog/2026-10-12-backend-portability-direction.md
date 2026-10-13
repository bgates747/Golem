# Shared source and selectable execution backends

The Author requires the same Golem source to compile for stock VDP buffered
commands or native VDPlets. Developers may choose based on requirements; end
users may select a path, including at runtime when the installed firmware and
application support it. Common semantics, storage and named entry interfaces
are therefore foundational requirements rather than optional later adaptation.

The [requirements and roadmap](../design/backend-portability.md) record stock
compatibility, explicit target diagnostics, state-preserving quiescent changes,
shared data ownership, safe availability detection and no replay of partial
effects. No firmware flashing or on-Agon compilation is implied.

The Author authorized recording the direction, committing/pushing all pending
work and proceeding at high thinking. GOLEM-024 is the first bounded implementation:
a task-local shared typed frontend/state planner with target-independent artifacts
and honest separate target requirements. It supplies a reviewed foundation for
later emitters; it does not claim either new backend is operational.
