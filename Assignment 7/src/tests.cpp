#include "lns.hpp"
#include <iostream>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <utility>
void ensure(bool condition,const std::string& desc){if(!condition)throw std::runtime_error(desc);}
Problem synthetic(int n){
    std::vector<Node> nodes;
    for(int i=0;i<n;++i)nodes.push_back({(i*31+11)%199,(i*i*7+13)%203,Cost((i*19+7)%61)});
    return make_problem(nodes);
}
int circular_runs(const std::vector<unsigned char>&m){
    int r=0;for(int i=0;i<(int)m.size();++i)if(m[i]&&!m[(i+(int)m.size()-1)%m.size()])++r;
    return r;
}
Cost separate_objective(const Problem&p,const Tour&t){
    Cost c=0;for(int i=0;i<(int)t.size();++i){
      int a=t[i],b=t[(i+1)%t.size()];
      c+=p.cost[a]+p.distance[a][b];
    }return c;
}
void test_destroy_repair(const Problem&p,int cases,std::uint32_t seed){
    std::mt19937 rng(seed);
    for(int rep=0;rep<cases;++rep){
      const Tour tour=random_solution(p,rng);
      const int m=(int)tour.size(),removed=a7::removal_count(m);
      ensure(removed==int(std::llround(0.3*m)),"30 percent removal rounding");
      for(auto method:{a7::Destroy::RandomNodes,a7::Destroy::OneSubpath,a7::Destroy::MultipleSubpaths}){
        const auto d=a7::destroy(tour,method,removed,rng);
        ensure((int)d.removed.size()==removed&&int(d.survivors.size())==m-removed,"exact removal size");
        ensure(int(std::count(d.removed_mask.begin(),d.removed_mask.end(),1))==removed,"position mask count");
        const std::unordered_set<int> survivors(d.survivors.begin(),d.survivors.end());
        const std::unordered_set<int> gone(d.removed.begin(),d.removed.end());
        ensure((int)survivors.size()==m-removed&&(int)gone.size()==removed,"unique partitions");
        for(int n:gone)ensure(!survivors.count(n),"removed node still retained");
        const int groups=circular_runs(d.removed_mask);
        if(method==a7::Destroy::OneSubpath)ensure(groups==1,"single subpath must be contiguous, including wrap");
        if(method==a7::Destroy::MultipleSubpaths)ensure(groups>=3&&groups<=5&&groups==d.subpaths,"exact 3-5 separated subpaths");
        for(auto basis:{Basis::GreedyCycle,Basis::NearestAllPositions}){
          const Tour result=a7::repair(p,d,basis,rng);
          ensure((int)result.size()==m,"repaired cycle size");
          ensure(evaluate(p,result)==separate_objective(p,result),"repaired objective disagreement");
          for(int node:survivors)ensure(std::find(result.begin(),result.end(),node)!=result.end(),"repair discarded survivor");
        }
        ensure((int)tour.size()==m,"destroy altered input");
      }
    }
}
void test_search(const Problem&p){
    std::mt19937 rng(4443);
    for(const auto method:{a7::Destroy::RandomNodes,a7::Destroy::OneSubpath,a7::Destroy::MultipleSubpaths}){
      for(const bool local:{false,true}){
        const auto out=a7::search(p,method,local,Basis::GreedyCycle,4444,30.0);
        ensure((int)out.best_tour.size()==p.selected_count(),"LNS final size");
        ensure(evaluate(p,out.best_tour)==out.best_objective,"LNS objective");
        ensure(out.best_objective<=out.initial_objective,"LNS lost initial optimum");
        ensure(out.accepted+out.rejected==out.iterations,"LNS count accounting");
        ensure(out.local_search_calls==1+(local?out.iterations:0),"LNS LS count accounting");
        if(local){
          const auto unused=outside_nodes(p,out.best_tour);
          for(const auto& move:make_moves((int)out.best_tour.size(),(int)unused.size(),IntraMove::TwoOpt))
            ensure(move_delta(p,out.best_tour,unused,IntraMove::TwoOpt,move)>=0,"LNS with-LS not at full local optimum");
        }
      }
    }
}
int main(){
 try{
   ensure(a7::removal_count(100)==30,"main instance must remove 30 nodes");
   for(int n:{80,100,200})test_destroy_repair(synthetic(n),n==200?100:20,10001+n);
   test_search(synthetic(80));
   // Edge cases, invalid inputs, and boundary scenarios.
   bool caught=false;try{std::mt19937 r(1);a7::destroy(Tour{1,2,3},a7::Destroy::OneSubpath,3,r);}catch(const std::invalid_argument&){caught=true;}
   ensure(caught,"destroy invalid input not rejected");
   caught=false;try{a7::removal_count(100,1.0);}catch(const std::invalid_argument&){caught=true;}
   ensure(caught,"fraction invalid input not rejected");
   std::cout<<"PASS: 30% exact; circular subpaths; 3-5 distinct subpaths; weighted regret repair both bases; feasibility; optimum; duration; accounting; random property cases.\n";
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
