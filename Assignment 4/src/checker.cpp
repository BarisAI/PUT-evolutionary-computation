#include "../../Assignment 1/src/problem.hpp"
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <filesystem>
int main(int argc,char**argv){
 try{
  if(argc!=3)throw std::runtime_error("Usage: checker DATA_DIR best_solutions.txt");
  const auto data=std::filesystem::path(argv[1]);
  std::ifstream in(argv[2]);if(!in)throw std::runtime_error("Cannot read best solutions");
  std::string line;int passed=0;
  while(std::getline(in,line)) {
   if(line.empty()||line[0]=='#')continue;
   std::istringstream ss(line);std::string id,method;Cost claimed;
   if(!(ss>>id>>method>>claimed))throw std::runtime_error("Invalid solution header");
   Tour tour;int v;while(ss>>v)tour.push_back(v);
   const auto nodes=read_nodes((data/(id+".csv")).string());
   const int n=int(nodes.size()),k=(n+1)/2;
   if(int(tour.size())!=k||int(std::set<int>(tour.begin(),tour.end()).size())!=k)
       throw std::runtime_error("Invalid cardinality/uniqueness for "+id+" "+method);
   Cost distance=0,nodecost=0;
   for(int i=0;i<k;i++) {
    int a=tour[i],b=tour[(i+1)%k];
    if(a<0||a>=n||b<0||b>=n)throw std::runtime_error("Bad index");
    nodecost+=nodes[a].cost;
    distance+=Cost(std::floor(std::hypot(double(nodes[a].x)-nodes[b].x,double(nodes[a].y)-nodes[b].y)+0.5));
   }
   if(nodecost+distance!=claimed)throw std::runtime_error("Mismatch "+id+" "+method);
   std::cout<<"PASS "<<id<<' '<<method<<" selected="<<k<<" distance="<<distance<<" cost="<<nodecost<<" total="<<claimed<<'\n';
   passed++;
  }
  if(passed!=8)throw std::runtime_error("Expected 8 best solutions");
  std::cout<<"All 8 best tours independently checked.\n";
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
