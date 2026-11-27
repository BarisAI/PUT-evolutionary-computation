// Independent checker: reads original coordinates and verifies every final
// Greedy local optimum against the complete 2-opt + replacement neighborhood.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
using Cost=std::int64_t;
struct Point {int x,y;Cost cost;};
static std::vector<Point> read_instance(const std::string& path){
 std::ifstream f(path);if(!f)throw std::runtime_error("Missing data: "+path);
 std::vector<Point> points;std::string line;
 while(std::getline(f,line)){
  if(line.empty())continue;
  std::replace(line.begin(),line.end(),';',' ');
  std::istringstream ss(line);Point pt{};if(!(ss>>pt.x>>pt.y>>pt.cost))throw std::runtime_error("Invalid node row");
  points.push_back(pt);
 }
 return points;
}
static void check_tour(const std::vector<Point>& points,const std::vector<int>& t,Cost reported){
 const int n=int(points.size()),m=int(t.size());
 if(n!=200||m!=100)throw std::runtime_error("Wrong size");
 std::vector<bool> used(n);for(int v:t){if(v<0||v>=n||used[v])throw std::runtime_error("Repeated/out-of-range node");used[v]=true;}
 std::vector<std::vector<Cost>> d(n,std::vector<Cost>(n));
 for(int i=0;i<n;++i)for(int j=0;j<i;++j){
  double dx=double(points[i].x)-points[j].x,dy=double(points[i].y)-points[j].y;
  d[i][j]=d[j][i]=Cost(std::floor(std::hypot(dx,dy)+0.5));
 }
 Cost score=0;
 for(int i=0;i<m;++i)score+=points[t[i]].cost+d[t[i]][t[(i+1)%m]];
 if(score!=reported)throw std::runtime_error("Objective is incorrect");
 for(int i=0;i<m;++i)for(int j=i+2;j<m;++j){
  if(i==0 && j==m-1)continue;
  int a=t[i],b=t[(i+1)%m],c=t[j],e=t[(j+1)%m];
  if(d[a][c]+d[b][e]-d[a][b]-d[c][e]<0)throw std::runtime_error("A 2-opt improvement remains");
 }
 for(int i=0;i<m;++i){
  int old=t[i],left=t[(i+m-1)%m],right=t[(i+1)%m];
  for(int fresh=0;fresh<n;++fresh){if(used[fresh])continue;
   Cost change=points[fresh].cost-points[old].cost+d[left][fresh]+d[fresh][right]-d[left][old]-d[old][right];
   if(change<0)throw std::runtime_error("A replacement improvement remains");
  }
 }
}
int main(int argc,char**argv){
 try{
  if(argc!=3)throw std::runtime_error("Usage: checker data_dir local_optima.csv");
  std::vector<Point> points[2]={read_instance(std::string(argv[1])+"/TSPA.csv"),read_instance(std::string(argv[1])+"/TSPB.csv")};
  std::ifstream f(argv[2]);if(!f)throw std::runtime_error("Missing local_optima.csv");
  std::string line;std::getline(f,line);int counts[2]{};
  while(std::getline(f,line)){
   if(line.empty())continue;
   std::stringstream ss(line);std::string field[7];
   for(int k=0;k<6;++k){if(!std::getline(ss,field[k],','))throw std::runtime_error("Bad columns");}
   std::getline(ss,field[6]);
   int index=field[0]=="TSPA"?0:field[0]=="TSPB"?1:-1;
   if(index<0)throw std::runtime_error("Unknown instance");
   if(std::stoi(field[1])!=counts[index])throw std::runtime_error("Unexpected run index");
   std::istringstream ts(field[6]);std::vector<int> tour;int v;
   while(ts>>v)tour.push_back(v);
   try{check_tour(points[index],tour,std::stoll(field[2]));}
   catch(const std::exception&ex){throw std::runtime_error(field[0]+" run="+field[1]+": "+ex.what());}
   ++counts[index];
  }
  if(counts[0]!=1000||counts[1]!=1000)throw std::runtime_error("Expected 1000 optima per instance");
  std::cout<<"PASS: TSPA="<<counts[0]<<" and TSPB="<<counts[1]
           <<" unique-node valid tours; exact full objectives; no improving 2-opt/replacements\n";
  return 0;
 }catch(const std::exception&ex){std::cerr<<"FAIL: "<<ex.what()<<'\n';return 1;}
}
