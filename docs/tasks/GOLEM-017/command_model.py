"""Test-only byte decoder/model extended from GOLEM-015 for patched table templates.
Independent of compiler emitters; not a firmware qualification.
"""
import math
import struct


def u16(data, pos=0):
    return struct.unpack_from('<H', data, pos)[0]


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


class Package:
    def __init__(self, directory, manifest):
        self.directory, self.manifest = directory, manifest
        self.owned = {item['id']: item for item in manifest['owned']}
        assert len(self.owned) == len(manifest['owned'])
        self.roles = {key: item['role'] for key, item in self.owned.items()}
        self.programs = {key for key, item in self.owned.items() if item['kind'] == 'program'}
        for key in self.owned:
            assert 0 < key < 65535
            assert all(not (r['first'] <= key <= r['last']) for r in manifest['reserved_ranges'])
        install = (directory / 'bootstrap.bin').read_bytes()
        assert install[:7] == bytes([23, 0, 248, 1, 0, 1, 0])
        self.install = self.decode(install[7:])
        clears = [op[1] for op in self.install if op[0] == 'clear']
        writes = [op[1] for op in self.install if op[0] == 'write']
        assert len(clears) == len(set(clears)) and set(clears) == self.owned.keys()
        assert len(writes) == len(set(writes)) and set(writes) == self.owned.keys()
        self.blocks = {op[1]: op[2] for op in self.install if op[0] == 'write'}
        self.code = {key: self.decode(self.blocks[key], program=True) for key in self.programs}
        self.edges = {key: {op[1] for op in code if op[0] in ('call', 'condition', 'jump_condition')}
                      for key, code in self.code.items()}
        self.depth = self._depth(manifest['entry']['id'])
        assert self.depth <= 3
        self.cleanup = self.decode((directory / 'cleanup.bin').read_bytes())
        assert self.cleanup == [('clear', key) for key in sorted(self.owned)]
        self.invoke = self.decode((directory / 'invoke.bin').read_bytes())
        assert self.invoke == [('call', manifest['entry']['id'])]
        assert manifest['bootstrap_bytes'] == len(install)
        assert manifest['cleanup_bytes'] == 6 * len(self.owned)
        assert manifest['entry']['bytes'] == len(self.blocks[manifest['entry']['id']])
        self.memory = {0: bytearray(b'loader')}
        for r in manifest['reserved_ranges']:
            for key in {r['first'], r['last']}:
                self.memory[key] = bytearray(b'external asset')
        self.external = {key: bytes(value) for key, value in self.memory.items()}
        self.text = bytearray()
        self.execute(self.install)
        self.assert_external()

    def span(self, key, offset, size):
        assert key in self.owned and size > 0
        assert offset >= 0 and offset + size <= self.owned[key]['bytes'], (key, offset, size)

    def field(self, data, pos):
        fmt, key, offset = data[pos], u16(data, pos + 1), u16(data, pos + 3)
        assert fmt in (0, 0x40, 0xc0)
        self.span(key, offset, 2 if fmt == 0xc0 else 4)
        return (fmt, key, offset)

    def decode(self, data, program=False):
        pos, result = 0, []
        while pos < len(data):
            if data[pos:pos + 3] != bytes([23, 0, 160]):
                assert program and data[pos] != 23, (pos, data[pos:pos + 10])
                result.append(('print', data[pos])); pos += 1; continue
            key, op = u16(data, pos + 3), data[pos + 5]
            assert key in self.owned
            if op == 0:
                assert not program
                size = u16(data, pos + 6)
                payload = data[pos + 8:pos + 8 + size]
                assert len(payload) == size == self.owned[key]['bytes']
                result.append(('write', key, payload)); pos += 8 + size
            elif op == 1:
                assert key in self.programs
                result.append(('call', key)); pos += 6
            elif op == 2:
                assert not program
                result.append(('clear', key)); pos += 6
            elif op == 5:
                flags, offset = data[pos + 6], u16(data, pos + 7)
                base = flags & 31
                assert base in (2, 4, 5, 6, 7)
                if flags & 0x40:
                    assert flags & 0x80
                    size = u16(data, pos + 9)
                    if flags & 0x20:
                        source, start = u16(data, pos + 11), u16(data, pos + 13)
                        self.span(source, start, size)
                        operand = ('buffer', source, start); width = 15
                    else:
                        literal = data[pos + 11:pos + 11 + size]
                        assert len(literal) == size
                        operand = ('literal', literal); width = 11 + size
                else:
                    assert flags == 7
                    size, operand, width = 1, ('literal', data[pos + 9:pos + 10]), 10
                self.span(key, offset, size + (base == 4))
                result.append(('adjust', key, offset, size, base, operand)); pos += width
            elif op in (6, 8):
                flags = data[pos + 6]
                assert key in self.programs and flags in (0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7)
                left, lo = u16(data, pos + 7), u16(data, pos + 9)
                right, ro = u16(data, pos + 11), u16(data, pos + 13)
                self.span(left, lo, 2); self.span(right, ro, 2)
                result.append(('condition' if op == 6 else 'jump_condition', key, flags & 31, left, lo, right, ro)); pos += 15
            elif op == 32:
                self.span(key, 0, 36)
                operation = data[pos + 6]
                if operation == 0:
                    result.append(('identity', key)); pos += 7
                else:
                    assert operation == 0x66
                    result.append(('translate', key, self.field(data, pos + 7), self.field(data, pos + 12)))
                    pos += 17
            elif op == 34:
                assert data[pos + 6:pos + 9] == bytes([0x20, 1, 1])
                self.span(key, 0, 4)
                result.append(('load', key, self.field(data, pos + 9))); pos += 14
            elif op == 41:
                assert data[pos + 6] == 8 and data[pos + 7] in (0x40, 0xc0)
                fmt, matrix, source = data[pos + 7], u16(data, pos + 8), u16(data, pos + 10)
                assert u16(data, pos + 12) == 1
                width = 8 if fmt == 0x40 else 4
                self.span(key, 0, width); self.span(matrix, 0, 36); self.span(source, 0, width)
                result.append(('transform', key, fmt, matrix, source)); pos += 14
            else:
                raise AssertionError((pos, op))
        assert pos == len(data)
        return result

    def _depth(self, key, ancestors=()):
        assert key not in ancestors, 'recursive helper graph'
        return max([1, *[self._depth(op[1], (*ancestors, key)) + (op[0] != 'jump_condition')
                        for op in self.code[key] if op[0] in ('call', 'condition', 'jump_condition')]])

    def normalized(self):
        def role(key): return self.roles[key]
        def field(value): return value[0], role(value[1]), value[2]
        def normalize(op):
            kind = op[0]
            if kind == 'print': return op
            values = [kind, role(op[1]), *op[2:]]
            if kind == 'adjust' and op[5][0] == 'buffer': values[5] = ('buffer', role(op[5][1]), op[5][2])
            if kind in ('condition', 'jump_condition'): values[3], values[5] = role(op[3]), role(op[5])
            if kind == 'translate': values[2], values[3] = field(op[2]), field(op[3])
            if kind == 'load': values[2] = field(op[2])
            if kind == 'transform': values[3], values[4] = role(op[3]), role(op[4])
            return tuple(values)
        # Normalize decoded ID operands only, never arbitrary bytes/constants.
        return {role(key): [normalize(op) for op in self.code[key]] if key in self.programs else self.blocks[key]
                for key in self.owned}

    def value(self, field):
        fmt, key, offset = field
        return struct.unpack_from({0: '<f', 0x40: '<i', 0xc0: '<h'}[fmt], self.memory[key], offset)[0]

    def execute(self, code, depth=0):
        assert depth <= 3
        for op in code:
            kind = op[0]
            if kind == 'print': self.text.append(op[1]); continue
            key = op[1]
            if kind == 'clear': self.memory.pop(key, None)
            elif kind == 'write': self.memory.setdefault(key, bytearray()).extend(op[2])
            elif kind == 'call': self.execute(self.decode(self.memory[key], program=True), depth + 1)
            elif kind in ('condition', 'jump_condition'):
                _, _, comparison, left, lo, right, ro = op
                a, b = u16(self.memory[left], lo), u16(self.memory[right], ro)
                assert a != 65535 and b != 65535, 'stock conditional invalid-operand sentinel'
                if {2:a==b, 3:a!=b, 4:a<b, 5:a>b, 6:a<=b, 7:a>=b}[comparison]:
                    self.execute(self.decode(self.memory[key], program=True), depth if kind == 'jump_condition' else depth + 1)
                    if kind == 'jump_condition': return
            elif kind == 'adjust':
                _, _, offset, size, base, operand = op
                data = (bytes(self.memory[operand[1]][operand[2]:operand[2] + size])
                        if operand[0] == 'buffer' else operand[1])
                assert len(data) == size
                carry = 0
                for i in range(size):
                    before, value = self.memory[key][offset + i], data[i]
                    if base == 2: after = value
                    elif base == 4: after = before + value + carry; carry = after >> 8
                    elif base == 5: after = before & value
                    elif base == 6: after = before | value
                    else: after = before ^ value
                    self.memory[key][offset + i] = after & 255
                if base == 4: self.memory[key][offset + size] = carry
            elif kind == 'identity': self.memory[key][:] = struct.pack('<9f', 1, 0, 0, 0, 1, 0, 0, 0, 1)
            elif kind == 'translate':
                tx, ty = self.value(op[2]), self.value(op[3])
                matrix = list(struct.unpack('<9f', self.memory[key]))
                matrix[2], matrix[5] = f32(matrix[2] + tx), f32(matrix[5] + ty)
                self.memory[key][:] = struct.pack('<9f', *matrix)
            elif kind == 'load': self.memory[key][:] = struct.pack('<f', self.value(op[2]))
            elif kind == 'transform':
                _, _, fmt, matrix, source = op
                tx, ty = struct.unpack('<9f', self.memory[matrix])[2:6:3]
                width, signed = (4, '<ii') if fmt == 0x40 else (2, '<hh')
                x, y = struct.unpack(signed, self.memory[source])
                output = [math.trunc(f32(x + tx)), math.trunc(f32(y + ty))]
                self.memory[key][:] = b''.join((v & ((1 << (8 * width)) - 1)).to_bytes(width, 'little') for v in output)
            else: raise AssertionError(kind)

    def assert_external(self):
        assert all(bytes(self.memory[key]) == data for key, data in self.external.items())
