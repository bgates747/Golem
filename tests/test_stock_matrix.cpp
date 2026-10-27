#include "../src/stock_matrix.hpp"
#include <cassert>
#include <iostream>
using namespace golem::stock_matrix;
const std::string source=R"(
float32 angle = 90
float32 zoom = 2
Matrix(4,4) model = Identity
Matrix(4,4) screen = Identity
Borrowed XYZ16 vertices = Buffer(1000,8)
Borrowed XYZ16 projected = Buffer(1001,8)
Sub transformFrame
matrix_identity(model)
matrix_rotate(model, 0, angle, 0)
matrix_scale(model, zoom, 1, -1)
matrix_translate(screen, 160, 120, 0)
matrix_multiply(screen, screen, model)
transform_vertices(projected, screen, vertices)
EndSub
)";
// Exact sequence independently executed against stock2.16 by fsim, 2026-10-25.
// SHA256 e75851aa170b1759e21a39c12dec9f0df7f081ea429310d7e684f78db724a9c3.
unsigned perspectiveChecks() {
    const std::string inverse=R"(
Matrix(3,3) base = Identity
Matrix(3,3) a = Identity
Borrowed XYZF32 clipped = Buffer(103,1)
Borrowed XY16 zero = Buffer(104,1)
Borrowed XY16 pixel = Buffer(105,1)
Sub project
 matrix_set_value(a,base,0,0,read_float32(clipped,8))
 matrix_set_value(a,a,1,1,read_float32(clipped,8))
 matrix_set_value(a,a,0,2,read_float32(clipped,0))
 matrix_set_value(a,a,1,2,read_float32(clipped,4))
 matrix_invert(a)
 transform_vertices(pixel,a,zero)
EndSub
)";
    auto r=compile(inverse,{{1,100},{103,105}});
    assert(r.resources[0].id==101 && r.resources[1].id==102);
    std::istringstream hex("17 00 a0 66 00 22 21 03 03 65 00 00 00 00 67 00 08 00 "
      "17 00 a0 66 00 22 21 03 03 66 00 01 01 00 67 00 08 00 "
      "17 00 a0 66 00 22 21 03 03 66 00 00 02 00 67 00 00 00 "
      "17 00 a0 66 00 22 21 03 03 66 00 01 02 00 67 00 04 00 "
      "17 00 a0 66 00 20 01 17 00 a0 69 00 29 08 c0 66 00 68 00 01 00");
    Bytes expected;unsigned byte;while(hex>>std::hex>>byte)expected.push_back(byte);
    assert(expected.size()==93 && r.blocks.at(r.entries.front().id)==expected);
    assert(r.resources[0].byteSize()==36 && r.resources[2].byteSize()==12);
    assert(r.resources[0].typeTag()==8 && r.resources[2].typeTag()==9 && r.resources[3].typeTag()==10);
    auto replaced=[&](std::string from,std::string to) {auto text=inverse;auto at=text.find(from);assert(at!=text.npos);text.replace(at,from.size(),to);return text;};
    unsigned rejected=0;
    auto bad=[&](const std::string &text) {try {compile(text,{{1,100},{103,105}});}catch(const std::exception&){++rejected;return;}throw std::runtime_error("accepted invalid perspective source: "+text);};
    bad(replaced("clipped,8","clipped,9"));bad(replaced("clipped,8","clipped,12"));
    bad(replaced("clipped,8","zero,0"));bad(replaced("clipped,8","missing,0"));
    bad(replaced("base,0,0","base,3,0"));bad(replaced("base,0,0","base,0,3"));
    bad(replaced("Matrix(3,3) a","Matrix(4,4) a"));
    bad(replaced("XYZF32 clipped","XYZ16 clipped"));bad(replaced("XY16 pixel","XYZ16 pixel"));
    bad(replaced("103,1","103,5462"));bad(replaced("104,1","104,16384"));
    bad(replaced("matrix_invert(a)","copy_bytes(pixel,1,zero,0,4)"));
    bad(replaced("matrix_invert(a)","copy_bytes(pixel,0,zero,1,4)"));
    bad(replaced("matrix_invert(a)","copy_bytes(pixel,0,zero,0,0)"));
    bad(replaced("matrix_invert(a)","copy_bytes(pixel,1,pixel,0,3)"));
    bad(replaced("matrix_invert(a)","copy_bytes(a,0,zero,0,4)"));
    bad(replaced("matrix_invert(a)","project()"));bad(replaced("matrix_invert(a)","missing()"));
    bad(inverse+"Sub cycle project() cycle() EndSub");
    bad(inverse+"Sub one two() EndSub Sub two one() EndSub");
    bad(inverse+"Sub one two() EndSub Sub two three() EndSub Sub three four() EndSub Sub four project() EndSub");
    bad(replaced("Matrix(3,3) a = Identity","Matrix(3,3) a = matrix_values(1,0,0,0,1,0,0,0)"));
    bad(replaced("Matrix(3,3) a = Identity","Matrix(3,3) a = matrix_values(1,0,0,0,1,0,0,0,1,0)"));
    bad(replaced("Matrix(3,3) a = Identity","Matrix(3,3) a = matrix_values(1,0,0,0,1,0,0,0,1e40)"));
    const std::string leaf="Matrix(3,3) m = matrix_values(1,0,0,0,1,0,0,0,1) ";
    auto nested=compile(leaf+"Sub frame child() child() EndSub Sub child matrix_invert(m) EndSub");
    Bytes calls=call(nested.entries[1].id);append(calls,call(nested.entries[1].id));
    assert(nested.blocks.at(nested.entries[0].id)==calls);
    bad(leaf+"Sub frame Child() EndSub Sub child matrix_invert(m) EndSub");
    auto limit=compile("Matrix(4,4) m = Identity Borrowed XYZF32 src = Buffer(103,5461) Borrowed XYZF32 dst = Buffer(104,5461) Sub tick transform_vertices(dst,m,src) EndSub",{{103,104}});
    assert(limit.resources[1].byteSize()==65532 && limit.resources[1].format==0);
    auto maximum=compile("Matrix(3,3) m = Identity Borrowed XY16 src = Buffer(103,16383) Borrowed XY16 dst = Buffer(104,16383) Sub tick transform_vertices(dst,m,src) EndSub",{{103,104}});
    assert(maximum.resources[1].byteSize()==65532);
    auto controls=compile("float32 x = 0 float32 read_float32 = 1 Matrix(3,3) m = Identity Sub tick matrix_set_value(m,m,0,2,read_float32) matrix_translate(m,read_float32(x,0),2) EndSub");
    const auto &set=controls.blocks.at(controls.entries.front().id);
    assert(set[6]==0x21 && set[14]==controls.input && set[16]==4);
    auto same=compile(inverse+"Sub copy copy_bytes(pixel,0,pixel,0,4) EndSub",{{1,100},{103,105}});
    assert(same.blocks.at(same.entries.back().id).size()==15);
    return rejected;
}
int main() {
    auto r=compile(source,{{1000,1001}});
    assert(r.input==1 && r.values==Bytes({0,0,180,66,0,0,0,64}));
    assert(r.resources[2].id==2 && r.resources[3].id==3 && r.entries[0].id==4);
    const auto &program=r.blocks.at(4);
    // Identity, then buffer-to-buffer copy of angle into rotate's Y argument.
    Bytes prefix{23,0,160,2,0,33,0, 23,0,160,5,0,5,226,12,0,4,0,1,0,0,0, 23,0,160,5,0,1};
    assert(std::equal(prefix.begin(),prefix.end(),program.begin()));
    assert(r.blocks.at(5)==Bytes({23,0,160,2,0,33,2,0, 0,0,0,0, 0,0,0,0, 0,0,0,0}));
    // Explicit operand order and count-bounded signed16 XYZ transform.
    Bytes suffix{23,0,160,3,0,34,6,4,4,3,0,2,0,
                 23,0,160,233,3,41,8,192,3,0,232,3,8,0};
    assert(std::equal(suffix.rbegin(),suffix.rend(),program.rbegin()));
    for(unsigned id:{1000,1001}) assert(!r.owned.count(id) && !r.blocks.count(id));
    assert(r.cleanup.size()==r.owned.size()*6);
    for(size_t i=0;i<r.cleanup.size();i+=6) {
        unsigned id=r.cleanup[i+3]+256*r.cleanup[i+4];
        assert(r.owned.count(id) && r.cleanup[i+5]==2);
    }
    assert(r.invoke==Bytes({23,0,160,4,0,1}));
    assert(r.assembly.find("golem_transformFrame:")!=std::string::npos);
    assert(r.assembly.find("golem_angle_offset: equ 0")!=std::string::npos);
    assert(r.json.find("\"runtime_checks\":false")!=std::string::npos);
    auto relocated=compile(source,{{1,1001}});
    assert(relocated.input==1002 && relocated.resources[4].id==1000);
    // Unlike angles, literals live in immutable parts of each call template.
    assert(r.blocks.at(6)==Bytes({23,0,160,2,0,33,5,0, 0,0,0,0, 0,0,128,63, 0,0,128,191}));
    unsigned rejected=0;
    auto reject=[&](std::string text,std::vector<golem::resident::Reservation> reservations={{1000,1001}}) {
        try { compile(text,reservations); } catch(const std::exception&) { ++rejected; return; }
        throw std::runtime_error("unexpectedly accepted: "+text);
    };
    auto replace=[&](std::string from,std::string to) { auto s=source;auto at=s.find(from);assert(at!=s.npos);s.replace(at,from.size(),to);return s; };
    reject(source,{}); reject(source,{{1000,1001},{1001,1002}});
    reject(replace("4,4","3,4")); reject(replace("4,4","4,3"));
    reject(replace("float32 angle = 90","float32 angle"));
    reject(replace("90","1e40")); reject(replace("90","1e-40")); reject(replace("90","1e-100"));
    reject(replace("1000,8","0,8")); reject(replace("1000,8","65535,8"));
    reject(replace("1000,8","1000,0")); reject(replace("1000,8","1000,10923"));
    reject(replace("1001,8","1000,8")); reject(replace("1001,8","1001,7"));
    reject(replace("projected, screen, vertices","vertices, screen, vertices"));
    reject(replace("matrix_identity(model)","matrix_identity(angle)"));
    reject(replace("matrix_rotate(model, 0, angle, 0)","matrix_rotate(model, 0, angle+1, 0)"));
    reject(replace("matrix_rotate(model, 0, angle, 0)","matrix_rotate(model, 0, screen, 0)"));
    reject(replace("matrix_identity(model)","matrix_inverse(model)"));
    reject(source+" print 1"); reject(source+";"); reject(source+"Sub bad matrix_identity(model)");
    reject("Matrix(4,4) m = Identity Sub tick EndSub");
    reject("Matrix(4,4) m = Identity Matrix(4,4) M = Identity Sub tick matrix_identity(m) EndSub");
    reject("Matrix(4,4) m = Identity Sub package_bootstrap matrix_identity(m) EndSub");
    reject(replace("transformFrame",std::string(41,'a')));
    auto comments=compile("// semicolon ; in comment\n"+source,{{1000,1001}});
    assert(comments.bootstrap==r.bootstrap);
    auto mixed=compile("mAtRiX(4,4) m = iDeNtItY sUb tick MATRIX_IDENTITY(m) eNdSuB");
    assert(!mixed.input && mixed.invoke.size()==6);
    std::cout<<"Stock matrix encoding, ownership, relocation and "<<rejected<<" rejection cases pass.\n";
    std::cout<<"Stock perspective exact93-byte peer sequence, typed views/calls/bounds and "<<perspectiveChecks()<<" rejection cases pass.\n";
}
