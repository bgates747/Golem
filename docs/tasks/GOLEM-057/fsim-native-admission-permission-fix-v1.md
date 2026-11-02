# FSIM native admission permission correction

The selected pair has not started. Native PID1 is systemd and the caller sees
the real host namespace, but an unprivileged readlink of /proc/1/ns/pid is
forbidden. Root independently confirms that a narrowly privileged read returns
the same namespace. Correct only this host admission assumption.

- ADM-001 [ ] Preserve the original no-run completion/log, unconsumed permit,
  absent output/SD destinations and unchanged67ee source freeze. No emulator,
  audio or diagnostic observation resulted; no target attempt was consumed.
- ADM-002 [ ] In the separate driver, retain PID1systemd and namespace equality.
  First try unprivileged os.readlink('/proc/1/ns/pid'). Only on PermissionError,
  use ['sudo','-n','readlink','/proc/1/ns/pid'] with a finite5s timeout, no shell.
  Compare its stripped output with the caller's own namespace and fail closed
  on denial/timeout/malformed/mismatched output. No emulator, build, staging or
  broad process inventory runs under sudo. Keep all other admission/run guards.
- ADM-003 [ ] Meaningful device-free cases cover direct success, PermissionError
  fallback/equality, mismatch and sudo refusal/timeout before staging. Reuse
  existing orchestration tests. Exercise the namespace-reading helper alone
  through the native host tool to confirm the actual permission condition;
  that read-only check must not consume a permit or stage/start anything.
- ADM-004 [ ] Freeze final driver/test/invocation/readiness and return the
  preserved no-run outcome plus exact source delta. Preserve c361 diagnostics,
  runtime/helper/physics/assets/old results. No rebuild or target readmission
  is selected in this correction. Root reviews the small final delta before
  separately selecting fresh admission for the still-unused ONE pair.

Native independent observation: PID1systemd, self pid:[4026531836]; unprivileged
PID1readlink PermissionError13; sudo -n readlink returns pid:[4026531836].
/proc/1/status also reads Name:systemd/Pid:1/NSpid:1. No host setting changed.
