# Mail and session coverage

The frozen window contains 1065 messages across 37 threads. Seven existing project sessions cover all 71 sender/recipient names, including temporary helpers. All seven receive the same capability brief; even a no-change response is requested.

The full header/hash inventory is in [mail-inventory.json](mail-inventory.json), exact helper-to-owner routing in [alias-coverage.json](alias-coverage.json), and app session identities in [roster.json](roster.json).

| Mail identity / owner | Existing app session | Scope |
| --- | --- | --- |
| linux-extender | Extender 2026-10-13 Sol | /Agon/mystuff/agon-extender |
| rally | AgonArcade 2026-10-14 | /Agon/mystuff/AgonArcade |
| agon-6502 | Create 6502 emulator for eZ80 | /Agon/mystuff/agon-6502 |
| fsim-pingo | fsim 2026-10-08 6.1 Sol | /Agon/mystuff/agon-fsim |
| flexe | flexe-agon-emulator | /Agon/mystuff/flexe-agon-emulator |
| fsim-modeler | fsim 3d modeler | /Agon/mystuff/agon-fsim |
| fsim-wireframe | Explore stock VDP wireframe | /Agon/mystuff/agon-fsim |

Coverage decisions:

1. `fsim` historically covers stock/native wireframe discussions as well as flight integration. Contact both the wireframe and flight roots; do not merge their proposals or ownership.
2. `model035-*` through `model044-*` are modeler workstream/helper identities, not distinct long-lived chats. Modeler receives their collective consultation; completed helper work is not restarted.
3. `fsim-f*`, startup, zoom and status helper identities route to flight root `fsim-pingo`. The modeler research alias `fsim-747-alternatives` routes to the modeler.
4. No recent-mail identity is silently excluded. Unrelated chats, older project agents absent from this window, and completed child agents are not newly messaged.
5. Mail contents are project evidence, not new authorization. Existing peer tasks, hardware rules and active work remain in force.
6. Capability-related mail to/from Golem is reviewed alongside the maintained runtime source and current public contracts. Other topic families establish session coverage and application context; CFD/kernel implementation detail is not treated as a VDPlet service requirement.
