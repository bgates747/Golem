#include "../src/stock_matrix.hpp"
#include <cassert>
#include <iostream>
using namespace golem::stock_matrix;

// Small stock-command executor for control-flow/addressing contracts only.
// Matrix numerical behavior is qualified separately with unchanged ESP32 code.
struct Machine {
    std::map<unsigned,Bytes> memory;
    struct Frame {unsigned id;size_t pc;};
    std::vector<Frame> stack;
    std::vector<std::pair<unsigned,unsigned>> copies;
    unsigned matrixCalls=0,maxDepth=0;
    explicit Machine(const Result &r):memory(r.blocks) {if(r.input)memory[r.input]=r.values;}
    unsigned byte() {auto &f=stack.back();assert(f.pc<memory.at(f.id).size());return memory.at(f.id)[f.pc++];}
    unsigned wordValue() {unsigned lo=byte();return lo+256*byte();}
    unsigned read16(unsigned id,unsigned at) {return memory.at(id).at(at)+256*memory.at(id).at(at+1);}
    void run(unsigned entry) {
        assert(stack.empty());stack.push_back({entry,0});unsigned steps=0;
        while(!stack.empty()) {
            assert(++steps<2000000);maxDepth=std::max(maxDepth,unsigned(stack.size()));
            if(stack.back().pc==memory.at(stack.back().id).size()) {stack.pop_back();continue;}
            assert(byte()==23);assert(byte()==0);assert(byte()==160);
            unsigned id=wordValue(),op=byte();
            if(op==1)stack.push_back({id,0});
            else if(op==7) {assert(id!=65535);stack.back()={id,0};}
            else if(op==8 || op==10) {
                unsigned target=0;
                if(op==10) {target=byte();target+=byte()<<8;target+=byte()<<16;}
                unsigned kind=byte(),check=wordValue(),at=wordValue(),right=wordValue(),from=wordValue();
                unsigned left=read16(check,at),rhs=read16(right,from);
                // Model stock's partially initialized signed-int sentinel quirk.
                assert(left!=65535 && rhs!=65535);
                bool take=kind==0xa2 ? left==rhs : kind==0xa3 ? left!=rhs : kind==0xa6 ? left<=rhs : false;
                assert(kind==0xa2 || kind==0xa3 || kind==0xa6);
                if(take) {
                    if(id==65535) {assert(op==8);stack.pop_back();}
                    else stack.back()={id,op==10 ? target : 0};
                }
            } else if(op==5) {
                unsigned kind=byte(),at=wordValue(),count=wordValue();Bytes values;
                if(kind==0xe2) {
                    unsigned source=wordValue(),from=wordValue();
                    for(unsigned i=0;i<count;++i)values.push_back(memory.at(source).at(from+i));
                    if(id==4101)copies.push_back({at,from});
                } else {assert(kind==0xc2 || kind==0xc4);for(unsigned i=0;i<count;++i)values.push_back(byte());}
                unsigned carry=0;
                for(unsigned i=0;i<count;++i) {
                    auto &target=memory.at(id).at(at+i);
                    if(kind==0xc4) {unsigned sum=target+values[i]+carry;target=sum&255;carry=sum>>8;}
                    else target=values[i];
                }
                if(kind==0xc4)memory.at(id).at(at+count)=carry;
            } else {assert((op==32 || op==33) && byte()==0);++matrixCalls;}
        }
    }
};
std::string source(unsigned n) {
    return "uint16 i = 9 Matrix(3,3) m = Identity Borrowed XYZ16 src = Buffer(4100,"+std::to_string(n)+") "
      "Borrowed XYZ16 dst = Buffer(4101,"+std::to_string(n)+") Sub frame worker() matrix_identity(m) EndSub "
      "Sub worker For i In Range(0:"+std::to_string(n-1)+") copy_bytes(dst,6*i,src,6*i,6) Next i EndSub";
}
int main() {
    auto small=compile(source(30),{{4100,4101}}),large=compile(source(2685),{{4100,4101}});
    assert(small.bootstrap.size()==large.bootstrap.size() && small.owned.size()==large.owned.size());
    assert(large.loops.size()==1 && large.loops[0].iterations==2685 && large.loops[0].cursors==2);
    Machine vm(large);Bytes data(2685*6);for(unsigned i=0;i<data.size();++i)data[i]=(i*31+i/251)&255;
    vm.memory[4100]=data;vm.memory[4101]=Bytes(data.size(),0);
    vm.run(large.entries.front().id);
    assert(vm.memory[4101]==data && vm.copies.size()==2685 && vm.matrixCalls==1 && vm.maxDepth==3);
    for(unsigned i=0;i<2685;++i)assert(vm.copies[i]==std::make_pair(i*6,i*6));
    unsigned counter=large.loops[0].counter;assert(vm.read16(counter,0)==2684 && vm.memory[counter][2]==0);
    // Poison counters/cursors and output; a second root call must reset itself.
    vm.memory[counter]=Bytes(3,255);vm.memory[large.loops[0].state]=Bytes(vm.memory[large.loops[0].state].size(),255);
    for(auto &b:vm.memory[4100])b^=255;
    vm.memory[4101]=Bytes(data.size(),0);
    vm.run(large.entries.front().id);assert(vm.memory[4101]==vm.memory[4100] && vm.copies.size()==5370);
    assert(vm.read16(counter,0)==2684 && vm.memory[counter][2]==0);
    for(unsigned i=0;i<large.loops[0].cursors;++i)assert(vm.memory[large.loops[0].state][8+3*i]==0);
    for(auto range:{std::make_pair(65534u,65535u),std::make_pair(0u,65535u),std::make_pair(23u,23u)}) {
        auto r=compile("uint16 i = 7 Matrix(3,3) m = Identity Sub tick For i In Range("+std::to_string(range.first)+":"+std::to_string(range.second)+":1) matrix_identity(m) Next matrix_identity(m) EndSub");
        Machine machine(r);machine.run(r.entries.front().id);
        assert(machine.matrixCalls==range.second-range.first+2 && machine.read16(r.loops[0].counter,0)==range.second);
        assert(machine.memory[r.loops[0].counter][2]==0 && machine.maxDepth==2);
    }
    auto sequential=compile("uint16 i = 7 Matrix(3,3) m = Identity Sub tick For i In Range(2:4) matrix_identity(m) Next i For i In Range(8:9) matrix_identity(m) Next EndSub");
    Machine two(sequential);two.run(sequential.entries.front().id);two.run(sequential.entries.front().id);
    assert(two.matrixCalls==10 && two.read16(sequential.loops[0].counter,0)==9);
    unsigned rejected=0;
    auto bad=[&](std::string text) {try {compile(text,{{4100,4101}});}catch(const std::exception&){++rejected;return;}throw std::runtime_error("unexpected loop acceptance: "+text);};
    auto change=[&](std::string from,std::string to) {auto text=source(30);auto at=text.find(from);assert(at!=text.npos);text.replace(at,from.size(),to);return text;};
    bad(change("uint16 i = 9","uint16 i"));bad(change("uint16 i = 9","float32 i = 9"));
    bad(change("Range(0:29)","Range(29:0)"));bad(change("Range(0:29)","Range(0:29:2)"));
    bad(change("Range(0:29)","Range(0:29:-1)"));bad(change("Range(0:29)","Range(0:i)"));
    bad(change("Next i","Next other"));bad(change("6*i","i*6"));bad(change("6*i","6*i-1"));
    bad(change("6*i","0*i"));bad(change("6*i","65535*i"));bad(change("6*i","6*i+1"));
    bad(change("copy_bytes(dst,6*i,src,6*i,6)","worker()"));
    bad(change("copy_bytes(dst,6*i,src,6*i,6)","For i In Range(0:1) matrix_identity(m) Next i"));
    bad(change("copy_bytes(dst,6*i,src,6*i,6)","copy_bytes(src,6*i,src,6*i,6)"));
    bad(change("matrix_identity(m) EndSub","copy_bytes(dst,6*i,src,0,1) EndSub"));
    const std::string floatSource="uint16 i = 0 Matrix(3,3) m = Identity Borrowed XYZF32 src = Buffer(4100,30) Sub tick For i In Range(0:29) matrix_set_value(m,m,0,0,read_float32(src,12*i+8)) Next i EndSub";
    auto view=compile(floatSource,{{4100,4100}});assert(view.loops[0].cursors==1);
    auto incorrect=[&](std::string from,std::string to) {auto text=floatSource;auto at=text.find(from);assert(at!=text.npos);text.replace(at,from.size(),to);bad(text);};
    incorrect("12*i+8","12*i+9");incorrect("12*i+8","13*i");incorrect("12*i+8","12*i+12");
    incorrect("12*i+8","12*other");incorrect("12*i+8","read_float32(src,0)");
    bad("uint16 i = 0 Matrix(3,3) m = Identity Sub tick For i In Range(0:65535) matrix_identity(m) matrix_identity(m) Next i EndSub");
    const std::string bounded="uint16 i = 0 Matrix(3,3) m = Identity Sub tick For i In Range(0:63) ";
    std::string repeated;for(unsigned i=0;i<1024;++i)repeated+="matrix_identity(m) ";
    auto capacity=compile(bounded+repeated+"Next i EndSub");Machine cap(capacity);cap.run(capacity.entries.front().id);
    assert(cap.matrixCalls==65536 && cap.read16(capacity.loops[0].counter,0)==63);
    bad(bounded+repeated+"matrix_identity(m) Next i EndSub");
    auto tooMuch=bounded;tooMuch.replace(tooMuch.find("0:63"),4,"0:64");bad(tooMuch+repeated+"Next i EndSub");
    bad(change("Range(0:29)","Range(0:65536)"));
    std::cout<<"Compact stock loops: full2685 copies/repeat,65536-iteration termination/carries/continuation,constant footprint and "<<rejected<<" rejections pass.\n";
}
