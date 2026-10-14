#pragma once
#include "root.hpp"
#include <cctype>
#include <fstream>
#include <iterator>
#include <limits>

namespace native_bindings {
struct Json {
    enum Kind { Object,Array,String,Number } kind=Object;
    std::map<std::string,Json> object;
    std::vector<Json> array;
    std::string string;
    uint64_t number=0;
    const Json &at(const std::string&key)const {if(kind!=Object||!object.count(key))throw std::runtime_error("native bindings missing field: "+key);return object.at(key);}
    std::string text()const {if(kind!=String)throw std::runtime_error("native binding requires string");return string;}
    unsigned value()const {if(kind!=Number||number>UINT32_MAX)throw std::runtime_error("native binding requires uint32");return static_cast<unsigned>(number);}
};
class Parser {
    std::string input_;size_t pos_=0;
    void ws(){while(pos_<input_.size()&&std::isspace(static_cast<unsigned char>(input_[pos_])))++pos_;}
    char take(){if(pos_>=input_.size())throw std::runtime_error("truncated native binding JSON");return input_[pos_++];}
    std::string text(){if(take()!='"')throw std::runtime_error("native binding string expected");std::string out;for(;;){char c=take();if(c=='"')return out;if(static_cast<unsigned char>(c)<32)throw std::runtime_error("invalid JSON string");if(c=='\\'){c=take();if(c!='"'&&c!='\\'&&c!='/')throw std::runtime_error("unsupported binding string escape");}out+=c;}}
    Json parse(unsigned depth){if(depth>16)throw std::runtime_error("native binding JSON too deep");ws();Json j;char c=take();
        if(c=='{'||c=='['){j.kind=c=='{'?Json::Object:Json::Array;ws();const char end=c=='{'?'}':']';if(pos_<input_.size()&&input_[pos_]==end){++pos_;return j;}for(;;){ws();std::string key;if(c=='{'){key=text();ws();if(take()!=':')throw std::runtime_error("JSON colon expected");}Json v=parse(depth+1);if(c=='{'){if(!j.object.emplace(key,std::move(v)).second)throw std::runtime_error("duplicate binding key: "+key);}else j.array.push_back(std::move(v));ws();char sep=take();if(sep==end)return j;if(sep!=',')throw std::runtime_error("JSON comma expected");}}
        if(c=='"'){--pos_;j.kind=Json::String;j.string=text();return j;}
        if(c<'0'||c>'9'){throw std::runtime_error("unsupported binding JSON value");}
        j.kind=Json::Number;j.number=c-'0';if(c=='0'&&pos_<input_.size()&&std::isdigit(static_cast<unsigned char>(input_[pos_])))throw std::runtime_error("JSON leading zero");while(pos_<input_.size()&&std::isdigit(static_cast<unsigned char>(input_[pos_]))){if(j.number>UINT32_MAX/10ull)throw std::runtime_error("binding number overflow");j.number=j.number*10+input_[pos_++]-'0';}if(j.number>UINT32_MAX)throw std::runtime_error("binding number overflow");return j;
    }
public:explicit Parser(std::string s):input_(std::move(s)){}Json run(){Json j=parse(0);ws();if(pos_!=input_.size())throw std::runtime_error("trailing native binding JSON");return j;}
};
inline void exact(const Json&j,std::initializer_list<const char*> names){std::set<std::string> allowed;for(auto n:names)allowed.insert(n);if(j.kind!=Json::Object||j.object.size()!=allowed.size())throw std::runtime_error("unexpected/missing native binding fields");for(const auto&p:j.object)if(!allowed.count(p.first))throw std::runtime_error("unknown native binding field: "+p.first);}
struct Config {
    Json json;std::string raw;
    explicit Config(const std::string&path){std::ifstream f(path);if(!f)throw std::runtime_error("cannot read native binding config");raw=std::string(std::istreambuf_iterator<char>(f),{});if(raw.size()>16384)throw std::runtime_error("native binding config too large");json=Parser(raw).run();
        exact(json,{"schema","profile","phase","hooks","state","opponents","constants","bindings","completion","reserve"});
        if(json.at("schema").value()!=1||json.at("profile").text()!="rally-render-v1"||json.at("phase").text()!="after_source_before_success")throw std::runtime_error("unsupported native binding profile/phase");
        exact(json.at("state"),{"session","request_generation","simulation_tick","player_station","player_lateral_q8","phase","steering","flags"});
        exact(json.at("opponents"),{"station","lateral_q8","active","heading_turn16"});
        exact(json.at("constants"),{"presentation_profile","reference_crash_remaining","reference_invulnerable"});
        for(const auto&p:json.at("constants").object)if(p.second.value()!=0)throw std::runtime_error("only normal presentation constants are supported");
        exact(json.at("bindings"),{"asset","outputs","track_identity","asset_checksum","resources"});
        exact(json.at("bindings").at("resources"),{"player_bitmap_base","traffic_bitmap_base","sign_bitmap_base","traffic_matrix_base","player_mirror_matrix","sign_matrix"});
        range(json.at("bindings").at("asset"));const auto&o=json.at("bindings").at("outputs");if(o.kind!=Json::Array||o.array.size()!=3)throw std::runtime_error("three renderer outputs required");for(const auto&r:o.array)range(r);
        range(json.at("completion"));if(json.at("completion").at("bytes").value()!=48)throw std::runtime_error("renderer completion must be 48 bytes");
        if(json.at("reserve").kind!=Json::Array)throw std::runtime_error("reserve must be array");
        if(json.at("hooks").kind!=Json::Object||json.at("hooks").object.empty())throw std::runtime_error("native hooks required");
    }
    static void range(const Json&r){exact(r,{"buffer","offset","bytes"});unsigned id=r.at("buffer").value(),off=r.at("offset").value(),n=r.at("bytes").value();if(!id||id>=65535||!n||off>UINT32_MAX-n)throw std::runtime_error("invalid native binding range");}
    std::vector<portable::Reservation> reservations()const {std::set<unsigned> ids;auto add=[&](const Json&r){ids.insert(r.at("buffer").value());};add(json.at("bindings").at("asset"));for(auto&r:json.at("bindings").at("outputs").array)add(r);add(json.at("completion"));for(auto&r:json.at("reserve").array){exact(r,{"first","last"});unsigned a=r.at("first").value(),b=r.at("last").value();if(!a||b<a||b>=65535)throw std::runtime_error("invalid native reservation");for(unsigned id=a;id<=b;++id)ids.insert(id);}std::vector<portable::Reservation> out;for(auto id:ids)out.push_back({id,id});return out;}
};
inline void replaceAll(std::string&s,const std::string&a,const std::string&b){size_t at=0;while((at=s.find(a,at))!=std::string::npos){s.replace(at,a.size(),b);at+=b.size();}}
inline std::string emit(const portable::Program&p,const portable::Layout&l,const Config&c,std::string source){
    using namespace portable;
    auto global=[&](const std::string&group,const std::string&field,Type type){const auto name=c.json.at(group).at(field).text();for(size_t i=0;i<p.globals.size();++i)if(p.globals[i].name==name){if(p.globals[i].type!=type)throw std::runtime_error("native binding global type mismatch: "+name);return l.offsets[i];}throw std::runtime_error("unknown native binding global: "+name);};
    auto array=[&](const std::string&field,Type type){const auto name=c.json.at("opponents").at(field).text();for(size_t i=0;i<p.arrays.size();++i)if(p.arrays[i].name==name){const auto&a=p.arrays[i];if(a.type!=type||a.lower!=0||a.upper!=5)throw std::runtime_error("native binding requires six zero-based typed elements: "+name);return l.arrayBuffers[i];}throw std::runtime_error("unknown native binding array: "+name);};
    std::ostringstream code;
    code<<"\n#include \"helper/rally_render.c\"\nvoid*memcpy(void*d,const void*s,size_t n){uint8_t*a=d;const uint8_t*b=s;for(size_t i=0;i<n;++i)a[i]=b[i];return d;}\nvoid*memset(void*d,int v,size_t n){uint8_t*a=d;for(size_t i=0;i<n;++i)a[i]=(uint8_t)v;return d;}\n";
    code<<"static RallyRenderContextV1 renderer={.struct_bytes=sizeof(renderer)};\n";
    auto range=[&](const Json&r,unsigned access){std::ostringstream o;o<<'{'<<r.at("buffer").value()<<','<<access<<','<<r.at("offset").value()<<"u,"<<r.at("bytes").value()<<"u}";return o.str();};
    const auto&b=c.json.at("bindings");
    code<<"static RallyRenderBindingsV1 render_binding={76,1,0,"<<range(b.at("asset"),1)<<",{";
    for(unsigned i=0;i<3;++i){if(i)code<<',';code<<range(b.at("outputs").array[i],2);}code<<"},"<<b.at("track_identity").value()<<"u,"<<b.at("asset_checksum").value()<<"u,{";
    bool comma=false;for(auto n:{"player_bitmap_base","traffic_bitmap_base","sign_bitmap_base","traffic_matrix_base","player_mirror_matrix","sign_matrix"}){if(comma)code<<',';comma=true;auto value=b.at("resources").at(n).value();if(value>65535)throw std::runtime_error("graphics resource out of range");code<<value;}code<<"}};\n";
    code<<"static int32_t renderer_hook(const vdplet_host_v1*h,unsigned entry,uint8_t*state){\nunsigned op=0;switch(entry){\n";
    for(const auto&hook:c.json.at("hooks").object){unsigned entry=UINT32_MAX;for(size_t i=0;i<p.procedures.size();++i)if(p.procedures[i].name==hook.first)entry=i;if(entry==UINT32_MAX)throw std::runtime_error("unknown hook Sub: "+hook.first);std::string value=hook.second.text();unsigned op=value=="bind"?1:value=="prepare"?2:value=="stop"?3:0;if(!op)throw std::runtime_error("unknown renderer hook operation");code<<"case "<<entry<<":op="<<op<<";break;\n";}
    code<<"default:return 0;}\nRallyRenderCompletionV1 done={.struct_bytes=48,.schema=1};\nRallyRenderStateV1 s={.struct_bytes=108,.schema=1};\n";
    for(auto pair:{std::pair<const char*,Type>{"session",Type::U32},{"request_generation",Type::U32},{"simulation_tick",Type::U32},{"player_station",Type::U32},{"player_lateral_q8",Type::S32},{"phase",Type::U16},{"steering",Type::S16},{"flags",Type::U16}}){unsigned off=global("state",pair.first,pair.second);code<<"s."<<pair.first<<'=';if(pair.second==Type::S16)code<<"signed16(get(state+"<<off<<"));\n";else if(pair.second==Type::U16)code<<"get(state+"<<off<<");\n";else if(pair.second==Type::S32)code<<"signed32(get32(state+"<<off<<"));\n";else code<<"get32(state+"<<off<<");\n";}
    unsigned heading=global("opponents","heading_turn16",Type::S32);
    code<<"if(!s.session||s.session!=get32(state+8)||get32(state+"<<global("state","player_station",Type::U32)<<")>2147483647u)return VDPL_E_ARGUMENT;\nint32_t rc=0;\nif(op==1){render_binding.session=s.session;rc=rally_render_bind_v1(h,&renderer,&render_binding,&done);}\nelse if(op==3)rc=rally_render_stop_v1(&renderer,&done);\nelse {int32_t heading=signed32(get32(state+"<<heading<<"));if(heading< -1920||heading>1920)return VDPL_E_ARGUMENT;\n";
    const std::vector<std::pair<std::string,Type>> cols{{"station",Type::U32},{"lateral_q8",Type::S32},{"active",Type::U16}};
    for(const auto&col:cols){unsigned id=array(col.first,col.second),width=col.second==Type::U16?2:4;code<<"{vdplet_view_v1 v={sizeof(v),0,0,0,0};rc=h->buffer_acquire(h->context,"<<id<<",VDPL_READ,0,"<<width*6<<",&v);if(rc)return rc;for(unsigned i=0;i<6;++i)s.opponents[i]."<<col.first<<'='<<(width==2?"get":"get32")<<"(v.data+i*"<<width<<");rc=h->buffer_release(h->context,v.lease);if(rc)return rc;}\n";}
    code<<"for(unsigned i=0;i<6;++i){if(s.opponents[i].active)s.opponents[i].heading_turn16=(int16_t)heading;else s.opponents[i]=(RallyRenderOpponentV1){0,0,0,0};}rc=rally_render_prepare_v1(h,&renderer,&s,&done);}\nif(done.reason==RALLY_RENDER_REASON_RELEASE)return rc;\n";
    const auto&completion=c.json.at("completion");
    code<<"vdplet_view_v1 out={sizeof(out),0,0,0,0};int32_t pr=h->buffer_acquire(h->context,"<<completion.at("buffer").value()<<",VDPL_WRITE,"<<completion.at("offset").value()<<",48,&out);if(pr)return pr;memcpy(out.data,&done,48);pr=h->buffer_release(h->context,out.lease);return pr?pr:rc;}\n";
    const auto header=source.find("__attribute__((visibility(\"default\")))");if(header==std::string::npos)throw std::logic_error("native entry anchor");source.insert(header,code.str());
    const std::string old="uint16_t id=get(args.data);rc=h->buffer_acquire";
    if(source.find(old)==std::string::npos)throw std::logic_error("native argument anchor");
    replaceAll(source,old,"uint16_t id=get(args.data);rc=h->buffer_release(h->context,args.lease);if(rc)return rc;args.lease=0;rc=h->buffer_acquire");
    replaceAll(source,"h->buffer_release(h->context,args.lease);","");
    // The early release above must remain: replaceAll also removed its text.
    replaceAll(source,"rc=if(rc)return rc;args.lease=0;","rc=h->buffer_release(h->context,args.lease);if(rc)return rc;args.lease=0;");
    const std::string success="put32(state+20,get32(state+20)+1u);";
    if(source.find(success)==std::string::npos)throw std::logic_error("native success anchor");
    replaceAll(source,success,"rc=renderer_hook(h,r->entry_id,state);if(rc){int32_t rr=h->buffer_release(h->context,view.lease);return rr?rr:rc;}"+success);
    replaceAll(source,"h->buffer_release(h->context,view.lease);return 0;","return h->buffer_release(h->context,view.lease);");
    return source;
}
}
