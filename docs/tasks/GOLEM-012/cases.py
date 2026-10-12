"""Arithmetic expectations independent of command encoding/execution."""
import random


def cases():
    boundaries = [
        (0, 0, 0, 0, 0, 9), (255, 1, 0, 0, 256, 9),
        (65535, 0, 0, 65535, 0, 9), (65534, 1, 0, 1, 1, 9),
        (65535, 1, 0, 0, 0, 9), (65535, 1, 65535, 0, 0, 9),
        (65534, 1, 1, 0, 0, 9), (32768, 32767, 0, 0, 0, 9),
        (32768, 32768, 0, 0, 0, 9), (1, 65534, 0, 0, 0, 9),
        (0, 0, 0, 65535, 1, 9), (0, 65535, 0, 1, 65535, 9),
        (1, 2, 3, 4, 5, 65535), (1, 2, 3, 4, 5, 65534),
        (0, 0, 0, 65534, 1, 9), (42, 7, 8, 9, 10, 9),
    ]
    rng = random.Random(12012)
    boundaries.extend(tuple(rng.randrange(65536) for _ in range(5)) + (9,) for _ in range(24))
    return [dict(zip(('a', 'b', 'c', 'selfValue', 'extra', 'tail'), values)) for values in boundaries]


def initial(values):
    return dict(a=values['a'], b=values['b'], c=values['c'], extra=values['extra'],
                before=52428, total=48879, selfValue=values['selfValue'], after=56797,
                tail=values['tail'], completed=0)


def expected(values):
    state = initial(values)
    state['before'] = state['a']
    candidate = state['a'] + state['b'] + state['c']
    if candidate > 65535:
        return state, 4660, 1
    state['total'] = candidate
    result = -1  # Independent floor(-0.75); source remains the qualified scalar slice.
    candidate = state['selfValue'] + state['extra']
    if candidate > 65535:
        return state, result, 1
    state['selfValue'] = candidate
    state['after'] = state['total']
    candidate = state['tail'] + 1
    if candidate > 65535:
        return state, result, 1
    state['tail'] = candidate
    state['completed'] = 1
    return state, result, 0
