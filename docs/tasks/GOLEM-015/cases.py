"""Exact quotient/remainder expectations independent of buffered division."""
import random


def cases():
    edges = [(0,1,1),(1,1,1),(65535,1,1),(65535,65535,1),(65534,65535,1),
             (65535,65534,2),(32768,32767,1),(32767,32768,1),(65535,256,3),
             (65535,257,2),(256,255,1),(255,256,1),(17,5,2),(0,65535,65535),
             (65535,0,1),(0,0,1),(65535,1,0),(17,5,0)]
    edges.extend((65535,1<<bit,3) for bit in range(16))
    edges.extend((1<<bit,257,1) for bit in range(16))
    rng=random.Random(15015)
    edges.extend((rng.randrange(65536),rng.randrange(65536),rng.randrange(1,65536)) for _ in range(24))
    return [dict(zip(('numerator','denominator','nextDivisor'), values)) for values in edges]


def initial(values):
    return dict(numerator=values['numerator'],denominator=values['denominator'],
                nextDivisor=values['nextDivisor'],before=52428,quotient=48879,
                remainder=222,secondQuotient=4660,secondRemainder=56797,
                after=43690,completed=0)


def expected(values):
    state=initial(values)
    state['before']=state['numerator']
    if state['denominator']==0:return state,4660,4
    state['quotient'],state['remainder']=divmod(state['numerator'],state['denominator'])
    assert state['numerator']==state['quotient']*state['denominator']+state['remainder']
    assert 0<=state['remainder']<state['denominator']
    if state['nextDivisor']==0:return state,-1,4
    state['secondQuotient'],state['secondRemainder']=divmod(state['quotient'],state['nextDivisor'])
    state['after']=state['remainder'];state['completed']=1
    return state,-1,0
