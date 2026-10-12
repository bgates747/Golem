"""Independent integer oracle for left-to-right checked subtraction/addition."""
import random


def cases():
    boundaries = [
        (0, 0, 0, 0, 0, 9), (256, 1, 0, 256, 1, 9),
        (65535, 0, 0, 65535, 0, 9), (65535, 65535, 0, 1, 1, 9),
        (0, 1, 0, 0, 0, 9), (0, 1, 65535, 0, 0, 9),
        (0, 1, 1, 0, 0, 9), (32768, 32767, 0, 0, 0, 9),
        (32767, 32768, 0, 0, 0, 9), (65535, 1, 1, 0, 0, 9),
        (0, 0, 0, 0, 1, 9), (65535, 0, 1, 65535, 65535, 9),
        (3, 2, 1, 5, 4, 0), (3, 2, 1, 5, 4, 1),
        (65535, 65535, 65535, 65535, 65535, 9), (42, 7, 8, 9, 10, 9),
    ]
    rng = random.Random(13013)
    boundaries.extend(tuple(rng.randrange(65536) for _ in range(5)) + (9,) for _ in range(24))
    return [dict(zip(('a', 'b', 'c', 'selfValue', 'extra', 'tail'), values)) for values in boundaries]


def initial(values):
    return dict(a=values['a'], b=values['b'], c=values['c'], extra=values['extra'],
                before=52428, total=48879, selfValue=values['selfValue'], after=56797,
                tail=values['tail'], completed=0)


def expected(values):
    state = initial(values)
    state['before'] = state['a']
    candidate = state['a'] - state['b']
    if candidate < 0:
        return state, 4660, 2
    candidate += state['c']
    if candidate > 65535:
        return state, 4660, 1
    state['total'] = candidate
    result = -1
    candidate = state['selfValue'] - state['extra']
    if candidate < 0:
        return state, result, 2
    state['selfValue'] = candidate
    state['after'] = state['total']
    candidate = state['tail'] - 1
    if candidate < 0:
        return state, result, 2
    state['tail'] = candidate
    state['completed'] = 1
    return state, result, 0
