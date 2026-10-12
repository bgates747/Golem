"""Exact integer expectations, independent of the emitted multiplication routine."""
import random


def cases():
    boundaries = [
        (0, 0, 65535, 65535, 0, 0), (0, 1, 65535, 1, 1, 1),
        (0, 65535, 1, 0, 65535, 9), (0, 65535, 65535, 1, 1, 9),
        (0, 255, 257, 255, 257, 9), (0, 256, 256, 1, 1, 9),
        (0, 32767, 2, 1, 1, 9), (0, 32768, 2, 1, 1, 9),
        (1, 1, 65535, 1, 1, 9), (65535, 0, 65535, 1, 1, 9),
        (0, 1, 1, 65535, 2, 9), (0, 1, 1, 65535, 1, 9),
        (1, 2, 3, 4, 5, 32767), (1, 2, 3, 4, 5, 32768),
        (0, 128, 256, 256, 255, 9), (0, 257, 255, 256, 256, 9),
    ]
    boundaries.extend((0, 1, 1 << bit, 1, 1, 9) for bit in range(16))
    rng = random.Random(14014)
    boundaries.extend(tuple(rng.randrange(65536) for _ in range(5)) + (9,) for _ in range(8))
    boundaries.extend((rng.randrange(65536), rng.randrange(256), rng.randrange(256),
                       rng.randrange(256), rng.randrange(256), 9) for _ in range(8))
    boundaries.extend((0, rng.randrange(256), rng.randrange(256),
                       rng.randrange(65536), rng.randrange(65536), 9) for _ in range(8))
    return [dict(zip(('a', 'b', 'c', 'selfValue', 'extra', 'tail'), values)) for values in boundaries]


def initial(values):
    return dict(a=values['a'], b=values['b'], c=values['c'], extra=values['extra'],
                before=52428, total=48879, selfValue=values['selfValue'], after=56797,
                tail=values['tail'], completed=0)


def expected(values):
    state = initial(values)
    state['before'] = state['a']
    product = state['b'] * state['c']
    if product > 65535:
        return state, 4660, 3
    candidate = state['a'] + product
    if candidate > 65535:
        return state, 4660, 1
    state['total'] = candidate
    result = -1
    candidate = state['selfValue'] * state['extra']
    if candidate > 65535:
        return state, result, 3
    state['selfValue'] = candidate
    state['after'] = state['total'] - state['c'] * state['b']
    candidate = state['tail'] * 2
    if candidate > 65535:
        return state, result, 3
    state['tail'] = candidate
    state['completed'] = 1
    return state, result, 0
