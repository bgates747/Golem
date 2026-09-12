"""Generate a readable typed-array fixture, not a VDU emitter/compiler."""
from pathlib import Path
import struct
root=Path(__file__).resolve().parent
s='''// Dynamic byte-offset lookup and unbiased unsigned floor qualification.
Buffer input(1000,80);
Field index(input,0,u16);
Field position(input,4,s32,0,3276799);
Field value(input,8,f32,0,65534);
Buffer output(1001,80);
Field outA(output,0,s32,-100,100);
Field outB(output,4,s32,-100,100);
Field outC(output,8,s32,-100,100);
Field floorValue(output,12,u16,0,65534);
Field interval(output,14,u16,0,512);
Field wideIndex(output,16,s32,0,65535);
Field wideFloor(output,20,s32,0,65534);
Buffer status(1002,10);
Field ok(status,0,u16,0,1);
Buffer untouched(1003,2);
Buffer constants(1004,2);
Field one(constants,0,u16,1,1);
Set(one,1);
Buffer records(1500,384);
'''
raw=b''.join(struct.pack('<iii',i+1,2*i+1,-i-1) for i in range(32))
s+='Bytes(records,'+','.join(map(str,raw))+');\n'
for i in range(32):
 for j,letter in enumerate('abc'):s+=f'Field {letter}{i}(records,{12*i+4*j},s32,-100,100);\n'
s+='''Buffer selected(1502,12);
Field a(selected,0,s32,-100,100);
Field b(selected,4,s32,-100,100);
Field c(selected,8,s32,-100,100);
Matrix left(1100,1,1);
Matrix right(1101,1,1);
Matrix product(1102,1,1);
Matrix rawValue(1103,1,1);
Matrix progress(1104,1,1);
Matrix fraction(1105,1,1);
Program compute(2000) {
    LoadElement(selected,records,index,ok);
    Copy(outA,a,4); Copy(outB,b,4); Copy(outC,c,4);
    WidenUnsigned(wideIndex,index);
    MatLoad(rawValue,value);
    StoreU16Floor(floorValue,rawValue);
    WidenUnsigned(wideFloor,floorValue);
    MatLoad(progress,position);
    MatScale(fraction,progress,0.00015625);
    StoreU16Floor(interval,fraction);
    CallIf(multiply,ok,eq,one);
};
Program multiply(2001) {
    MatLoad(left,a); MatLoad(right,b); MatMul(product,left,right);
};
'''
(root/'kernel.golem').write_text(s)
