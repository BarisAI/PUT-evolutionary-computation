#include "candidate_search.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <sstream>
#include <string>

namespace fs=std::filesystem;
using Clock=std::chrono::steady_clock;
inline double ms_since(Clock::time_point start) {return std::chrono::duration<double,std::milli>(Clock::now()-start).count();}
struct Group {
 std::string instance,method;
 int runs=0,best_start=-1;
 Cost min=std::numeric_limits<Cost>::max(),max=std::numeric_limits<Cost>::min();
 long double sum=0,sum_ms=0,sum_eval=0,sum_accepted=0,sum_initial=0;
 double pre_ms_per_run=0;
 Tour best_tour;
 void add(int index,const Tour&t,Cost initial,const Result&r,double time) {
  ++runs;sum+=r.objective;sum_ms+=time;sum_eval+=r.evaluations;
  sum_accepted+=r.accepted;sum_initial+=initial;
  if(r.objective<min){min=r.objective;best_start=index;best_tour=t;}
  max=std::max(max,r.objective);
 }
};
void save_svg(const std::vector<Node>&nodes,const Tour&tour,const fs::path&file,
              const std::string&label,Cost objective){
 std::ofstream out(file);if(!out)throw std::runtime_error("Cannot write SVG");
 int xmin=nodes.front().x,xmax=xmin,ymin=nodes.front().y,ymax=ymin;
 Cost cmin=nodes.front().cost,cmax=cmin;
 for(auto v:nodes){xmin=std::min(xmin,v.x);xmax=std::max(xmax,v.x);
  ymin=std::min(ymin,v.y);ymax=std::max(ymax,v.y);cmin=std::min(cmin,v.cost);cmax=std::max(cmax,v.cost);}
 double sc=580.0/std::max({1,xmax-xmin,ymax-ymin});
 auto x=[&](int i){return 70.0+(nodes[i].x-xmin)*sc;};
 auto y=[&](int i){return 670.0-(nodes[i].y-ymin)*sc;};
 auto color=[&](Cost c){double t=cmax==cmin?0.0:double(c-cmin)/double(cmax-cmin);
  return "rgb("+std::to_string(int(40+200*t))+","+std::to_string(int(130-80*t))+","+std::to_string(int(220-180*t))+")";};
 out<<std::fixed<<std::setprecision(2);
 out<<"<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"860\" height=\"780\" viewBox=\"0 0 860 780\">\n"
    <<"<rect width=\"860\" height=\"780\" fill=\"white\"/>\n"
    <<"<g font-family=\"sans-serif\" fill=\"#17222d\"><text x=\"60\" y=\"35\" font-size=\"20\">"
    <<label<<"</text><text x=\"60\" y=\"60\" font-size=\"15\">Objective: "<<objective
    <<" | selected: "<<tour.size()<<" / "<<nodes.size()<<"</text></g>\n"
    <<"<path d=\"M 70 90 V 670 H 670\" fill=\"none\" stroke=\"#aab4be\"/>\n"
    <<"<polyline fill=\"none\" stroke=\"#334155\" stroke-width=\"1.5\" points=\"";
 for(int node:tour)out<<x(node)<<','<<y(node)<<' ';
 out<<x(tour.front())<<','<<y(tour.front())<<"\"/>\n";
 std::vector<bool> selected(nodes.size());for(int v:tour)selected[v]=true;
 for(int layer=0;layer<2;layer++)for(int i=0;i<int(nodes.size());i++){
  if(selected[i]!=bool(layer))continue;
  out<<"<circle cx=\""<<x(i)<<"\" cy=\""<<y(i)<<"\" r=\""
     <<(selected[i]?4.5:3.0)<<"\" fill=\""<<color(nodes[i].cost)
     <<"\" opacity=\""<<(selected[i]?1.0:0.45)<<"\" stroke=\""
     <<(selected[i]?"#0f172a":"none")<<"\"><title>Node "<<i<<"; cost "<<nodes[i].cost
     <<(selected[i]?"; selected":"; not selected")<<"</title></circle>\n";
 }
 out<<"<g font-family=\"sans-serif\" font-size=\"13\" fill=\"#334155\"><text x=\"702\" y=\"125\">Node cost</text>\n";
 for(int i=0;i<100;i++)out<<"<rect x=\"705\" y=\""<<150+i*3<<"\" width=\"20\" height=\"3.1\" fill=\""
   <<color(cmin+(cmax-cmin)*i/99)<<"\"/>\n";
 out<<"<text x=\"735\" y=\"160\">"<<cmin<<"</text><text x=\"735\" y=\"450\">"<<cmax
    <<"</text><text x=\"700\" y=\"500\">Outlined: selected</text><text x=\"700\" y=\"525\">Faded: unselected</text>"
    <<"<text x=\"55\" y=\"755\">Closed tour; node indices start at zero.</text></g></svg>\n";
}
int main(int argc,char**argv) {
 try {
  fs::path data=argc>1?argv[1]:"../Assignment 1/data";
  fs::path output=argc>2?argv[2]:"results";
  int runs=argc>3?std::stoi(argv[3]):200;
  if(runs<1||runs>200)throw std::runtime_error("Runs must be 1..200");
  fs::create_directories(output/"plots");
  std::ofstream runlog(output/"runs.csv"),summary(output/"summary.csv"),best(output/"best_solutions.txt"),log(output/"run_info.txt");
  if(!runlog||!summary||!best||!log)throw std::runtime_error("Cannot write output");
  runlog<<"instance,method,run,initial_objective,objective,accepted_moves,delta_evaluations,local_ms\n";
  summary<<"instance,method,runs,min,max,average_objective,mean_local_ms,mean_total_ms,mean_delta_evaluations,mean_moves\n";
  best<<"# Instance method objective, followed by 100 ordered zero-based indices (closing node omitted).\n";
  std::mt19937 rng(42);
  const std::array<std::string,2> instances{"TSPA","TSPB"};
  const std::array<int,3> ks{5,10,20};
  for(const auto&name:instances) {
   const auto nodes=read_nodes((data/(name+".csv")).string());
   Problem p=make_problem(nodes);
   std::vector<Tour> starts;starts.reserve(runs);
   for(int run=0;run<runs;run++)starts.push_back(random_solution(p,rng));
   const std::array<std::string,4> methods{"full_steepest","candidate_k5","candidate_k10","candidate_k20"};
   for(int mi=0;mi<4;mi++) {
    const bool candidate=mi>0;
    double prep_ms=0;
    CandidateList near;
    if(candidate){auto t=Clock::now();near=nearest_candidates(p,ks[mi-1]);prep_ms=ms_since(t);}
    Group group;group.instance=name;group.method=methods[mi];
    for(int run=0;run<runs;run++) {
     const Cost initial=evaluate(p,starts[run]);
     const auto before=Clock::now();
     Result result=steepest_search(p,starts[run],candidate?SearchMode::Candidates:SearchMode::Full,candidate?&near:nullptr);
     const double local_ms=ms_since(before);
     if(result.objective>initial || evaluate(p,result.tour)!=result.objective)
       throw std::runtime_error("Search result objective validation failed");
     group.add(run,result.tour,initial,result,local_ms);
     runlog<<name<<','<<methods[mi]<<','<<run<<','<<initial<<','<<result.objective
           <<','<<result.accepted<<','<<result.evaluations<<','<<std::fixed<<std::setprecision(6)<<local_ms<<'\n';
    }
    group.pre_ms_per_run=prep_ms/runs;
    // Same start tours used by the full and candidate versions; random generation
    // is ~same in each version and excluded from primary search-only timing.
    const double average=double(group.sum/group.runs),search_avg=double(group.sum_ms/group.runs);
    summary<<name<<','<<methods[mi]<<','<<group.runs<<','<<group.min<<','<<group.max<<','
           <<std::fixed<<std::setprecision(3)<<average<<','<<std::setprecision(6)<<search_avg
           <<','<<search_avg+group.pre_ms_per_run<<','<<double(group.sum_eval/group.runs)
           <<','<<double(group.sum_accepted/group.runs)<<'\n';
    best<<name<<' '<<methods[mi]<<' '<<group.min;
    for(int v:group.best_tour)best<<' '<<v;
    best<<'\n';
    save_svg(nodes,group.best_tour,output/"plots"/(name+"_"+methods[mi]+".svg"),name+" / "+methods[mi],group.min);
    std::cout<<name<<' '<<methods[mi]<<" best="<<group.min<<" average="<<average
             <<" mean_local_ms="<<search_avg<<" preparation_ms="<<prep_ms<<'\n'<<std::flush;
   }
  }
  log<<"Same initial random tours across all four variants for each instance.\n"
     <<"Random Fisher-Yates seed 42, TSPA generated before TSPB.\n"
     <<"Candidates: for each selected u, visit its K directed nearest vertices v ranked by D[u][v] + cost[v].\n"
     <<"Candidate moves: two 2-opt cuts or two inter-route replacements that introduce (u,v).\n"
     <<"No full-neighborhood filtering. We enumerate moves directly from candidate lists and deduplicate before delta evaluation.\n"
     <<"Full baseline: identical steepest two-opt plus replacement neighborhood as Assignment 3.\n"
     <<"Runtime is from std::chrono::steady_clock; mean_local_ms covers search only; mean_total_ms adds candidate-list preprocessing amortized over runs.\n"
     <<"The timing excludes reading files, plot writing and startup tour creation (shared across variants).\n";
  return 0;
 }catch(const std::exception&e){std::cerr<<"ERROR: "<<e.what()<<'\n';return 1;}
}
