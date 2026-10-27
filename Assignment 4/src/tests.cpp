#include "candidate_search.hpp"
#include <iostream>
#include <stdexcept>
#include <random>
#include <set>
#include <tuple>

void verify(bool condition,const char *msg){if(!condition)throw std::runtime_error(msg);}
void test_one(const Problem&p,const Tour&t,int k) {
 auto near=nearest_candidates(p,k);
 auto outside=outside_of(p,t);
 const auto moves=candidate_moves(p,t,outside,near);
 std::set<std::tuple<int,int,int>> unique;
 const auto orig=evaluate(p,t);
 std::vector<bool> selected(p.size());for(int v:t)selected[v]=true;
 for(const auto &mv:moves) {
  verify(unique.emplace(int(mv.kind),mv.i,mv.j).second,"Duplicate candidate move");
  Tour tt=t;
  const auto delta=mv.kind==MoveKind::TwoOpt?opt_delta(p,t,mv.i,mv.j):replace_delta(p,t,mv.i,outside[mv.j]);
  apply(tt,mv,outside);
  verify(evaluate(p,tt)-orig==delta,"Wrong delta formula");
  bool valid=false;
  for(int u:t)for(int v:near[u]) {
   if(mv.kind==MoveKind::TwoOpt && selected[v]) {
    const int a=t[mv.i],b=t[(mv.i+1)%t.size()],c=t[mv.j],d=t[(mv.j+1)%t.size()];
    if((u==a&&v==c)||(u==c&&v==a)||(u==b&&v==d)||(u==d&&v==b))valid=true;
   }
   if(mv.kind==MoveKind::Replace && v==outside[mv.j]) {
    const int a=t[(mv.i+t.size()-1)%t.size()],b=t[(mv.i+1)%t.size()];
    if(u==a||u==b)valid=true;
   }
  }
  verify(valid,"Move does not introduce any candidate edge");
 }
 if(k==p.size()-1) {
  auto full=full_moves(int(t.size()),int(outside.size()));
  std::set<std::tuple<int,int,int>> all;
  for(auto v:full)all.emplace(int(v.kind),v.i,v.j);
  verify(all==unique,"K=n-1 should equal entire neighborhood");
  Result fullres=steepest_search(p,t,SearchMode::Full);
  Result candres=steepest_search(p,t,SearchMode::Candidates,&near);
  verify(fullres.objective<=orig && candres.objective<=orig,"Full-K searches should improve");
 }
 auto sol=steepest_search(p,t,SearchMode::Candidates,&near);
 verify(evaluate(p,sol.tour)==sol.objective,"Delta accumulated objective wrong");
 verify(sol.objective<=orig,"Search worsened solution");
 auto fin=candidate_moves(p,sol.tour,outside_of(p,sol.tour),near);
 auto outs=outside_of(p,sol.tour);
 for(auto z:fin){auto d=z.kind==MoveKind::TwoOpt?opt_delta(p,sol.tour,z.i,z.j):replace_delta(p,sol.tour,z.i,outs[z.j]);verify(d>=0,"Not candidate local optimum");}
}
int main() {
 try {
  std::mt19937 rng(123);
  for(int n:{8,9,10,11,12,13}) {
   std::vector<Node> nodes;
   for(int i=0;i<n;i++)nodes.push_back({int(rng()%25),int(rng()%25),Cost(rng()%60)});
   auto p=make_problem(nodes);
   for(int r=0;r<10;r++) {
    auto t=random_solution(p,rng);
    for(int k:{1,3,n-1}) if(k<n)test_one(p,t,k);
   }
  }
  std::cout<<"PASS: candidate eligibility, both orientations, all deltas, full-K equality, local optimality.\n";
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
