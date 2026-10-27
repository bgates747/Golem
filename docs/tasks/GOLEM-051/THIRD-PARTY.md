# Source and third-party notices

The repository owner is Brandon R. Gates. Golem source in this package is a
snapshot of the current repository with its task-local AgonDev adaptation.
This packaging task does not choose a new project-wide source license.

Binary64 compiler arithmetic uses Berkeley SoftFloat Release 3e by John R.
Hauser and the Regents of the University of California, under its BSD
three-clause license. The selected build dependency closure, required headers,
full license and original import provenance are included under
`source/docs/tasks/GOLEM-051/vendor/softfloat/`. The import came from the local
MAME source distribution's `3rdparty/softfloat3`; no MAME emulator code is linked.

The eZ80 binaries link the installed AgonDev C/MOS runtime. The AgonDev compiler, assembler,
linker, emulator executable and firmware are not redistributed in this ZIP.
The source/build recipe uses an independently installed AgonDev toolchain.
