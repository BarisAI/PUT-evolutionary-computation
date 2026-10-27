#pragma once
#include "../../Assignment 1/src/problem.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

// We reuse 2-opt + selected/unselected replacement, the strongest random-start
// local-search neighbourhood observed in Assignment 3.
enum class SearchMode { Full, Candidates };
enum class MoveKind { TwoOpt, Replace };
struct Move { MoveKind kind; int i,j; };
struct Result { Tour tour; Cost objective; int accepted; std::uint64_t evaluations,enumerated; };
using CandidateList = std::vector<std::vector<int>>;

// Directed K-nearest lists: select v minimizing D[u][v] + cost[v]. This follows
// the lecturer's selected-vertex -> nearest-vertex iteration convention.
inline CandidateList nearest_candidates(const Problem& p,int k) {
    if(k<1||k>=p.size())throw std::runtime_error("Candidate count must be 1..n-1");
    CandidateList near(p.size());
    for(int u=0;u<p.size();++u) {
        std::vector<int> ids;
        for(int v=0;v<p.size();++v)if(u!=v)ids.push_back(v);
        std::sort(ids.begin(),ids.end(),[&](int a,int b){
            const Cost da=p.distance[u][a]+p.cost[a], db=p.distance[u][b]+p.cost[b];
            return da==db?a<b:da<db;
        });
        ids.resize(k);
        near[u]=std::move(ids);
    }
    return near;
}

// For 2-opt, cut (t[i],t[i+1]) and (t[j],t[j+1]), i<j.
// New edges (t[i],t[j]) and (t[i+1],t[j+1]) both count toward candidate test.
inline bool valid_two_opt(int i,int j,int m) {
    return i>=0 && j>=0 && i<j && j<m && j!=i+1 && !(i==0&&j==m-1);
}
inline Cost opt_delta(const Problem&p,const Tour&t,int i,int j) {
    int m=int(t.size());int a=t[i],b=t[(i+1)%m],c=t[j],d=t[(j+1)%m];
    return p.distance[a][c]+p.distance[b][d]-p.distance[a][b]-p.distance[c][d];
}
inline Cost replace_delta(const Problem&p,const Tour&t,int i,int candidate) {
    const int m=int(t.size());int old=t[i];
    if(m==1)return p.cost[candidate]-p.cost[old];
    int a=t[(i+m-1)%m],b=t[(i+1)%m];
    return p.cost[candidate]-p.cost[old]
         +p.distance[a][candidate]+p.distance[candidate][b]
         -p.distance[a][old]-p.distance[old][b];
}
inline void apply(Tour&t,const Move&z,const std::vector<int>&outside) {
    if(z.kind==MoveKind::TwoOpt)std::reverse(t.begin()+z.i+1,t.begin()+z.j+1);
    else t[z.i]=outside[z.j];
}
inline std::vector<int> outside_of(const Problem&p,const Tour&t) {
    std::vector<char> in(p.size());for(int v:t)in[v]=1;
    std::vector<int> outs;for(int v=0;v<p.size();v++)if(!in[v])outs.push_back(v);return outs;
}
inline void emit_unique(std::vector<Move>&out,std::vector<char>&seen,int m,int outcount,MoveKind kind,int i,int j) {
    if(kind==MoveKind::TwoOpt){if(i>j)std::swap(i,j);if(!valid_two_opt(i,j,m))return;}
    if(kind==MoveKind::Replace && (i<0||i>=m||j<0||j>=outcount))return;
    const std::size_t index=kind==MoveKind::TwoOpt?std::size_t(i)*m+j:
        std::size_t(m)*m+std::size_t(i)*outcount+j;
    if(seen[index])return;
    seen[index]=1;out.push_back({kind,i,j});
}

// IMPORTANT: candidate-only enumeration. We NEVER loop all moves to check
// whether they are candidates. Instead loop selected u -> near[u].
//   selected v: two distinct 2-opt cuts introducing edge (u,v)
//      1) u=t[i], v=t[j] -> cuts i,j
//      2) u=t[i+1], v=t[j+1] -> cuts i-1,j-1
//   unselected v: two replacements introducing edge (u,v)
//      1) replace predecessor of u with v
//      2) replace successor of u with v
// Deduplicate identical move descriptors (without calculating objective deltas).
inline std::vector<Move> candidate_moves(const Problem&p,const Tour&t,
                 const std::vector<int>&outside,const CandidateList&near) {
    const int m=int(t.size()),n=p.size();
    std::vector<int> pos(n,-1),out_pos(n,-1);
    for(int i=0;i<m;i++)pos[t[i]]=i;
    for(int j=0;j<int(outside.size());j++)out_pos[outside[j]]=j;
    std::vector<char> seen(std::size_t(m)*m+std::size_t(m)*outside.size());
    std::vector<Move> moves;
    moves.reserve(std::size_t(m)*near[0].size()*2);
    for(int i=0;i<m;i++) {
        const int u=t[i];
        for(int v:near[u]) {
            if(pos[v]>=0) {
                const int j=pos[v];
                emit_unique(moves,seen,m,int(outside.size()),MoveKind::TwoOpt,i,j);
                emit_unique(moves,seen,m,int(outside.size()),MoveKind::TwoOpt,
                    (i+m-1)%m,(j+m-1)%m);
            } else {
                const int j=out_pos[v];
                emit_unique(moves,seen,m,int(outside.size()),MoveKind::Replace,(i+m-1)%m,j);
                emit_unique(moves,seen,m,int(outside.size()),MoveKind::Replace,(i+1)%m,j);
            }
        }
    }
    return moves;
}

// Full neighbourhood exactly matches Assignment 3's Steepest + 2-opt + inter
// ordering, for a like-for-like baseline. No candidate filtering is performed.
inline std::vector<Move> full_moves(int m,int outcount) {
    std::vector<Move> moves;
    for(int i=0;i<m;i++)for(int j=i+1;j<m;j++)
        if(valid_two_opt(i,j,m))moves.push_back({MoveKind::TwoOpt,i,j});
    for(int i=0;i<m;i++)for(int j=0;j<outcount;j++)
        moves.push_back({MoveKind::Replace,i,j});
    return moves;
}

inline Result steepest_search(const Problem&p,Tour init, SearchMode mode,
                       const CandidateList*near=nullptr) {
    if(mode==SearchMode::Candidates && (!near||int(near->size())!=p.size()))
        throw std::runtime_error("Invalid candidate lists");
    Cost obj=evaluate(p,init);
    Result result{std::move(init),obj,0,0,0};
    const int m=int(result.tour.size());
    const auto whole=mode==SearchMode::Full?full_moves(m,p.size()-m):std::vector<Move>{};
    for(;;) {
        auto outside=outside_of(p,result.tour);
        const auto moves=mode==SearchMode::Full?whole:candidate_moves(p,result.tour,outside,*near);
        ++result.enumerated; // counts scans; separate 'evaluations' below
        Cost best=0;
        Move chosen{MoveKind::TwoOpt,0,0};
        for(const auto &move:moves) {
            Cost delta=move.kind==MoveKind::TwoOpt?opt_delta(p,result.tour,move.i,move.j):
                    replace_delta(p,result.tour,move.i,outside[move.j]);
            ++result.evaluations;
            if(delta<best) {best=delta;chosen=move;}
        }
        if(best>=0)break;
        apply(result.tour,chosen,outside);
        result.objective+=best;
        ++result.accepted;
    }
    return result;
}

// Same reproducible Fisher-Yates selection as Assignment 1.
inline Tour random_solution(const Problem&p,std::mt19937&rng) {
    Tour t(p.size());std::iota(t.begin(),t.end(),0);
    for(int i=p.size()-1;i>0;--i){
        std::uint64_t range=std::uint64_t(i)+1;
        std::uint64_t limit=(std::uint64_t(1)<<32)/range*range;
        std::uint64_t draw;
        do{draw=rng();}while(draw>=limit);
        std::swap(t[i],t[draw%range]);
    }
    t.resize(p.selected_count());return t;
}
