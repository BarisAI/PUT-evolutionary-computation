#include "../../Assignment 1/src/heuristics.hpp"
#include "../../Assignment 3/src/local_search.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>

namespace fs=std::filesystem;
using Clock=std::chrono::steady_clock;
int main(int argc,char**argv){
 try{
  const fs::path data=argc>1?argv[1]:"../Assignment 1/data";
  const fs::path output=argc>2?argv[2]:"results/local_optima.csv";
  const int count=argc>3?std::stoi(argv[3]):1000;
  if(count<=0)throw std::runtime_error("Number of runs must be positive");
  fs::create_directories(output.parent_path());
  std::ofstream out(output);
  if(!out)throw std::runtime_error("Cannot write output");
  out<<"instance,run,objective,accepted_moves,delta_checks,elapsed_ms,tour\n";
  for(int id=0;id<2;++id){
   const std::string name=id==0?"TSPA":"TSPB";
   const auto nodes=read_nodes((data/(name+".csv")).string());
   const Problem p=make_problem(nodes);
   if(p.size()!=200||p.selected_count()!=100)throw std::runtime_error("Unexpected instance size");
   const std::uint32_t seed=20260801u+std::uint32_t(id)*100u;
   std::mt19937 rng(seed);
   Cost best=std::numeric_limits<Cost>::max();
   long double sum=0;
   for(int run=0;run<count;++run){
    const auto start=Clock::now();
    auto initial=random_solution(p,rng);
    auto result=local_search(p,std::move(initial),LocalStrategy::Greedy,IntraMove::TwoOpt,rng);
    const double ms=std::chrono::duration<double,std::milli>(Clock::now()-start).count();
    if(evaluate(p,result.tour)!=result.objective)throw std::runtime_error("Inconsistent objective");
    if(result.objective<best)best=result.objective;
    sum+=result.objective;
    out<<name<<','<<run<<','<<result.objective<<','<<result.accepted_moves<<','<<result.delta_checks
       <<','<<std::fixed<<std::setprecision(6)<<ms<<',';
    for(size_t i=0;i<result.tour.size();++i){if(i)out<<' ';out<<result.tour[i];}
    out<<'\n';
   }
   std::cout<<name<<" seed="<<seed<<" count="<<count<<" best="<<best
            <<" mean="<<std::fixed<<std::setprecision(3)<<double(sum/count)<<'\n';
  }
  return 0;
 }catch(const std::exception&ex){std::cerr<<"ERROR: "<<ex.what()<<'\n';return 1;}
}
