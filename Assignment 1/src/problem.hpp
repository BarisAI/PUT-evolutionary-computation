#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using Cost = std::int64_t;
using Tour = std::vector<int>;

struct Node { int x, y; Cost cost; };

// Optimization algorithms receive this structure, never the coordinates.
struct Problem {
    std::vector<std::vector<Cost>> distance;
    std::vector<Cost> cost;
    int size() const { return static_cast<int>(cost.size()); }
    int selected_count() const { return (size() + 1) / 2; }
};

inline std::vector<Node> read_nodes(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot open instance: " + path);
    std::vector<Node> nodes;
    std::string line;
    int line_number = 0;
    while (std::getline(file, line)) {
        ++line_number;
        if (line.find_first_not_of(" \t\r") == std::string::npos) continue;
        if (std::count(line.begin(), line.end(), ';') != 2)
            throw std::runtime_error("Expected three semicolon-separated integers at line " + std::to_string(line_number));
        std::replace(line.begin(), line.end(), ';', ' ');
        std::istringstream input(line);
        Node node{};
        std::string extra;
        if (!(input >> node.x >> node.y >> node.cost) || (input >> extra))
            throw std::runtime_error("Invalid instance row " + std::to_string(line_number));
        nodes.push_back(node);
    }
    if (nodes.empty()) throw std::runtime_error("Instance has no nodes");
    return nodes;
}

inline Problem make_problem(const std::vector<Node>& nodes) {
    Problem p;
    const int n = static_cast<int>(nodes.size());
    p.distance.assign(n, std::vector<Cost>(n));
    for (int i = 0; i < n; ++i) {
        p.cost.push_back(nodes[i].cost);
        for (int j = 0; j < i; ++j) {
            const double dx = static_cast<double>(nodes[i].x) - nodes[j].x;
            const double dy = static_cast<double>(nodes[i].y) - nodes[j].y;
            // Nonnegative distances: mathematical rounding, including .5 upwards.
            const Cost d = static_cast<Cost>(std::floor(std::hypot(dx, dy) + 0.5));
            p.distance[i][j] = p.distance[j][i] = d;
        }
    }
    return p;
}

inline Cost evaluate(const Problem& p, const Tour& tour) {
    if (static_cast<int>(tour.size()) != p.selected_count())
        throw std::runtime_error("Incorrect number of selected nodes");
    std::vector<bool> seen(p.size());
    for (int node : tour) {
        if (node < 0 || node >= p.size() || seen[node])
            throw std::runtime_error("Out-of-range or duplicate node");
        seen[node] = true;
    }
    Cost value = 0;
    for (std::size_t i = 0; i < tour.size(); ++i)
        value += p.cost[tour[i]] + p.distance[tour[i]][tour[(i + 1) % tour.size()]];
    return value;
}
