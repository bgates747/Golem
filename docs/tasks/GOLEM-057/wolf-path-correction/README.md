# Wolf staging path candidate

This isolated source candidate corrects the generation78 prelaunch directory
mismatch. Four device-free tests pass using the actual unchanged frozen runner,
including its real prelaunch code through a mocked process boundary. No emulator,
audio device, SSH, build or consumer source edit occurred.

The five changed lines use a fresh r02 profile and the original ring-output
basename in both staging tools. The profile itself provides isolation. No
runner, oracle, backend, media or timing behavior changes; r01 and its failed
attempt remain untouched. `path-only.patch` is supplied for the Wolf owner to
review and adopt within its source fence. `inputs.json` pins original/candidate
stagers, so source drift must be resolved before applying it.

Run from Golem:

```bash
.venv/bin/python docs/tasks/GOLEM-057/wolf-path-correction/test_prelaunch.py
```

The tests reproduce the old missing-path exception before any launch, reach the
mocked launch boundary with the corrected location, reject nonfresh output
without deleting evidence, and verify stager/wrapper/runner path and frozen pin
agreement. Runtime hash checking and process creation are mocked: these tests
prove path integration only, not a real target profile or audio measurement.
A separate owner readiness/target selection is still required before deployment.
