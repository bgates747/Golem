#include "../src/load_plan.hpp"
#include <cassert>
#include <iostream>
using namespace golem::loading;
Plan example() {
    Plan p;p.profile="test";p.common={{10,{1,2,3}}};p.commonOwned={10};p.reservations={{1,9},{20,29}};
    Component b;b.name="buffered";b.records={{11,{42}}};b.resources={11};b.entries={11};b.setup={23,0,248,1,0,1,0};
    Component n;n.name="native";n.records={{12,{99}}};n.resources={12};n.entries={12};
    p.components={b,n};return p;
}
int main() {
    auto p=example();p.finalize();auto &b=p.components[0];auto &n=p.components[1];
    assert(b.setupEntry==13 && b.cleanupEntry==14 && n.moduleBuffer==15 && n.cleanupEntry==16);
    assert(b.records.at(11)==Bytes({42}) && n.records.at(12)==Bytes({99}));
    assert(b.records.at(14)==Bytes({23,0,160,10,0,2,23,0,160,11,0,2,23,0,160,13,0,2}));
    assert(n.records.at(16)==Bytes({23,0,160,10,0,2,23,0,160,12,0,2,23,0,160,15,0,2}));
    unsigned rejected=0;
    auto reject=[&](Plan q){try{q.finalize();}catch(const std::exception&){++rejected;return;}throw std::runtime_error("invalid load plan accepted");};
    auto q=example();q.commonOwned.push_back(10);reject(q);
    q=example();q.components[1].resources.push_back(11);reject(q);
    q=example();q.components[0].records[19]={1};reject(q);
    q=example();q.common[11]={1};reject(q);
    q=example();q.components[0].entries={19};reject(q);
    q=example();q.components[0].records[11]={};reject(q);
    q=example();q.reservations.push_back({10,10});reject(q);
    q=example();q.components[1].name="buffered";reject(q);
    q=example();q.components[0].name="other";reject(q);
    q=example();q.symbols="golem_AGNB_buffered_load:\n ret\n";reject(q);
    q=example();q.reservations={{1,9},{13,65534}};reject(q);
    q=example();q.components.clear();reject(q);
    std::cout<<"Load-plan allocation, lifecycle and "<<rejected<<" rejection cases pass.\n";
}
