# Assembly application integration contract

The Author clarified the intended workflow: compile VDP buffers on the PC and
consume them from a hand-written eZ80 assembly application. Assembly owns file
loading, installation, updates, calls, pacing and cleanup. Compiler output
publishes assigned locations and entry points. Direct assembly emission is the
preferred additional integration option, not implemented functionality.

The GOLEM-010 ImportBinary source prototypes misleadingly suggested runtime
file loading. They are withdrawn from the current storage/index reviews.
Table/Array declarations describe types, dimensions and logical bounds; build
configuration maps source files to those symbols. The assembly application must
install configured data before a resident job reads it. Stock VDP cannot request
that MOS load a file into eZ80 RAM or push it into a VDP buffer.

The compiler can emit symbolic equates, labels and byte definitions for command
and data payloads, their lengths and relevant interface offsets. Application
assembly can include them directly. Generate those definitions, machine-readable
metadata and AGNB placement from the same layout accounting. Compilation assigns
locations; assembly performs installation. This does not imply native eZ80 or
Xtensa compilation of the Golem job, or a mandatory generated host controller.

AGNB remains useful for the assembly-side loader: one sequential container load
avoids repeated MOS directory searches, and embedded IDs remove per-asset runtime
filenames and lookup tables. Existing image/audio loaders can be reused; raw
Table/program forms still require explicit extensions. Build-time validation and
ownership accounting must precede compiler allocation/installation.

A BASIC-like interactive Golem application with editing, immediate commands and
file operations is a separate possible future frontend. The historical standalone
MOSlet loader idea is optional, not the current integration priority.

The maintained resident contract, README, handoff, living language proposals and
GOLEM-010 review documents now reflect this decision. Examples explicitly name
the required build mappings and assembly installation steps. No compiler code,
shared examples or emulator fixtures changed, and no output mode was implemented.

See [the integration contract](../design/resident-scalar.md#assembly-application-integration--direction-agreed-2026-10-12)
and [the indexed-access review](../tasks/GOLEM-010/indexed-access-review.md).
