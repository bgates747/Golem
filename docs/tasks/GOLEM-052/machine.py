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
        self.text=bytearray()

    def execute(self, data, depth=0):
        assert depth <= 32
        data = bytes(data)
        p = 0
        while p < len(data):
            self.steps += 1
            assert self.steps < 20000000
            if 32<=data[p]<=126 or data[p] in (10,13):
                self.text.append(data[p]);p+=1;continue
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
            elif op == 11:
                offset=int.from_bytes(data[p+6:p+9],'little');assert offset<0x800000
                self.execute(self.memory[key][offset:],depth+1);p+=9
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
