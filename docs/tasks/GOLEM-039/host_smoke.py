"""Bounded supplied-player/lifecycle check; Rally owns independent world validation."""
from pathlib import Path
import importlib.util
import json
import struct
import sys

HERE = Path(__file__).resolve().parent
binding = HERE.parent / 'GOLEM-037' / 'binding'
loader = importlib.util.spec_from_file_location('accepted_binding_host', binding / 'host_checks.py')
accepted = importlib.util.module_from_spec(loader)
loader.loader.exec_module(accepted)


def check(directory):
    spec = accepted.spec(directory)
    runtime = accepted.Mock(spec, directory / 'host.so')
    assert runtime.invoke(operation=1) == 0
    assert runtime.invoke('InitRace') == 0
    root, arrays = runtime.snapshot()
    assert spec.get(root, 'playerStation') == 3270000
    assert spec.get(root, 'renderPhase') == 0
    initial_unwrapped = spec.get(root, 'playerUnwrapped')
    for tick in range(1, 9):
        station = (3270000 + tick * 1700) % 3276800
        phase = tick * 101
        lateral = tick * 10
        steering = tick - 4
        for name, value in [('playerStation', station), ('playerLateral', lateral),
                            ('renderPhase', phase), ('renderSteering', steering)]:
            runtime.put(name, value)
        assert runtime.invoke('Tick120') == 0
        root, arrays = runtime.snapshot()
        for name, value in [('playerStation', station), ('playerLateral', lateral),
                            ('renderPhase', phase), ('renderSteering', steering),
                            ('playerUnwrapped', initial_unwrapped), ('worldTick', tick)]:
            assert spec.get(root, name) == value, (name, tick)
    assert runtime.invoke('Render30') == 0
    completion = runtime.data(runtime.completion)
    assert struct.unpack_from('<HHii', completion) == (48, 1, 0, 0)
    assert struct.unpack_from('<III', completion, 12) == (7, 1, 8)
    assert struct.unpack_from('<H', completion, 44)[0] == 1
    lengths = struct.unpack_from('<III', completion, 24)
    assert all(0 < n <= out['bytes'] for n, out in
               zip(lengths, runtime.cfg['bindings']['outputs']))
    assert runtime.generation() == 10
    assert runtime.invoke('StopRace') == 0
    before = runtime.generation()
    assert runtime.invoke('Render30') == 12
    assert runtime.generation() == before
    assert runtime.invoke('ResetRace') == 0
    root, arrays = runtime.snapshot()
    assert spec.get(root, 'playerStation') == 3270000
    assert spec.get(root, 'worldTick') == 0
    assert runtime.invoke(operation=3) == 0
    assert runtime.peak <= 3 and not runtime.live
    runtime.canaries()
    return dict(scope='host generated-source/helper contract smoke, not independent game or target qualification',
                supplied_player_ticks=8, calls=runtime.calls, peak_leases=runtime.peak,
                completion_lengths=list(lengths), checks=['supplied snapshot preserved across each Tick120',
                'source worldTick increments', 'render completion matches session/request/tick',
                'stop rejects subsequent renderer prepare', 'reset restores initial state',
                'INIT/FINI return', 'buffer canaries and lease cleanup'])


if __name__ == '__main__':
    print(json.dumps(check(Path(sys.argv[1]).resolve()), indent=2))
