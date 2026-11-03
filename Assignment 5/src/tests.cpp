#include "lm_search.hpp"
#include "../../Assignment 4/src/candidate_search.hpp"
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <stdexcept>

void ensure(bool x,const char*msg){if(!x)throw std::runtime_error(msg);}
void test_two_orientation(const Problem&p,const Tour&t) {
    lm::State s(p,t);int count=0,deferred=0;
    for(int i=0;i<int(t.size());i++)for(int j=i+1;j<int(t.size());j++) {
        lm::Edge e1=s.edges[i],e2=s.edges[j];
        if(!lm::valid_pair(e1,e2))continue;
        int applicable=0;
        for(int mode=0;mode<2;mode++){
            const auto k=lm::two_key(e1,e2,mode);
            auto state=lm::check(k,t,s);
            ensure(state.status!=lm::Status::Invalid,"Two-edge must exist");
            if(state.status==lm::Status::Applicable){
                applicable++;
                Tour cp=t;lm::apply(cp,k,state);
                ensure(lm::fixed_two_delta(p,k)==evaluate(p,cp)-evaluate(p,t),"2opt delta mismatch");
            }else deferred++;
        }
        ensure(applicable==1,"Exactly one relative orientation is applicable");
        count++;
    }
    ensure(count>0&&deferred==count,"Inverted-edge moves evaluated too");
}
void test_replace(const Problem&p,const Tour&t) {
    lm::State s(p,t);
    for(int old:t)for(int fresh:s.outside){
        int i=s.position[old],m=int(t.size());
        auto key=lm::replacement_key(old,fresh,t[(i+m-1)%m],t[(i+1)%m]);
        auto state=lm::check(key,t,s);
        ensure(state.status==lm::Status::Applicable,"Valid replacement rejected");
        Tour cp=t;lm::apply(cp,key,state);
        ensure(evaluate(p,cp)-evaluate(p,t)==lm::fixed_replace_delta(p,key),"Replacement delta mismatch");
    }
}
int main(){try{
  const Problem p=make_problem({{0,0,4},{3,4,1},{8,1,6},{12,8,7},
    {9,14,3},{1,13,10},{17,12,8},{20,0,0},{5,18,4},{15,17,9},
    {8,20,2},{13,1,6},{21,8,5},{1,20,2},{16,4,1},{24,15,8}});
  std::mt19937 rng(432);
  for(int rep=0;rep<80;rep++){
    Tour t(p.size());std::iota(t.begin(),t.end(),0);std::shuffle(t.begin(),t.end(),rng);
    t.resize(p.selected_count());test_two_orientation(p,t);test_replace(p,t);
    auto lmres=lm::local_search(p,t);
    ensure(lmres.objective==evaluate(p,lmres.tour),"LM accumulated objective incorrect");
    ensure(lmres.objective<=evaluate(p,t),"LM must improve or keep quality");
    std::mt19937 dummy(12);
    auto full=steepest_search(p,lmres.tour,SearchMode::Full);
    ensure(full.objective==lmres.objective,"LM must reach unrestricted local optimum");
  }
  // Small odd and even instances, including the one-node closed tour.
  for(int n : {1,2,3,4,5,6,7,8}) {
    std::vector<Node> nodes;
    for(int i=0;i<n;i++) nodes.push_back({3*i,2*i,10-i});
    Problem q=make_problem(nodes); Tour first;
    for(int j=0;j<q.selected_count();j++)first.push_back(j);
    auto sol=lm::local_search(q,first);
    ensure(sol.objective==evaluate(q,sol.tour),"Small instance objective mismatch");
    ensure(sol.objective<=evaluate(q,first),"Small instance must not worsen");
  }
  // Same edge identity, relative direction flipped by reversing just one edge.
  {
    const Tour before{0,1,2,3,4,5,6,7};
    const Tour after{0,3,2,1,4,5,6,7};
    lm::Key key=lm::two_key(lm::edge(1,2),lm::edge(5,6),0);
    const auto a=lm::check(key,before,lm::State(p,before));
    const auto b=lm::check(key,after,lm::State(p,after));
    ensure(a.status!=lm::Status::Invalid&&b.status!=lm::Status::Invalid,"Both removed edges must exist");
    ensure(a.status!=b.status,"One reversed edge must flip applicability");
  }
  std::cout<<"PASS: orientations (including inverted), exact 2-opt/replacement deltas, feasibility, full-neighborhood local optima.\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
