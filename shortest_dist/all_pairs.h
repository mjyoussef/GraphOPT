#ifndef ALL_PAIRS_H
#define ALL_PAIRS_H

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <limits.h>
#include <cstdlib>
#include <time.h>
#include <graphs.h>

std::srand(std::time(NULL));

// ########################## MIN-PLUS MATRIX MULTIPLICATION ################################

struct MatrixInput {
    int i;
    int j;
    int *visited;

    std::mutex *c_m;
    std::mutex *v_m;
    std::mutex *lk_m,

    std::condition_variable *this_var;
    std::condition_variable *parent_var;
    
    Matrix *A;
    Matrix *B;
    Matrix *C;

    MatrixInput(int i_, int j_, int *visited_, 
                std::mutex *c_m_, std::mutex *v_m_, std::mutex *lk_m_,
                std::condition_variable *this_var_, std::condition_variable *parent_var_,
                Matrix *A_, Matrix *B_, Matrix *C_) {
        i = i_;
        j = j_;
        visited = visited_;
        c_m = c_m_;
        v_m = v_m_;
        lk_m = lk_m_;

        this_var = this_var_;
        parent_var = parent_var_;

        A = A_;
        B = B_;
        C = C_;
    }
}

void entry_by_row(MatrixInput *input) {

    for (;;) {
        std::unique_lock<std::mutex> lk(*(input->lk_m));

        Matrix &A = *(input->A);

        while (visited >= A[0].size()) {
            // sleep
            (*(input->this_var)).wait(lk);
        }

        // task
        int i = input->i;
        int j = input->j;

        Matrix &A = *(input->A);
        Matrix &B = *(input->B);
        Matrix &C = *(input->C);

        int a = A[i][j];
        long sum = 0;

        for (int k=0; k<B[j].size(); k++) {
            sum = a + B[j][k];
            sum = min(INT_MAX, sum);

            input->c_m.lock();
            C[i][k] = min(C[i][k], sum);
            input->c_m.unlock();
        }

        (*(input->v_m)).lock();
        int visited = *(input->visited);
        visited += 1;
        (*(input->v_m)).unlock();

        if (visited >= A[0].size()) {
            (*(input->parent_var)).notify_one();
        }

        // this will put the thread to sleep
        go = false;
    }
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

/*
In the parent thread, we have a while loop that continues while visited is 
less than the number of entries in a row of A. It puts all of the threads to work,
and then it goes to sleep. When it wakes up, it moves onto the next row.
*/

Matrix &min_plus_product(Matrix &A, Matrix &B) {
    Matrix &C = initialize_output(A, B);
    std::mutex c_m;

    int visited = 0;
    std::mutex v_m;

    // condition variable for waking up *this* thread
    std::condition_variable cv;
    std::mutex cv_m;


    std::vector<thread> threads;
    std::vector<MatrixInput> inputs;

    int m = A.size();
    int n = A[0].size();

    for (int i=0; i<m; i++) {
        std::unique_lock<std::mutex> lk(cv_m);

        // dispatch worker threads
        for (int j=0; j<n; j++) {
            visited = 0;

            if (i == 0) {
                std::mutex lk_m;
                std::condition_variable wv;

                MatrixInput input(i, j, &visited, *c_m, *v_m, *lk_m, *wv, *cv, A, B, C);
                threads.push(std::thread(&min_plus_product, *input));
                inputs.push(input);
            } else {
                MatrixInput &input = inputs[j];
                input.i = i;
                input.j = j;

                // wake up the worker thread
                (*(input.wv)).notify_one();
            }
        }

        while (visited < n) {
            // sleep
            cv.wait(lk);
        }
    }

    // delete all of the child threads
    for (int i=0; i<threads.size(); i++) {
        threads[i].terminate();
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

// ##################################################################################


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

Matrix &floyd_warshall_naive(Matrix &graph) {
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

Matrix &floyd_warshall_concurrent(Matrix &graph) {
    return floyd_warshall_naive(graph);
}

// ###############################################################################

#endif