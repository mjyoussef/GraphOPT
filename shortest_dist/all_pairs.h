#ifndef ALL_PAIRS_H
#define ALL_PAIRS_H

#include <vector>
#include <thread>
#include <mutex>
#include <limits.h>
#include <cstdlib>
#include <time.h>
#include <graphs.h>

std::srand(std::time(NULL));

// ########################## MIN-PLUS MATRIX MULTIPLICATION ################################

// multiplying A and B and storing result in C
struct MatrixInput {
    std::mutex index_mut;
    int index = 0;

    Matrix *A;
    Matrix *B;
    Matrix *C;

    MatrixInput(Matrix *A_, Matrix *B_, Matrix *C_) {
        A = A_;
        B = B_;
        C = C_;
    }
}

void row_product_col(MatrixInput *input) {
    Matrix &A = *(input->A);
    Matrix &B = *(input->B);
    Matrix &C = *(input->C);

    index_mut.lock();
    int i = input->index;
    input->index += 1;
    index_mut.unlock();

    int row = i / C[0].size();
    int col = i % C[0].size();

    if (row >= C.size() || col >= C[row].size()) {
        return;
    }

    int min_sum = INT_MAX;
    long sum;
    for (int k=0; k<C[row].size(); k++) {
        
        // need to account for overflow
        sum = A[row][k] + B[k][col];
        sum = min(INT_MAX, sum);

        min_sum = min(min_sum, (int) sum);
    }

    C[row][col] = min_sum;
}

Matrix &initialize_output(Matrix &A, Matrix &B) {
    Matrix C;
    for (int i=0; i<A.size(); i++) {
        std::vector<int> row;
        for (int j=0; j<B[0].size(); j++) {
            row.push_back(0);
        }
        C.push_back(row);
    }

    return C;
}

Matrix &min_plus_product(Matrix &A, Matrix &B) {

    // one thread for each row
    int num_threads = max(A.size(), B[0].size());
    pthread_t threads[num_threads];

    Matrix C = initialize_output(A, B);

    MatrixInput input(&A, &B, &C);

    for (int i=0; i<num_threads; i++) {
        threads[i] = std::thread(&row_product_col, &input);
    }

    for (int i=0; i<num_threads; i++) {
        threads[i].join();
    }

    return C;
}

Matrix &min_plus_product_naive(Matrix &A, Matrix &B) {
    
    Matrix C = initialize_output(A, B);

    for (int i=0; i<C.size(); i++) {
        for (int j=0; j<C[i].size(); j++) {
            int min_sum = INT_MAX;
            long sum;
            for (int k=0; k<C[i].size(); k++) {
                sum = A[i][k] + A[k][j];
                sum = min(INT_SUM, sum);
                min_sum = min(min_sum, (int) sum);
            }

            C[i][j] = min_sum;
        }
    }

    return C;
}


Matrix &generate_matrix(int r, int c, int max) {
    Matrix m;

    for (int i=0; i<r; i++) {
        std::vector<int> row;
        for (int j=0; j<c; j++) {
            row.push_back(std::rand() % (max + 1));
        }
        m.push_back(row);
    }

    return row;
}

// ###############################################################################


// ######################## SHORTEST DISTANCE ALGORITHMS #########################

Matrix &all_pairs(Matrix &graph) {
    Matrix shortest_paths = graph;
    
    int exp = 1;
    int max_exp = graph.size();

    // log(|v|) matrix multiplication operations
    // means that our algorithm runs in O( |v|^3log(|v|) )
    while (exp < max_exp) {
        shortest_paths = min_plus_product(shortest_paths, shortest_paths);
        exp <<= 1;
    }

    return shortest_paths;
}

Matrix &floyd_warshall(Matrix &graph) {
    Matrix shortest_paths = graph;

    int size = graph.size();

    // runs in O(|v|^3) time
    for (int k=0; k<size; k++) {
        for (int i=0; i<size; i++) {
            for (int j=0; j<size; j++) {
                long sum = graph[i][k] + graph[k][j];

                if (sum < shortest_paths[i][j]) {
                    shortest_paths[i][j] = sum;
                }
            }
        }
    }

    return shortest_paths;
}

// ###############################################################################

#endif