#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
int main(){
    const std::string base=
        "Buffer b(1000,12); Field left(b,0,s16,0,319); Field top(b,2,s16,0,239); "
        "Field right(b,4,s16,0,319); Field bottom(b,6,s16,0,239); "
        "Field direction(b,8,u16,0,3); Field amount(b,10,u16,0,255); ";
    auto result=hosted::compile(base+"Program p(2000) { GraphicsViewport(left,top,right,bottom); ScrollGraphics(direction,amount); };");
    for(const hosted::Bytes &bytes : {hosted::Bytes{24,0,0,0,0,0,0,0,0,25,4,0,0,0,0},hosted::Bytes{23,7,2,0,0}})
        assert(std::search(result.code.begin(),result.code.end(),bytes.begin(),bytes.end())!=result.code.end());
    assert(result.map.find("wire_0_3_2_1")!=std::string::npos);
    assert(result.map.find("extent2 direction0_right_1_left_2_down_3_up")!=std::string::npos);
    // Native firmware owns invalid-order/outside-current-mode semantics.
    (void)hosted::compile(base+"GraphicsViewport(right,bottom,left,top);");
    (void)hosted::compile(base+"GraphicsViewport(left,top,left,top);");
    (void)hosted::compile("Buffer b(1000,2); Field d(b,0,u8,0,3); Field n(b,1,u8); ScrollGraphics(d,n);");
    auto reject=[](const std::string&s){bool failed=false;try{(void)hosted::compile(s);}catch(const CompileError&){failed=true;}assert(failed);};
    reject(base+"GraphicsViewport(left,top,right);");
    reject(base+"GraphicsViewport(left,top,right,amount);");
    reject(base+"Field negative(b,0,s16); GraphicsViewport(negative,top,right,bottom);");
    reject(base+"GraphicsViewport(0,top,right,bottom);");
    reject(base+"ScrollGraphics(direction);");
    reject(base+"ScrollGraphics(top,amount);");
    reject(base+"ScrollGraphics(amount,amount);");
    reject(base+"Field wide(b,10,u16); ScrollGraphics(direction,wide);");
    reject(base+"ScrollGraphics(1,amount);");
    puts("Viewport pixel/wire order, finite scroll extent and typed rejection checks pass.");
}
