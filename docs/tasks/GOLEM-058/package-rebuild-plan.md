# Standalone distribution refresh

The existing ZIP already carries current compiler sources and workers, integer
and float printing, bitwise examples, a how-to and a language reference. Refresh
only demonstrated gaps and prove rebuildability from the distributed bytes.
The confirmed how-to correction explains the supplied runner's final newline.

- AUD-001 [x] Map every implemented profile and option to maintained source,
  packaged source/reference and initialized examples; distinguish native-C-only
  operations from stock buffered output and proposed syntax.
- BLD-001 [x] Extract the exact accepted ZIP into a new isolated build directory,
  verify its manifest and safe paths, then run its own shipped build_all.py with
  AgonDev. Compare all four workers, MOS dispatcher and runner to shipped bytes.
  Preserve failures and identities instead of substituting repository sources.
- BLD-002 [x] Run maintained compiler checks and existing adapted-host parity
  and shipped-example checks, extending coverage only for actual gaps.
- PKG-001 [x] Build a candidate ZIP without overwriting the accepted archive or
  its evidence, with corrected how-to and complete reference/source/examples.
- TGT-001 [ ] Qualify shipped native compiles and stock output under real MOS in
  a dedicated, ordinarily scheduled Oryx instance. Stage the walkthrough stopped
  for human review, then publish only after explicit approval.

Source and accepted worker hashes currently match. A successful exact rebuild
may therefore retain the six binaries; it does not itself add language features.
