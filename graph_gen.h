#ifndef GRAPH_GEN_H
#define GRAPH_GEN_H

#include <limits.h>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <time.h>
#include <cstdlib>

void shuffle(std::vector<int> &vect) {
    int j;
    for (int i=0; i<vect.size(); i++) {
        j = std::rand() % vect.size();
        std::swap(vect[i], vect[j]);
    }
}

void generate_graph_helper(int v, std::vector< std::vector<int> > &graph, int *edges, int directed, int weighted) {

    std::vector<int> remaining;
    std::vector<int> neighbors = graph[v];
    for (int i=0; i<graph.size(); i++) {
        if (neighbors[i] == INT_MAX) {
            remaining.push_back(i);
        }
    }

    shuffle(remaining);
    int num_options = std::rand() % std::min((int) *edges, (int) remaining.size());
    for (int j=0; j<num_options; j++) {
        if (*edges <= 0) {
            return;
        }
        int weight = weighted ? ((int) std::rand() % 51) : 1;
        graph[v][remaining[j]] = weight;
        if (!directed) {
            graph[remaining[j]][v] = weight;
        }

        *edges = *edges - 1;
        generate_graph_helper(remaining[j], graph, edges, directed, weighted);
    }
}

std::vector< std::vector<int> > generate_graph(int size, int *edges, int directed, int weighted) {
    std::srand(std::time(NULL));
    std::vector< std::vector<int> > graph;

    for (int i=0; i<size; i++) {
        std::vector<int> neighbors;
        for (int j=0; j<size; j++) {
            neighbors.push_back(INT_MAX);
        }

        graph.push_back(neighbors);
    }

    int v = std::rand() % size;

    generate_graph_helper(v, graph, edges, directed, weighted);
    return graph;
}

#endif