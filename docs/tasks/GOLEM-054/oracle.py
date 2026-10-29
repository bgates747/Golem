"""Independent exact-rational float32 text oracle for GOLEM-054.

Only integer arithmetic and Fraction determine expected text.  No compiler
formatter, host printf, decimal-to-float conversion or binary floating-point
arithmetic participates in the oracle.
"""
from fractions import Fraction
import argparse
import json
import random

MIN_NORMAL = 0x00800000
MAX_FINITE = 0x7f7fffff


def _positive_fraction(magnitude):
    exponent = (magnitude >> 23) & 255
    mantissa = magnitude & 0x7fffff
    if exponent == 255:
        raise ValueError('nonfinite binary32')
    power = exponent - 150 if exponent else -149
    significand = mantissa | (0x800000 if exponent else 0)
    return (Fraction(significand << power) if power >= 0 else
            Fraction(significand, 1 << -power))


def fraction(bits, *, permit_subnormal=False):
    """Decode all four input bytes; external subnormals are normally invalid."""
    if not 0 <= bits < 1 << 32:
        raise ValueError('not a uint32 bit pattern')
    magnitude = bits & 0x7fffffff
    if 0 < magnitude < MIN_NORMAL and not permit_subnormal:
        raise ValueError('subnormal external input')
    value = _positive_fraction(magnitude)
    return -value if bits >> 31 else value


def decimal_exponent(value):
    """Return floor(log10(value)), using decimal integer lengths and comparison."""
    assert value > 0
    exponent = len(str(value.numerator)) - len(str(value.denominator))
    while value < power10(exponent):
        exponent -= 1
    while value >= power10(exponent + 1):
        exponent += 1
    return exponent


def power10(exponent):
    return Fraction(10 ** exponent) if exponent >= 0 else Fraction(1, 10 ** -exponent)


def rounded_digits(bits):
    """Return the rounded nine-digit integer and its post-round decimal exponent."""
    value = abs(fraction(bits))
    if not value:
        return 0, 0
    exponent = decimal_exponent(value)
    scaled = value * power10(8 - exponent)
    quotient, remainder = divmod(scaled.numerator, scaled.denominator)
    doubled = remainder * 2
    if doubled > scaled.denominator or (doubled == scaled.denominator and quotient & 1):
        quotient += 1
    if quotient == 1000000000:
        quotient //= 10
        exponent += 1
    assert 100000000 <= quotient <= 999999999
    return quotient, exponent


def text(bits):
    value = fraction(bits)
    if not value:
        return '0'
    quotient, exponent = rounded_digits(bits)
    digits = str(quotient)
    sign = '-' if value < 0 else ''
    if -4 <= exponent <= 8:
        point = exponent + 1
        if point <= 0:
            result = '0.' + '0' * -point + digits
        elif point < len(digits):
            result = digits[:point] + '.' + digits[point:]
        else:
            result = digits + '0' * (point - len(digits))
        if '.' in result:
            result = result.rstrip('0').rstrip('.')
        return sign + result
    tail = digits[1:].rstrip('0')
    mantissa = digits[0] + ('.' + tail if tail else '')
    return sign + mantissa + 'e' + ('+' if exponent >= 0 else '-') + str(abs(exponent)).zfill(2)


def nearest_bits(value):
    """Round an exact rational to binary32, for expression/boundary fixtures.

    Search IEEE magnitude encodings, which are monotone, then select the nearest
    neighbour with ties to even.  Subnormal results follow Golem's zero policy.
    Overflow is an explicit error; no approximate host arithmetic is used.
    """
    value = Fraction(value)
    negative = value < 0
    value = abs(value)
    if not value:
        return 0
    if value > _positive_fraction(MAX_FINITE):
        # The upper neighbour has value 2**128, although its encoding is Inf.
        distance = value - _positive_fraction(MAX_FINITE)
        if distance >= Fraction(1 << 103):
            raise OverflowError('binary32 overflow')
        magnitude = MAX_FINITE
    else:
        low, high = 0, MAX_FINITE
        while low < high:
            middle = (low + high + 1) // 2
            if _positive_fraction(middle) <= value:
                low = middle
            else:
                high = middle - 1
        magnitude = low
        if low < MAX_FINITE:
            below = value - _positive_fraction(low)
            above = _positive_fraction(low + 1) - value
            if below > above or (below == above and low & 1):
                magnitude += 1
    if magnitude < MIN_NORMAL:
        return 0
    return magnitude | (0x80000000 if negative else 0)


def tie_details(bits):
    value = abs(fraction(bits))
    if not value:
        return None
    exponent = decimal_exponent(value)
    scaled = value * power10(8 - exponent)
    quotient, remainder = divmod(scaled.numerator, scaled.denominator)
    return {'tie': remainder * 2 == scaled.denominator,
            'round_up': remainder * 2 > scaled.denominator or
                        (remainder * 2 == scaled.denominator and bool(quotient & 1)),
            'pre_exponent': exponent, 'carry': rounded_digits(bits)[1] != exponent}


def cases(random_count=128):
    """Stable, ordered values; all exponent fields plus ties and decimal edges."""
    records = []
    seen = set()

    def add(bits, category):
        magnitude = bits & 0x7fffffff
        if magnitude and not MIN_NORMAL <= magnitude <= MAX_FINITE:
            return
        if bits in seen:
            return
        seen.add(bits)
        records.append({'bits': bits, 'hex': f'{bits:08x}', 'category': category,
                        'text': text(bits)})

    for bits in (0, 0x80000000, MIN_NORMAL, MIN_NORMAL + 1, MAX_FINITE - 1,
                 MAX_FINITE, 0x3f800000, 0x3f804000, 0x3f80c000, 0x4b800000):
        add(bits, 'zero-extreme-tie')
        add(bits ^ 0x80000000, 'zero-extreme-tie')
    # Distinct significands and both signs across the entire admitted range.
    for exponent in range(1, 255):
        for mantissa in (0, 0x7fffff):
            for sign in (0, 0x80000000):
                add(sign | exponent << 23 | mantissa, 'all-binary-exponents')
    # Several consecutive bit patterns around each decimal power, including
    # fixed/scientific boundaries and values rounded across a decimal carry.
    for exponent in range(-38, 39):
        center = nearest_bits(power10(exponent))
        for delta in (-2, -1, 0, 1, 2):
            for sign in (0, 0x80000000):
                add((center + delta) | sign, 'decimal-power-neighbour')
    # Nine-digit ties arise, for example, in exact dyadic values near 1.
    for mantissa in range(0x4000, 0x800000, 0x8000):
        bits = 0x3f800000 | mantissa
        details = tie_details(bits)
        assert details['tie']
        if mantissa <= 0x1c000 or mantissa >= 0x7dc000:
            for sign in (0, 0x80000000):
                add(bits | sign, 'ties-even-up' if details['round_up'] else 'ties-even-down')
    rng = random.Random(5401)
    for _ in range(random_count):
        add((rng.randrange(2) << 31) | rng.randrange(MIN_NORMAL, MAX_FINITE + 1), 'random')
    # Repeat explicitly, because the decoder must expose stale sign/digit state.
    for bits in (0xff7fffff, 0, 0x3f800000, 0x80000000, 0x7f7fffff,
                 MIN_NORMAL, 0xbf800000, 0, 0x4cbe0000, 0x3f804000):
        records.append({'bits': bits, 'hex': f'{bits:08x}',
                        'category': 'repeated-large-small', 'text': text(bits)})
    return records


def target_cases():
    """A short representative subset for real VDP qualification and review."""
    result = []
    wanted = (0, 0x80000000, MIN_NORMAL, MIN_NORMAL + 1, 0x80800000,
              MAX_FINITE, 0xff7fffff, 0x3f800000, 0xbf800000,
              0x3f804000, 0x3f80c000, 0x4b800000,
              0x19416d9a, 0x99416d9a)
    for bits in wanted:
        result.append({'bits': bits, 'text': text(bits)})
    for exponent in (-5, -4, -3, 7, 8, 9):
        center = nearest_bits(power10(exponent))
        for delta in (-1, 0, 1):
            for sign in (0, 0x80000000):
                bits = (center + delta) | sign
                result.append({'bits': bits, 'text': text(bits)})
    for bits in (0xff7fffff, 0, 0x3f800000, 0x7f7fffff, MIN_NORMAL, 0):
        result.append({'bits': bits, 'text': text(bits)})
    return result


def capacity():
    # x=M*2^p with M<2^24 and -149<=p<=104.  For negative p,
    # multiplying M by 5^-p gives an exact finite decimal coefficient.
    assert (1 << 24) * 5 ** 149 < 10 ** 112
    assert ((1 << 24) - 1) * (1 << 104) < 10 ** 39
    return {'coefficient_capacity_digits': 112, 'allocated_coefficient_bytes': 113,
            'maximum_power5_rounds': 149, 'maximum_power2_rounds': 104,
            'maximum_significand_decimal_digits': 8, 'maximum_text_bytes': 15,
            'maximum_text_including_spare_byte': 16, 'target_cases': len(target_cases()),
            'coefficient_capacity_proof': '2^24 * 5^149 < 10^112; (2^24 - 1) * 2^104 < 10^39',
            'examples': {hex(bits): text(bits) for bits in
                         (MIN_NORMAL, MAX_FINITE, 0x19416d9a, 0x3f804000, 0x3f80c000)}}


def self_check():
    known = {
        0: '0', 0x80000000: '0', MIN_NORMAL: '1.17549435e-38',
        MAX_FINITE: '3.40282347e+38', 0xff7fffff: '-3.40282347e+38',
        0x3f800000: '1', 0x3fc00000: '1.5',
        0x3f804000: '1.00195312', 0x3f80c000: '1.00585938',
        0x4b800000: '16777216', 0x4cbebc20: '100000000',
        0x4e6e6b28: '1e+09', 0x38d1b717: '9.99999975e-05',
        0x38d1b718: '0.000100000005',
    }
    for bits, expected in known.items():
        assert text(bits) == expected, (hex(bits), text(bits), expected)
        assert nearest_bits(fraction(bits)) == (bits if bits & 0x7fffffff else 0)
    for bits in (1, 0x007fffff, 0x80000001, 0x7f800000, 0x7fc00000, 0xff800000):
        try:
            text(bits)
        except ValueError:
            pass
        else:
            raise AssertionError(f'invalid admitted input {bits:08x}')
    records = cases()
    assert {r['bits'] >> 23 & 255 for r in records if r['bits'] & 0x7fffffff} == set(range(1, 255))
    assert any(r['category'] == 'ties-even-up' for r in records)
    assert any(r['category'] == 'ties-even-down' for r in records)
    assert any(tie_details(r['bits'])['carry'] for r in records if r['bits'] & 0x7fffffff)
    for record in records:
        expected = record['bits'] if record['bits'] & 0x7fffffff else 0
        assert nearest_bits(Fraction(record['text'])) == expected
    assert max(len(r['text']) for r in records) <= 15
    return {'passed': True, 'exact_known_values': len(known), 'generated_values': len(records),
            'binary_exponent_fields': 254, 'exact_roundtrips': len(records),
            'decimal_exponent_carry_cases': sum(bool(tie_details(r['bits'])['carry'])
                                                for r in records if r['bits'] & 0x7fffffff),
            'maximum_text_bytes': max(len(r['text']) for r in records)}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--cases', action='store_true')
    parser.add_argument('--target-cases', action='store_true')
    parser.add_argument('--capacity', action='store_true')
    args = parser.parse_args()
    result = capacity() if args.capacity else target_cases() if args.target_cases else cases() if args.cases else self_check()
    print(json.dumps(result, indent=2))
