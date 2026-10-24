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
}
