"""Independent byte-stream decoder for the emitted stock integer commands."""
import struct


class Machine:
    def __init__(self):
        self.memory = {}
        self.steps = 0

    def execute(self, data, depth=0):
        assert depth <= 4
        data = bytes(data)
        p = 0
        while p < len(data):
            self.steps += 1
            assert self.steps < 20000000
            assert data[p:p+3] == bytes((23,0,160)), (p,data[p:p+15])
            key, op = struct.unpack_from('<HB',data,p+3)
            if op == 0:
                size = struct.unpack_from('<H',data,p+6)[0]
                payload = data[p+8:p+8+size]
                assert len(payload) == size
                self.memory.setdefault(key,bytearray()).extend(payload)
                p += 8+size
            elif op == 2:
                self.memory.pop(key,None)
                p += 6
            elif op == 1:
                self.execute(self.memory[key],depth+1)
                p += 6
            elif op == 7:
                if key==65535:return
                data=bytes(self.memory[key]);p=0;continue
            elif op == 5:
                flags, offset, count = struct.unpack_from('<BHH',data,p+6)
                base = flags&31
                assert flags&0xC0 == 0xC0 and base in (2,4,5,6,7)
                if flags&32:
                    source, start = struct.unpack_from('<HH',data,p+11)
                    operand = bytes(self.memory[source][start:start+count])
                    p += 15
                else:
                    operand = data[p+11:p+11+count]
                    p += 11+count
                assert len(operand)==count and offset+count+(base==4)<=len(self.memory[key])
                carry = 0
                for index,value in enumerate(operand):
                    before = self.memory[key][offset+index]
                    after = (value if base==2 else before+value+carry if base==4 else
                             before&value if base==5 else before|value if base==6 else before^value)
                    carry = after>>8
                    self.memory[key][offset+index] = after&255
                if base==4:
                    self.memory[key][offset+count] = carry
            elif op in (6,8):
                flags, left, lo, right, ro = struct.unpack_from('<BHHHH',data,p+6)
                assert flags&0xA0 == 0xA0
                a = struct.unpack_from('<H',self.memory[left],lo)[0]
                b = struct.unpack_from('<H',self.memory[right],ro)[0]
                assert a!=65535 and b!=65535, 'stock invalid-operand sentinel'
                condition = {2:a==b,3:a!=b,4:a<b,5:a>b,6:a<=b,7:a>=b}[flags&31]
                p += 15
                if condition:
                    if op==8:
                        if key==65535:return
                        data=bytes(self.memory[key]);p=0;continue
                    self.execute(self.memory[key],depth+1)
            else:
                raise AssertionError(op)


class ReferenceError(Exception):
    def __init__(self, status):
        self.status = status


def reference(interface, program, state, entry):
    result = bytearray(state)
    error = interface['error']['offset']
    struct.pack_into('<H',result,error,0)
    fields = interface['globals']

    def value(index):
        return struct.unpack_from('<H',result,fields[index]['offset'])[0]

    def admit(field, number):
        if not field['bounds'][0] <= number <= field['bounds'][1]:
            raise ReferenceError(32)

    def expression(node):
        if 'literal_bits' in node:
            return node['literal_bits']
        if 'global' in node:
            return value(node['global'])
        a = expression(node['left'])
        b = expression(node['right'])
        op = node['operator']
        if op in ('==','!=','<','>','<=','>='):
            return int({'==':a==b,'!=':a!=b,'<':a<b,'>':a>b,'<=':a<=b,'>=':a>=b}[op])
        candidate = a+b if op=='+' else a-b if op=='-' else a*b
        if candidate<0 or candidate>65535:
            raise ReferenceError({'+':1,'-':2,'*':3}[op])
        return candidate

    try:
        for index,field in enumerate(fields):
            admit(field,value(index))
        def statements(body):
            for statement in body:
                if 'for' in statement:
                    loop=statement['for']
                    # Independent inclusive-range iteration, rather than trusting
                    # the emitter's computed trips/last metadata.
                    stop=loop['end']+(1 if loop['step']>0 else -1)
                    for index in range(loop['start'],stop,loop['step']):
                        struct.pack_into('<H',result,fields[loop['counter']]['offset'],index)
                        statements(statement['body'])
                elif 'if' in statement:
                    condition=statement['if']
                    yes=bool(expression(condition['left']))
                    statements(statement['then'] if yes else statement['else'])
                else:
                    candidate=expression(statement['value'])
                    destination=fields[statement['destination']]
                    admit(destination,candidate)
                    struct.pack_into('<H',result,destination['offset'],candidate)
        statements(program['procedures'][entry]['assignments'])
    except ReferenceError as failure:
        struct.pack_into('<H',result,error,failure.status)
    return bytes(result)
