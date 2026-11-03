#pragma once
#include "../../Assignment 1/src/problem.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <queue>
#include <set>
#include <vector>
#include <stdexcept>

namespace lm {
using Edge = std::pair<int,int>;
inline Edge edge(int a, int b) { if(a>b)std::swap(a,b);return {a,b}; }
inline bool valid_pair(const Edge &a,const Edge &b) {
    return a.first!=b.first && a.first!=b.second && a.second!=b.first && a.second!=b.second;
}

// Every stored 2-opt move is determined by its two undirected cut edges AND
// one of the two possible re-connections. No vertex positions are cached.
// The first reconnect pairs (a,c),(b,d); the second pairs (a,d),(b,c).
struct Key {
    int kind=0, a=0, b=0, c=0, d=0, matching=0;
    bool operator==(const Key& x) const {
        return kind==x.kind && a==x.a && b==x.b && c==x.c && d==x.d && matching==x.matching;
    }
};
inline Key two_key(Edge e1,Edge e2,int matching) {
    if(e2<e1)std::swap(e1,e2);
    return {0,e1.first,e1.second,e2.first,e2.second,matching};
}
inline Key replacement_key(int old,int fresh,int left,int right) {
    if(left>right)std::swap(left,right);
    return {1,old,fresh,left,right,0};
}
inline Cost fixed_two_delta(const Problem&p,const Key&k) {
    const Cost removed=p.distance[k.a][k.b]+p.distance[k.c][k.d];
    return (k.matching==0 ? p.distance[k.a][k.c]+p.distance[k.b][k.d]
            :p.distance[k.a][k.d]+p.distance[k.b][k.c]) - removed;
}
inline Cost fixed_replace_delta(const Problem&p,const Key&k) {
    if(k.c==k.a && k.d==k.a) return p.cost[k.b]-p.cost[k.a]; // one-node cycle
    return p.cost[k.b]-p.cost[k.a]
           +p.distance[k.c][k.b]+p.distance[k.b][k.d]
           -p.distance[k.c][k.a]-p.distance[k.a][k.d];
}
struct State {
    std::vector<int> position;
    std::vector<Edge> edges;
    std::vector<int> outside;
    State(const Problem&p,const Tour&t):position(p.size(),-1) {
        const int m=int(t.size());edges.reserve(m);
        for(int i=0;i<m;i++) {position[t[i]]=i;edges.push_back(edge(t[i],t[(i+1)%m]));}
        for(int v=0;v<p.size();v++)if(position[v]<0)outside.push_back(v);
    }
    int cut_index(const Key&k, bool first,const Tour&t) const {
        const int a=first?k.a:k.c,b=first?k.b:k.d;
        int i=position[a],j=position[b];
        if(i<0||j<0)return -1;
        const int m=int(t.size());
        if(t[(i+1)%m]==b)return i;
        if(t[(j+1)%m]==a)return j;
        return -1;
    }
};
enum class Status { Invalid, Deferred, Applicable };
struct Check {Status status=Status::Invalid;int i=-1,j=-1;};
inline Check check(const Key& k,const Tour&t,const State&s) {
    const int m=int(t.size());
    if(k.kind==1) {
        const int i=s.position[k.a];
        if(i<0 || s.position[k.b]>=0)return {};
        const int before=t[(i+m-1)%m],after=t[(i+1)%m];
        if(edge(before,after)!=edge(k.c,k.d))return {};
        return {Status::Applicable,i,k.b};
    }
    const int ix=s.cut_index(k,true,t),jx=s.cut_index(k,false,t);
    if(ix<0||jx<0)return {}; // at least one removed edge disappeared
    if(ix==jx||!valid_pair(edge(k.a,k.b),edge(k.c,k.d)))return {};
    const int i=std::min(ix,jx),j=std::max(ix,jx);
    const int a=t[i],b=t[(i+1)%m],c=t[j],d=t[(j+1)%m];
    // The equality above depends on endpoints canonically ordered. Check both
    // unordered newly introduced edge sets, independent of tour reversal.
    const Edge e1=edge(a,c),e2=edge(b,d);
    const Edge want1=k.matching==0?edge(k.a,k.c):edge(k.a,k.d);
    const Edge want2=k.matching==0?edge(k.b,k.d):edge(k.b,k.c);
    const bool applicable=(e1==want1&&e2==want2)||(e1==want2&&e2==want1);
    return {applicable?Status::Applicable:Status::Deferred,i,j};
}
inline void apply(Tour&t,const Key&k,const Check&c) {
    if(k.kind==1)t[c.i]=k.b;
    else std::reverse(t.begin()+c.i+1,t.begin()+c.j+1);
}
struct Stats {
    Tour tour;
    Cost objective=0;
    int accepted=0, applied_two_opt=0, applied_replace=0;
    std::uint64_t evaluated_deltas=0, saved_moves=0;
    std::uint64_t discarded=0,deferred=0,refreshed=0;
    std::uint64_t added_by_update=0;
};
// Ordered by improving objective change, best first. A set prevents adding
// the same edge-identity move repeatedly. Stale entries are discarded lazily.
class ImprovingList {
    const Problem &p;
    struct Entry{Cost delta;Key key;std::uint64_t serial;};
    struct Order {
        bool operator()(const Entry& a,const Entry& b) const {
            return a.delta!=b.delta?a.delta>b.delta:a.serial>b.serial;
        }
    };
    std::priority_queue<Entry,std::vector<Entry>,Order> heap;
    std::uint64_t serial=0;
    Stats &stats;
public:
    ImprovingList(const Problem&prob,Stats&out):p(prob),stats(out){}
    void consider(Key k) {
        const Cost delta=k.kind==0?fixed_two_delta(p,k):fixed_replace_delta(p,k);
        ++stats.evaluated_deltas;
        if(delta<0){heap.push({delta,k,serial++});++stats.saved_moves;}
    }
    void two(Edge e1,Edge e2) {
        if(!valid_pair(e1,e2))return;
        consider(two_key(e1,e2,0));
        consider(two_key(e1,e2,1));
    }
    void replacements(const Tour&t,const State&s,int selected,int only_outside=-1) {
        const int i=s.position[selected];if(i<0)return;
        const int m=int(t.size());
        const int left=t[(i+m-1)%m],right=t[(i+1)%m];
        if(only_outside>=0){if(s.position[only_outside]<0)consider(replacement_key(selected,only_outside,left,right));return;}
        for(int v:s.outside)consider(replacement_key(selected,v,left,right));
    }
    void initial(const Tour&t){
        State s(p,t);
        for(int i=0;i<int(s.edges.size());i++)
            for(int j=i+1;j<int(s.edges.size());j++)two(s.edges[i],s.edges[j]);
        for(int v:t)replacements(t,s,v);
    }
    void update(const Tour&t,const State&before,const Key&performed){
        State after(p,t);
        auto old_edges=before.edges;
        std::sort(old_edges.begin(),old_edges.end());
        std::vector<Edge> added;
        for(auto e:after.edges)if(!std::binary_search(old_edges.begin(),old_edges.end(),e))added.push_back(e);
        const auto prior=stats.evaluated_deltas;
        for(auto e:added)for(auto other:after.edges)two(e,other);
        std::vector<char> affected(p.size(),0);
        for(auto e:added){affected[e.first]=1;affected[e.second]=1;}
        for(int v:t)if(affected[v])replacements(t,after,v);
        if(performed.kind==1)for(int v:t)replacements(t,after,v,performed.a);
        stats.added_by_update+=stats.evaluated_deltas-prior;
    }
    // Pop the lowest cached delta first. Deferred entries are kept untouched
    // and put back after the scan; invalid removed edges are forgotten.
    bool choose(const Tour&t,const State&s,Key&chosen,Check&where,Cost&delta){
        std::vector<Entry> deferred;
        while(!heap.empty()){
            auto e=heap.top();heap.pop();
            const auto check_result=check(e.key,t,s);
            if(check_result.status==Status::Invalid){++stats.discarded;}
            else if(check_result.status==Status::Deferred){deferred.push_back(e);++stats.deferred;}
            else {
                chosen=e.key;where=check_result;delta=e.delta;
                
                for(const auto &d:deferred)heap.push(d);
                return true;
            }
        }
        for(const auto &d:deferred)heap.push(d);
        return false;
    }
    void refresh(const Tour&t){++stats.refreshed;initial(t);}
};
inline Stats local_search(const Problem&p,Tour initial) {
    Stats out;out.objective=evaluate(p,initial);out.tour=std::move(initial);
    ImprovingList lm(p,out);
    lm.initial(out.tour);
    for(;;){
        State before(p,out.tour);
        Key move;Check where;Cost delta=0;
        if(!lm.choose(out.tour,before,move,where,delta)) {
            lm.refresh(out.tour);
            if(!lm.choose(out.tour,before,move,where,delta))break;
        }
        apply(out.tour,move,where);
        out.objective+=delta;
        ++out.accepted;
        if(move.kind==0)++out.applied_two_opt;else ++out.applied_replace;
        lm.update(out.tour,before,move);
    }
    return out;
}
} // namespace lm
