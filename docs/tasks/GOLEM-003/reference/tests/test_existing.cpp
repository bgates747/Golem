// Freeze the existing language/bytecode contract before adding VDP intrinsics.
#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
int main() {
    assert(compile("print \"hello\"; print \"\\n\";") == std::vector<unsigned char>({'h','e','l','l','o','\n'}));
    assert(compile("PRINT \"ok\";") == compile("print \"ok\";"));
    // Command 0, buffer 2, two little-endian bytes. No accumulator needed.
    assert(compile("uint16 counter = 4660;") == std::vector<unsigned char>({23,0,160,2,0,0,2,0,0x34,0x12}));
    assert(compile("uint16 counter;") == compile("uint16 counter=0;"));
    assert(compile("uint16 a=10; uint16 b=5; uint16 c; c=a+b;").size()>30);
    auto loop=compile("uint16 i; For i=1 To 50; print \"Hello \"; Next i;");
    const std::vector<unsigned char> compare={23,0,160,4,0,6,0xa6,2,0,0,0,3,0,0,0};
    assert(std::equal(compare.begin(),compare.end(),loop.end()-compare.size()));
    for(const char *bad:{"print \"missing terminator\"", "uint16 a=65536;", "a=1;", "uint16 Print;",
                         "uint16 a; uint16 a;", "uint16 Name; name=2;", "For i=1 To 2; Next i;",
                         "uint16 i; For i=1 To 2; Next other;", "Next i;", "print str(1);"}) {
        bool failed=false;try{(void)compile(bad);}catch(const std::exception&){failed=true;}
        assert(failed);
    }
    puts("Existing language: print escapes/case, little-endian uint16, addition, loop buffer comparison, ten rejection cases pass.");
}
