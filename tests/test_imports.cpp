#define main golemc_cli_main
#include "../src/golemc.cpp"
#undef main
#include <cassert>
namespace fs = std::filesystem;
int main() {
    fs::path root = fs::path("build/import-fixtures");
    fs::create_directories(root);
    writeFile((root / "records.dat").string(),
              {7, 0, 0, 0, 9, 0, 0, 0, 252, 255, 255, 255, 6, 0, 0, 0});
    writeFile((root / "tag.dat").string(), {0x55, 0xaa});
    const std::string declarations =
        "Table samples(4000,8,2,\"records.dat\"); Buffer record(1500,8); Field "
        "x(record,0,s32,-10,10); Field y(record,4,s32,1,10); Buffer state(1000,4); Field "
        "index(state,0,u16); Field ok(state,2,u16,0,1); ";
    auto result = hosted::compile(
        declarations + "Program read(2000) { LoadIndexed(record,samples,index,ok); };", root);
    assert(result.map.find("records.dat 16 fnv1a64") != std::string::npos);
    assert(result.map.find("source_id_offset 11 guarded_body") != std::string::npos);
    const std::vector<unsigned char> compare = {23, 0, 160, 5, 0, 6, 0xa4, 232,
                                                3,  0, 0,   3, 0, 0, 0};
    assert(std::search(result.code.begin(), result.code.end(), compare.begin(), compare.end()) !=
           result.code.end());
    assert(result.clear.size() % 6 == 0);
    std::set<unsigned> cleared;
    for (size_t i = 0; i < result.clear.size(); i += 6) {
        assert(result.clear[i] == 23 && result.clear[i + 1] == 0 && result.clear[i + 2] == 160 &&
               result.clear[i + 5] == 2);
        assert(cleared.insert(result.clear[i + 3] + 256 * result.clear[i + 4]).second);
    }
    assert(cleared.count(4000) && cleared.count(4001) && cleared.count(2000) && !cleared.count(0) &&
           !cleared.count(55555));
    auto asset =
        hosted::compile("Asset tag(4100,\"tag.dat\",2); Field magic(tag,0,u16,43605,43605);", root);
    assert(asset.map.find("tag.dat 2 fnv1a64") != std::string::npos);
    const std::string element =
        "Asset tag(4100,\"tag.dat\",2); Field a(tag,0,u8,85,85); Field b(tag,1,u8,170,170); Buffer "
        "selected(1500,1); Field value(selected,0,u8); Buffer indexBuffer(1000,2); Field "
        "index(indexBuffer,0,u16); Buffer statusBuffer(1002,2); Field ok(statusBuffer,0,u16,0,1); ";
    auto selected = hosted::compile(element + "LoadElement(selected,tag,index,ok);", root);
    assert(selected.map.find("source 4100 count 2 stride 1") != std::string::npos);
    auto reject = [&](const std::string &s) {
        bool failed = false;
        try {
            (void)hosted::compile(s, root);
        } catch (const CompileError &) {
            failed = true;
        }
        assert(failed);
    };
    reject("Asset tag(4100,\"tag.dat\",3);");
    reject("Asset tag(4100,\"missing\",2);");
    reject("Asset tag(4100,\"/etc/passwd\",2);");
    reject("Asset tag(4100,\"../tag.dat\",2);");
    reject("Asset tag(4100,\"tag.dat\",2); Field magic(tag,0,u16,0,100);");
    reject("Asset tag(4100,\"tag.dat\",2); Field magic(tag,0,u16); Set(magic,1);");
    reject("Table samples(4000,8,2,\"records.dat\"); Buffer collision(4001,2);");
    reject("Buffer collision(4001,2); Table samples(4000,8,2,\"records.dat\");");
    reject("Table samples(4999,8,2,\"records.dat\");");
    reject("Table samples(4000,8,0,\"records.dat\");");
    reject(declarations + "Program p(2000) { LoadIndexed(record,samples,index); };");
    reject(declarations + "Program p(2000) { LoadIndexed(record,samples,index,index); };");
    reject("Table samples(4000,8,2,\"records.dat\"); Buffer record(1500,8); Field "
           "x(record,0,s32,0,10); Buffer state(1000,2); Field index(state,0,u16,0,1); Program "
           "p(2000) { LoadIndexed(record,samples,index); };");
    writeFile((root / "large.dat").string(), std::vector<unsigned char>(65535, 0));
    std::string assets;
    for (unsigned i = 0; i < 9; ++i)
        assets += "Asset a" + std::to_string(i) + "(" + std::to_string(5000 + i) +
                  ",\"large.dat\",65535); ";
    reject(assets);
    puts("Resident imports: exact sizes, source-relative assets, typed records, ID collisions, "
         "guarded patch lowering and 14 negatives pass.");
}
