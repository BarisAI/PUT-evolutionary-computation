#include "lns.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
// Standalone independent verification of each best tour using raw coordinates.
// For with-LS, also verify no negative 2-opt or replacement delta remains.
int main(int argc,char**argv){
 try{
  if(argc!=3)throw std::runtime_error("Usage: assignment7_checker data_dir best_solutions.txt");
  std::ifstream in(argv[2]);if(!in)throw std::runtime_error("Cannot read tours file");
  std::string line;int total=0;
  while(std::getline(in,line)){
    if(line.empty()||line[0]=='#')continue;
    std::istringstream ss(line);std::string dataset,method,mode;
    Cost expected;
    if(!(ss>>dataset>>method>>mode>>expected))throw std::runtime_error("Incorrect best file record");
    const auto nodes=read_nodes(std::string(argv[1])+"/"+dataset+".csv");
    Tour tour;int v;while(ss>>v)tour.push_back(v);
    if(tour.size()!=100||nodes.size()!=200)throw std::runtime_error("Wrong number of nodes");
    const std::set<int> distinct(tour.begin(),tour.end());
    if(distinct.size()!=100||*distinct.begin()<0||*distinct.rbegin()>=200)throw std::runtime_error("Duplicate or out of bounds node");
    Cost travel=0,selected_cost=0;
    for(std::size_t i=0;i<tour.size();++i){
        const Node&a=nodes[tour[i]],&b=nodes[tour[(i+1)%tour.size()]];
        travel+=static_cast<Cost>(std::floor(std::hypot(double(a.x)-b.x,double(a.y)-b.y)+0.5));
        selected_cost+=a.cost;
    }
    if(travel+selected_cost!=expected)throw std::runtime_error("Objective mismatch for "+dataset+" "+method+" "+mode);
    if(mode=="with_ls"){
      const auto p=make_problem(nodes);const auto unused=outside_nodes(p,tour);
      for(const Move& move:make_moves((int)tour.size(),(int)unused.size(),IntraMove::TwoOpt))
        if(move_delta(p,tour,unused,IntraMove::TwoOpt,move)<0)throw std::runtime_error("Not full local optimum");
    }
    std::cout<<"PASS "<<dataset<<' '<<method<<' '<<mode<<" travel="<<travel<<" nodes="<<selected_cost<<" total="<<expected<<'\n';
    ++total;
  }
  if(total!=12)throw std::runtime_error("Expected exactly twelve best tour records, found "+std::to_string(total));
  std::cout<<"PASS all "<<total<<" best tours.\n";
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
