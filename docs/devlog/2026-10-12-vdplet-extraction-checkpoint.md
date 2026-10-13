# VDPlet extraction and authorized repository checkpoint

The Author authorized committing and pushing all pending work, followed by the
first GOLEM-023 research tranche. The native runtime now has its own hardware
repository at `/Agon/mystuff/agon-vdplet`. Earlier records accurately describe
their original review boundaries; this authorization supersedes their pending
commit approval, without claiming human emulator acceptance or physical proof.

The extracted repository owns the public ABI, restricted ELF loader, firmware
services, independent firmware/module builders and numeric/packed-renderer
examples. Its documentation is hardware-focused, with MIT licensing and
Brandon R. Gates identified as owner. Historical task evidence stays here;
compatibility links and builder adapters refer to the new maintained sources.

The extraction's full firmware build passed in 111.64 seconds and produced a
1,086,192-byte application binary. All three native example packages remained
byte-identical to the existing references. Host admission/relocation/BSS,
allocation-failure and cleanup checks passed under ASan/UBSan. At this commit
checkpoint, `make -C src check` also passes maintained contracts and all 132
rejection cases. No device has been flashed.

GOLEM-021 and GOLEM-022 human reviews remain open. GOLEM-023 is authorized for
research only: compare integration options, recommend a small native subset and
present initialized examples plus the proposed assembly interface. Compiler and
runtime implementation remain outside this tranche.
