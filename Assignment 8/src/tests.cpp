#include "../../Assignment 1/src/heuristics.hpp"
#include "../../Assignment 3/src/local_search.hpp"
#include <iostream>
#include <stdexcept>
static void check(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
int main(){
 try{
  for(int n:{14,19,30}){
   std::vector<Node> nodes;
   for(int i=0;i<n;++i)nodes.push_back({i*31%97,i*i*7%83,Cost((i*17)%47)});
   const auto p=make_problem(nodes);std::mt19937 rng(5423+n);
   for(int i=0;i<10;++i){
    const auto initial=random_solution(p,rng);
    auto stats=local_search(p,initial,LocalStrategy::Greedy,IntraMove::TwoOpt,rng);
    check(stats.objective==evaluate(p,stats.tour),"Stored objective differs from full evaluation");
    check(stats.objective<=evaluate(p,initial),"Greedy made the solution worse");
    auto outside=outside_nodes(p,stats.tour);
    for(const auto&m:make_moves(int(stats.tour.size()),int(outside.size()),IntraMove::TwoOpt))
     check(move_delta(p,stats.tour,outside,IntraMove::TwoOpt,m)>=0,"Not a local optimum");
   }
  }
  std::cout<<"PASS: greedy local search, feasibility, full objective and local optimality (30 randomized cases)\n";
  return 0;
 }catch(const std::exception&ex){std::cerr<<"FAIL: "<<ex.what()<<'\n';return 1;}
}
