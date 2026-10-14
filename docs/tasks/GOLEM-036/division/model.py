"""Independent byte-stream decoder for the emitted stock integer commands."""
import struct
import math

def float_value(bits):return struct.unpack("<f",struct.pack("<I",bits))[0]
def float_bits(value):
    try:return struct.unpack("<I",struct.pack("<f",value))[0]
    except OverflowError:return 0xff800000 if value<0 else 0x7f800000
def normalize(bits,input=False):
    magnitude=bits&0x7fffffff
    if magnitude>=0x7f800000 or (input and 0<magnitude<0x00800000):raise ReferenceError(33)
    return 0 if magnitude<0x00800000 else bits



class Machine:
    def __init__(self):
        self.memory = {}
        self.matrices = {}
        self.steps = 0

    def execute(self, data, depth=0):
        assert depth <= 32
        data = bytes(data)
        p = 0
        while p < len(data):
            self.steps += 1
            assert self.steps < 20000000
            if data[p:p+3]==bytes((23,0,248)):
                assert data[p:p+7]==bytes((23,0,248,1,0,1,0));p+=7;continue
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
                self.matrices.pop(key,None)
                p += 6
            elif op == 1:
                self.execute(self.memory[key],depth+1)
                p += 6
            elif op == 7:
                if key==65535:return
                data=bytes(self.memory[key]);p=0;continue
            elif op == 34:
                operation,rows,columns=data[p+6:p+9];p+=9
                if operation==32:
                    fmt,source,offset=struct.unpack_from('<BHH',data,p);p+=5
                    assert fmt in (0,64)
                    values=[]
                    for index in range(rows*columns):
                        bits=struct.unpack_from('<I',self.memory[source],offset+index*4)[0]
                        if fmt==64:bits=float_bits(bits-4294967296 if bits>=2147483648 else bits)
                        values.append(bits)
                else:
                    source=struct.unpack_from('<H',data,p)[0];p+=2
                    assert self.matrices[source]==(rows,columns)
                    left=list(struct.unpack('<'+'I'*(rows*columns),self.memory[source]))
                    if operation==39:
                        fmt,other,offset=struct.unpack_from('<BHH',data,p);p+=5;assert fmt==0
                        b=float_value(struct.unpack_from('<I',self.memory[other],offset)[0])
                        values=[float_bits(float_value(a)*b) for a in left]
                    else:
                        assert operation in (4,5)
                        other=struct.unpack_from('<H',data,p)[0];p+=2
                        assert self.matrices[other]==(rows,columns)
                        right=list(struct.unpack('<'+'I'*(rows*columns),self.memory[other]))
                        values=[float_bits(float_value(a)+float_value(b) if operation==4 else float_value(a)-float_value(b)) for a,b in zip(left,right)]
                self.memory[key]=bytearray(struct.pack('<'+'I'*len(values),*values));self.matrices[key]=(rows,columns)
            elif op == 41:
                options,fmt,transform,source,sets=struct.unpack_from('<BBHHH',data,p+6);p+=14
                assert (options,fmt,sets)==(8,64,1) and self.matrices[transform]==(2,2)
                matrix=struct.unpack('<4f',self.memory[transform]);x=struct.unpack('<i',self.memory[source])[0]
                value=matrix[0]*x+matrix[1]
                assert 0<=value<65536
                self.memory[key]=bytearray(struct.pack('<I',int(value)))
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


def source_reference(interface, program, state, entry,arrays=None):
    result = bytearray(state)
    error = interface['error']['offset']
    struct.pack_into('<H',result,error,0)
    fields = interface['globals']

    def value(index):
        return struct.unpack_from('<I' if fields[index]['bytes']==4 else '<H',result,fields[index]['offset'])[0]

    def signed(v,bits=16):return v-(1<<bits) if v>=(1<<(bits-1)) else v

    def admit(field, number):
        if field["type"]=="float32":number=float_value(normalize(number,True))
        if field["type"] in ("sint16","sint32"):number=signed(number,32 if field["type"]=="sint32" else 16)
        if not field['bounds'][0] <= number <= field['bounds'][1]:
            raise ReferenceError(32)

    def expression(node):
        if 'literal_bits' in node:
            return normalize(node['literal_bits']) if node['type']=='float32' else node['literal_bits']
        if 'global' in node:
            return normalize(value(node['global'])) if node['type']=='float32' else value(node['global'])
        if 'array' in node:
            field,offset=array_offset(node['array'],node['index'])
            return struct.unpack_from('<I' if field['element_bytes']==4 else '<H',arrays[field['buffer']],offset)[0]
        a = expression(node['left'])
        op = node['operator']
        integer_types={'uint16':(0,65535,16),'sint16':(-32768,32767,16),'uint32':(0,4294967295,32),'sint32':(-2147483648,2147483647,32)}
        if op in integer_types and node['left']['type'] in integer_types:
            source=node['left']['type'];n=signed(a,integer_types[source][2]) if source.startswith('sint') else a
            low,high,bits=integer_types[op]
            if not low<=n<=high:raise ReferenceError(32)
            return n&((1<<bits)-1)
        if op in ('sint16','uint16','float32') and (node['type']=='float32' or node['left']['type']=='float32'):
            source=node['left']['type']
            if node['type']=='float32':return a if source=='float32' else float_bits(signed(a) if source=='sint16' else a)
            candidate=math.trunc(float_value(a));low,high=(-32768,32767) if node['type']=='sint16' else (0,65535)
            if not low<=candidate<=high:raise ReferenceError(32)
            return candidate&65535
        if op in ('sint16','uint16'):
            if node['type']!=node['left']['type'] and a>32767:raise ReferenceError(32)
            return a
        if op == 'Not':return int(not a)
        if op == 'And' and not a:return 0
        if op == 'Or' and a:return 1
        b = expression(node['right'])
        if op in ('And','Or'):return b
        if node['left']['type'] in ('sint16','sint32'):
            bits=32 if node['left']['type']=='sint32' else 16;a=signed(a,bits);b=signed(b,bits)
        if node['left']['type']=='float32':a=float_value(a);b=float_value(b)
        if op in ('==','!=','<','>','<=','>='):
            return int({'==':a==b,'!=':a!=b,'<':a<b,'>':a>b,'<=':a<=b,'>=':a>=b}[op])
        candidate = a+b if op=='+' else a-b if op=='-' else a*b
        if node['type']=='float32':return normalize(float_bits(candidate))
        low,high,bits=integer_types[node['type']]
        if candidate<low or candidate>high:
            raise ReferenceError({'+':1,'-':2,'*':3}[op])
        return candidate&((1<<bits)-1)

    def array_offset(index,node):
        field=interface['arrays'][index];number=expression(node)
        if node['type']=='sint16':number=signed(number)
        if number<field['lower'] or number>field['upper']:raise ReferenceError(5)
        return field,(number-field['lower'])*field['element_bytes']

    try:
        for index,field in enumerate(fields):
            admit(field,value(index))
        def statements(body):
            for statement in body:
                if 'divmod' in statement:
                    pair=statement['divmod']
                    n=expression(pair['numerator']);d=expression(pair['denominator'])
                    if not d:raise ReferenceError(4)
                    kind=pair['numerator']['type'];bits=32 if kind in ('uint32','sint32') else 16
                    if kind=='sint32':
                        n=signed(n,32);d=signed(d,32)
                        if n==-(1<<31) and d==-1:raise ReferenceError(6)
                        q=abs(n)//abs(d)*(1 if (n<0)==(d<0) else -1);r=n-q*d
                    else:q,r=divmod(n,d)
                    q&=(1<<bits)-1;r&=(1<<bits)-1
                    qfield=fields[pair['quotient']];rfield=fields[pair['remainder']]
                    admit(qfield,q);admit(rfield,r)
                    struct.pack_into('<I' if qfield['bytes']==4 else '<H',result,qfield['offset'],q)
                    struct.pack_into('<I' if rfield['bytes']==4 else '<H',result,rfield['offset'],r)
                elif 'for' in statement:
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
                    if 'array_destination' in statement:
                        destination,offset=array_offset(statement['array_destination'],statement['index'])
                        candidate=expression(statement['value'])
                        struct.pack_into('<I' if destination['element_bytes']==4 else '<H',arrays[destination['buffer']],offset,candidate)
                        continue
                    candidate=expression(statement['value'])
                    destination=fields[statement['destination']]
                    admit(destination,candidate)
                    struct.pack_into('<I' if destination['bytes']==4 else '<H',result,destination['offset'],candidate)
        statements(program['procedures'][entry]['assignments'])
    except ReferenceError as failure:
        struct.pack_into('<H',result,error,failure.status)
    return bytes(result)


def reference(interface, program, state, entry, root, named=True,arrays=None):
    """Independent structured protocol oracle around the frozen source semantics."""
    result=bytearray(state)
    def get(at,fmt='H'):return struct.unpack_from('<'+fmt,result,at)[0]
    def put(at,value,fmt='H'):struct.pack_into('<'+fmt,result,at,value)
    if named:put(28,entry)
    put(12,0)
    checks=[(1,get(0)!=0x4752),(2,get(2)!=1),
            (3,get(4)!=112 or get(6)!=len(result) or get(26)!=4),
            (6,get(30)!=0),(3,get(36)!=len(program['procedures']) or get(38)!=len(result)-112),
            (6,get(40,'I')!=0 or get(44,'I')!=0x52323530)]
    for code,bad in checks:
        if bad:put(12,code);return bytes(result)
    expected=bytearray(64)
    for i,b in enumerate(root['bindings']):
        struct.pack_into('<HHIIHH',expected,16*i,b['role'],b['buffer'],b['offset'],b['bytes'],b['access'],0)
    if result[48:112]!=expected:put(12,5);return bytes(result)
    if get(8,'I')==0:put(12,4);return bytes(result)
    if get(28)>=len(program['procedures']):put(12,7);return bytes(result)
    result=bytearray(source_reference(interface,program,result,get(28),arrays))
    if get(14):return bytes(result)
    put(20,(get(20,'I')+1)&0xffffffff,'I');put(24,get(28));put(32,get(8,'I'),'I')
    return bytes(result)
