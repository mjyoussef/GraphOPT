#include <iostream>
#include "graph_gen.h"
#include <limits.h>

using namespace std;

#include <time.h>

int main(int argc, char** argv) {
    // ############ TESTS #############
    
    int edges = 100;
    vector< vector<int> > graph = generate_graph(20, &edges, 1, 1);

    int count = 0;
    for (int i=0; i<graph.size(); i++) {
        for (int j=0; j<graph.size(); j++) {
            if (graph[i][j] == INT_MAX) {
                cout << "__";
            } else {
                count++;
                cout << "_";
                cout << graph[i][j];
            }
        }
        cout << endl;
    }

    cout << count << endl;


    // ##################################
}