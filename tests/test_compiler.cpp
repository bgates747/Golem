#include "../src/assembly_output.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <set>

namespace {
unsigned rejections = 0;
template<class F> void reject(F action, const std::string &message) {
    try { action(); }
    catch (const std::exception &error) {
        if (std::string(error.what()).find(message) == std::string::npos) {
            std::cerr << "Expected " << message << "; got " << error.what() << '\n';
            assert(false);
        }
        ++rejections;
        return;
    }
    std::cerr << "Unexpected acceptance; expected " << message << '\n';
    assert(false && "unexpected acceptance");
}
const std::string floorSource =
    "float32 x Range(-32768,32767)=-0.00006103515625 sint16 result "
    "Sub compute result=sint16(floor(x)) EndSub";
}

int main() {
    using namespace golem;
    assert(ordinary::compile("print \"hello\"") == std::vector<unsigned char>({'h','e','l','l','o'}));
    assert(ordinary::compile("uint16 x") == ordinary::compile("UINT16 x=0"));
    reject([] { ordinary::compile("uint16 x=1 x=X"); }, "undeclared variable");
    reject([] { ordinary::compile("uint16 x;"); }, "remove ';'");
    reject([] { ordinary::compile("For i=1 To 2 Next i"); }, "undeclared variable");
    reject([] { ordinary::compile("uint16 i For i=1 To 2 Next j"); }, "does not match");

    const auto pure = resident::compileResident(floorSource);
    assert(pure.bootstrap.size() == 1008 && pure.owned.size() == 25);
    assert(pure.cleanup.size() == pure.owned.size() * 6 && pure.invoke.size() == 6);
    assert(resident_scalar::inputUpdate(pure, -0.75).size() + pure.invoke.size() == 21);
    const std::string mixed = "uint16 calls=0 uint16 previous=0 "
        "float32 x Range(-32768,32767)=-0.00006103515625 sint16 result "
        "Sub compute previous=calls calls=calls+1 result=sint16(floor(x)) print \"done\\n\" EndSub";
    const auto stateful = resident::compileResident(mixed);
    assert(stateful.extras.errorStatus != 0 && stateful.extras.programs.size() == 1);
    assert(stateful.owned.at(stateful.extras.errorStatus) == 2);
    assert(stateful.cleanup.size() == stateful.owned.size() * 6);
    assert(resident_scalar::inputUpdate(stateful, -0.75).size() + stateful.invoke.size() == 21);
    const std::vector<resident::Reservation> reserved{{1,254},{260,264},{50000,50000}};
    const auto relocated = resident::compileResident(mixed, reserved);
    for (const auto &entry : relocated.owned) {
        assert(entry.first != 0 && entry.first != 65535);
        for (const auto &range : reserved)
            assert(entry.first < range.first || entry.first > range.last);
    }
    assert(relocated.layout.input >= 255 && relocated.extras.globals.size() == 2);
    const auto metadata = resident::manifest(relocated, reserved);
    assert(metadata.find("\"profile\": \"resident-scalar\"") != std::string::npos);
    assert(metadata.find("experimental_task") == std::string::npos);
    assert(metadata.find("\"serialized_nonreentrant\": true") != std::string::npos);
    assert(metadata.find("\"external_input_requires_host_validation\": true") != std::string::npos);
    assert(metadata.find("\"uint16_add_overflow\": 1") != std::string::npos);
    assert(metadata.find("\"reset\": \"each_invocation\"") != std::string::npos);
    assert(pure.extras.errorStatus == 0);
    const auto include = assembly::render(relocated, resident_scalar::inputUpdate(relocated, -0.75));
    assert(include.find("golem_compute:\n    ld hl,golem_compute_call") != std::string::npos);
    assert(include.find("golem_x_buffer: equ " + std::to_string(relocated.layout.input)) != std::string::npos);
    assert(include.find("golem_runtime_error_buffer:") != std::string::npos);
    reject([&] { assembly::render(resident::compileResident("uint16 X=0 " + floorSource), {}); }, "assembly symbol collision");
    reject([&] { assembly::render(resident::compileResident("uint16 runtime_error=0 " + mixed), {}); }, "assembly symbol collision");
    reject([&] { assembly::render(resident::compileResident("uint16 " + std::string(65, 'a') + "=0 " + floorSource), {}); }, "64-character limit");
    assert(resident::manifest(pure).find("runtime_error") == std::string::npos);
    assert(resident::compileResident(mixed).bootstrap == stateful.bootstrap);
    for (double value : {-32768.0, -32767.75, -1.5, -1.0, -0.00006103515625,
                         -static_cast<double>(std::numeric_limits<float>::denorm_min()),
                         -0.0, 0.0, static_cast<double>(std::numeric_limits<float>::denorm_min()),
                         0.75, 32766.75, 32767.0})
        assert(resident_scalar::inputUpdate(pure, value).size() == 15);
    for (double value : {-32768.01, 32767.01})
        reject([&] { resident_scalar::inputUpdate(pure, value); }, "declared Range");
    for (double value : {std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()})
        reject([&] { resident_scalar::inputUpdate(pure, value); }, "finite representable float32");

    auto replace = [&](const std::string &from, const std::string &to) {
        auto source = floorSource;
        source.replace(source.find(from), from.size(), to);
        return source;
    };
    reject([&] { resident::compileResident("uint16 x " + floorSource); }, "duplicate declaration");
    reject([&] { resident::compileResident("uint16 a=65535+1 " + floorSource); }, "constant addition overflows");
    reject([&] { resident::compileResident("uint16 a=1 uint16 b=a+1 " + floorSource); }, "initializer addition requires constant");
    reject([&] { resident::compileResident("uint16 a=0 " + replace("result=", "a=65535+1 result=")); }, "constant addition overflows");
    assert(resident::compileResident("uint16 a=32768+32767 " + floorSource).extras.errorStatus == 0);
    assert(resident::compileResident("uint16 a=65535-65535+1 " + floorSource).extras.errorStatus == 0);
    reject([&] { resident::compileResident("uint16 a=0-1 " + floorSource); }, "constant subtraction underflows");
    reject([&] { resident::compileResident("uint16 a=0-1+1 " + floorSource); }, "constant subtraction underflows");
    reject([&] { resident::compileResident("uint16 a=1 uint16 b=a-1 " + floorSource); }, "initializer subtraction requires constant");
    const auto subtract = resident::compileResident("uint16 a=1 " + replace("result=", "a=a-1 result="));
    assert(subtract.extras.errorStatus && subtract.extras.programs.size() == 2);
    assert(resident::manifest(subtract).find("\"uint16_sub_underflow\": 2") != std::string::npos);
    reject([&] { resident::compileResident("uint16 a=256*256 " + floorSource); }, "constant multiplication overflows");
    reject([&] { resident::compileResident("uint16 a=65535*2*0 " + floorSource); }, "constant multiplication overflows");
    reject([&] { resident::compileResident("uint16 a=1 uint16 b=a*1 " + floorSource); }, "initializer multiplication requires constant");
    reject([] { ordinary::compile("uint16 a=2*3"); }, "multiplication requires --resident");
    assert(resident::compileResident("uint16 a=1+2*3-4 " + floorSource).extras.errorStatus == 0);
    assert(resident::compileResident("uint16 a=1+2*3-4 " + floorSource).bootstrap ==
           resident::compileResident("uint16 a=3+0 " + floorSource).bootstrap);
    assert(resident::compileResident("uint16 a=0*65535*65535 " + floorSource).extras.errorStatus == 0);
    const auto multiply = resident::compileResident("uint16 a=2 " + replace("result=", "a=a*a result="));
    assert(resident::manifest(multiply).find("\"uint16_mul_overflow\": 3") != std::string::npos);
    const std::string pair = "uint16 q=0 uint16 r=0 " +
        replace("result=", "(q,r)=DiVmOd(65535,1) result=");
    assert(resident::manifest(resident::compileResident(pair)).find("\"uint16_divide_by_zero\": 4") != std::string::npos);
    reject([&] { resident::compileResident("uint16 q=0 " + replace("result=", "(q,q)=divmod(17,5) result=")); }, "distinct uint16 globals");
    reject([&] { resident::compileResident("uint16 q=0 uint16 r=0 " + replace("result=", "(q,r)=divmod(17) result=")); }, "expected ','");
    reject([&] { resident::compileResident("uint16 q=0 uint16 r=0 " + replace("result=", "(q,r)=divmod(65536,1) result=")); }, "does not fit in a uint16");
    reject([&] { resident::compileResident("uint16 q=0 uint16 r=0 " + replace("result=", "(q,r)=divmod(1,2,3) result=")); }, "expected ')'");
    reject([&] { resident::compileResident("uint16 q=0 " + replace("result=", "(q,result)=divmod(1,2) result=")); }, "undeclared variable");
    reject([] { ordinary::compile("uint16 q uint16 r (q,r)=divmod(17,5)"); }, "requires a resident job");
    reject([&] { resident::compileResident("uint16 divmod=0 " + floorSource); }, "reserved identifier");
    reject([&] { resident::compileResident(replace("floor(x)", "floor(X)")); }, "declared float32 input");
    reject([&] { resident::compileResident(replace("floor(x)", "floor(x+1)")); }, "expected ')'");
    reject([&] { resident::compileResident(replace("Range(-32768,32767)", "Range(-1,1,1)")); }, "expected ')'");
    reject([&] { resident::compileResident(replace("Range(-32768,32767)", "Range(-32769,32767)")); }, "finite input Range");
    reject([&] { resident::compileResident(replace("-0.00006103515625", "32768")); }, "declared Range");
    reject([&] { resident::compileResident(replace("result=sint16(floor(x))", "print \"empty\"")); }, "requires one");
    reject([&] { resident::compileResident(replace("EndSub", "result=sint16(floor(x)) EndSub")); }, "exactly one");
    reject([&] { resident::compileResident(replace("EndSub", "")); }, "expected 'EndSub'");
    reject([&] { resident::compileResident(replace("result=", "For i=1 To 2 Next i result=")); }, "not qualified");
    reject([&] { resident::compileResident(replace("result=", "uint16 local result=")); }, "declare resident uint16 globals");
    reject([&] { resident::compileResident(floorSource + " print \"extra\""); }, "trailing source");
    reject([&] { resident::compileResident(floorSource, {{1,65534}}); }, "namespace exhausted");
    reject([&] { resident::compileResident(floorSource, {{5,10},{8,20}}); }, "already claimed");
    reject([&] { resident::compileResident(floorSource, {{0,1}}); }, "reserved");
    reject([&] { resident::compileResident(floorSource, {{65535,65535}}); }, "reserved");
    reject([] { resident::updateValue("1 garbage"); }, "unexpected text");
    reject([] { resident::updateValue("1e9999"); }, "representable range");
    const std::string tableSource =
        "Record Sample uint16 tag sint16 delta EndRecord Table Sample samples(0:3) "
        "Sample selected=Sample(65535,-32768) uint16 index=0 "
        "Sub fetch selected=samples(index) print \"selected\" EndSub";
    const resident_scalar::Bytes tableData{1,0,254,255,2,0,3,0,3,0,252,255,4,0,5,0};
    const auto table = resident_table::compile(tableSource,"samples",tableData);
    assert(table.spec.rowBytes()==4 && table.spec.members[1].offset==2);
    assert(table.owned.size()==19 && table.base==1 && table.initial.at(3)[2]==252);
    assert(table.initial.at(table.output)==resident_scalar::Bytes({255,255,0,128}));
    assert(resident_table::inputUpdate(table,65535).size()+table.invoke.size()==19);
    assert(resident_table::manifest(table).find("table_index_out_of_bounds")!=std::string::npos);
    const auto tableInclude = assembly::render(table,{});
    assert(tableInclude.find("golem_selected_delta_offset: equ 2")!=std::string::npos);
    assert(tableInclude.find("golem_fetch:\n    ld hl,golem_fetch_call")!=std::string::npos);
    const auto tableRelocated = resident_table::compile(tableSource,"samples",tableData,reserved);
    assert(tableRelocated.base==255);
    for (const auto &v : tableRelocated.owned)
        for (const auto &range : reserved) assert(v.first<range.first || v.first>range.last);
    BufferAllocator runs(1,12); runs.reserve(4,5,"asset");
    assert(runs.allocateRun(4,"rows")==6); // Skip an insufficient hole atomically.
    const auto claims = runs.claims().size();
    reject([&] { runs.allocateRun(4,"more"); },"no consecutive");
    assert(runs.claims().size()==claims && runs.allocate("next")==10);
    reject([&] { runs.allocateRun(0,"empty"); },"invalid consecutive");
    auto alteredTable = [&](const std::string &from, const std::string &to) {
        auto text=tableSource; text.replace(text.find(from),from.size(),to); return text;
    };
    reject([&] { resident_table::compile(tableSource,"other",tableData); },"name must match");
    reject([&] { resident_table::compile(tableSource,"samples",{}); },"data size");
    reject([&] { resident_table::compile(tableSource,"samples",tableData,{{1,65520}}); },"namespace exhausted");
    for (const auto &change : std::vector<std::pair<std::string,std::string>>{
            {"0:3","1:3"},{"0:3","0:1024"},{"uint16 tag","float32 tag"},
            {"-32768","-32769"},{"65535,","-1,"},{"index=0","index=65536"},
            {"samples(index)","samples(Index)"},{"samples(index)","samples(index+1)"},
            {"samples(index)","samples(1)"},{"selected=samples(index)","samples(index)=selected"},
            {"uint16 tag sint16 delta","uint16 tag sint16 tag"},
            {"EndSub","selected=samples(index) EndSub"},{"selected=samples(index)",""},
            {"Sub fetch","Sub selected"},{"EndSub","EndSub print \"trailing\""}}) {
        reject([&] { resident_table::compile(alteredTable(change.first,change.second),"samples",tableData); },"");
    }
    reject([&] { assembly::render(resident_table::compile(alteredTable("Sub fetch","Sub package_bootstrap"),"samples",tableData),{}); },"symbol collision");
    const auto keywordCase = alteredTable("Record Sample","rEcOrD Sample");
    assert(resident_table::compile(keywordCase,"samples",tableData).bootstrap==table.bootstrap);
    const std::string drawingSource = "uint8 colour Bounds(0,63)=11 sint16 x=-32768 sint16 y=32767 "
        "Sub paint draw_point(colour,x,y) draw_line(2,x,y,0,0) fill_quad(colour,0,0,20,0,30,20,-10,20) EndSub";
    const auto drawing = resident_drawing::compile(drawingSource, {{1,254},{50000,50000}});
    assert(drawing.input==255 && drawing.entry==256 && drawing.spec.values==resident_scalar::Bytes({11,0,128,255,127}));
    assert(drawing.spec.globals[1].offset==1 && drawing.spec.globals[2].offset==3);
    assert(drawing.invoke.size()==6 && resident_drawing::inputUpdate(drawing).size()==16);
    assert(drawing.owned.size()==5 && !drawing.owned.count(0) && !drawing.owned.count(50000));
    const auto drawAsm = assembly::render(drawing,{});
    assert(drawAsm.find("golem_colour_type: equ 5")!=std::string::npos);
    assert(drawAsm.find("golem_x_offset: equ 1")!=std::string::npos);
    assert(drawAsm.find("golem_paint:\n")!=std::string::npos);
    const auto literalDrawing = resident_drawing::compile("Sub paint fill_quad(3,0,0,20,0,30,20,-10,20) EndSub");
    assert(!literalDrawing.input && resident_drawing::inputUpdate(literalDrawing).empty());
    assert(literalDrawing.programs.at(2)==resident_scalar::Bytes({18,0,3,25,4,0,0,0,0,25,4,20,0,0,0,
        25,85,246,255,20,0,25,85,30,0,20,0}));
    auto drawChange = [&](const std::string &from, const std::string &to) {
        auto s=drawingSource; s.replace(s.find(from),from.size(),to); return s;
    };
    assert(resident_drawing::compile(drawChange("draw_point","DrAw_PoInT"),{{1,254},{50000,50000}}).bootstrap==drawing.bootstrap);
    for (const auto &change : std::vector<std::pair<std::string,std::string>>{
            {"Bounds(0,63)","Bounds(0,255)"},{"Bounds(0,63)","Bounds(40,30)"},
            {"=11","=64"},{"=11","=1.5"},{"x=-32768","x=-32769"},{"y=32767","y=32768"},
            {"x=-32768","x"},{"colour,x,y","colour,X,y"},{"colour,x,y","x,x,y"},
            {"colour,x,y","colour,colour,y"},{"colour,x,y","64,x,y"},
            {"colour,x,y","colour,x+1,y"},{"colour,x,y","colour,x"},
            {"draw_point(colour,x,y)","draw_bitmap(colour,x,y)"},
            {"draw_point(colour,x,y)","x=1"},{"EndSub",""},{"EndSub","EndSub Sub two EndSub"},
            {"Sub paint","Sub x"},{"Sub paint","Sub draw_point"},
            {"=11","=11;"},{"sint16 y=32767","sint16 x=32767"}})
        reject([&] {resident_drawing::compile(drawChange(change.first,change.second));}, "");
    reject([] {resident_drawing::compile("Sub empty EndSub");},"requires an operation");
    reject([&] {resident_drawing::compile(drawingSource,{{1,65531}});},"namespace exhausted");
    reject([&] {assembly::render(resident_drawing::compile(drawChange("Sub paint","Sub package_bootstrap")),{});},"symbol collision");
    reject([&] {assembly::render(resident_drawing::compile(drawChange("Sub paint","sint16 X=0 Sub paint")),{});},"symbol collision");
    const std::string bitmapSource = "uint16 image Bounds(1,65534)=50000 sint16 x=-32768 sint16 y=32767 "
        "Sub paint draw_bitmap(image,x,y) draw_point(2,0,0) EndSub";
    assert(resident_drawing::profile(bitmapSource));
    assert(!resident_drawing::profile("uint16 a=1 // Sub fake draw_bitmap\n" + floorSource));
    assert(!resident_drawing::profile("uint16 a=1 " + floorSource));
    const auto bitmap = resident_drawing::compile(bitmapSource,{{1,254},{50000,50000}});
    assert(bitmap.input==255 && bitmap.entry==256 && bitmap.spec.values==resident_scalar::Bytes({80,195,0,128,255,127}));
    assert(bitmap.spec.globals[0].width()==2 && bitmap.spec.globals[0].typeTag()==1);
    assert(bitmap.programs.at(257)==resident_scalar::Bytes({23,27,32,0,0,23,0,150,1,255,255,
        23,27,3,0,0,0,0,23,0,150,1,255,255}));
    // Three independent word patches at the full-ID selector and signed positions.
    assert(bitmap.programs.at(256)==resident_scalar::Bytes({
        23,0,160,1,1,5,226,3,0,2,0,255,0,0,0,
        23,0,160,1,1,5,226,14,0,2,0,255,0,2,0,
        23,0,160,1,1,5,226,16,0,2,0,255,0,4,0,
        23,0,160,1,1,1,23,0,160,2,1,1}));
    assert(bitmap.initialBitmapIDs==std::set<unsigned>({50000}));
    assert(!bitmap.owned.count(50000) && bitmap.invoke.size()==6);
    assert(resident_drawing::inputUpdate(bitmap).size()+bitmap.invoke.size()==23);
    assert(resident_scalar::Bytes(bitmap.bootstrap.begin(),bitmap.bootstrap.begin()+7)==resident_scalar::Bytes({23,0,248,1,0,1,0}));
    assert(resident_drawing::manifest(bitmap,{{50000,50000}}).find("\"initial_bitmap_ids\": [50000]")!=std::string::npos);
    assert(assembly::render(bitmap,{}).find("golem_image_type: equ 1")!=std::string::npos);
    const auto literalBitmap = resident_drawing::compile(
        "Sub paint draw_bitmap(50000,-32768,32767) draw_bitmap(65534,-1,0) EndSub",{{50000,50000},{65534,65534}});
    assert(!literalBitmap.input && literalBitmap.owned.size()==3);
    assert(literalBitmap.programs.at(2)==resident_scalar::Bytes({23,27,32,80,195,23,0,150,1,255,255,
        23,27,3,0,128,255,127,23,0,150,1,255,255}));
    assert(literalBitmap.programs.at(3)[3]==254 && literalBitmap.programs.at(3)[4]==255);
    assert(literalBitmap.programs.at(3)[14]==255 && literalBitmap.programs.at(3)[15]==255);
    auto bitmapChange = [&](const std::string &from, const std::string &to) {
        auto s=bitmapSource; s.replace(s.find(from),from.size(),to); return s;
    };
    assert(resident_drawing::compile(bitmapChange("draw_bitmap","DrAw_BiTmAp"),{{1,254},{50000,50000}}).bootstrap==bitmap.bootstrap);
    reject([&] {resident_drawing::compile(bitmapSource);},"requires --reserve");
    reject([] {resident_drawing::compile("Sub paint draw_bitmap(50000,0,0) EndSub");},"requires --reserve");
    reject([&] {resident_drawing::compile(bitmapSource,{{49999,49999}});},"requires --reserve");
    for (const auto &change : std::vector<std::pair<std::string,std::string>>{
            {"uint16 image","sint16 image"},{"uint16 image","uint8 image"},
            {"Bounds(1,65534)","Bounds(0,65534)"},{"Bounds(1,65534)","Bounds(1,65535)"},
            {"Bounds(1,65534)",""},{"=50000","=65535"},{"=50000","=-1"},
            {"image,x,y","0,x,y"},{"image,x,y","65535,x,y"},{"image,x,y","1.5,x,y"},
            {"image,x,y","image,image,y"},{"image,x,y","image,x,image"},
            {"image,x,y","Image,x,y"},{"image,x,y","image,x+1,y"},
            {"image,x,y","image,x,y,0"},{"image,x,y","image,x"},
            {"Sub paint","Sub draw_bitmap"},{"image,x,y","image,-32769,y"},
            {"image,x,y","image,x,32768"}})
        reject([&] {resident_drawing::compile(bitmapChange(change.first,change.second),{{50000,50000}});},"");
    std::cout << "Maintained raw/resident contract checks pass; " << rejections << " rejection cases.\n";
}
