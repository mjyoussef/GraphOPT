#ifndef ALL_PAIRS_H
#define ALL_PAIRS_H

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <limits.h>
#include <cstdlib>
#include <time.h>
#include <graphs.h>

std::srand(std::time(NULL));

// ########################## MIN-PLUS MATRIX MULTIPLICATION ################################

// store mutexes and neccesary condition variables in here
class CondVarContainer {
    public:
        int i;
        int j;

        std::mutex worker_mutex;

        int sleep = 0;

        /*
        track the number of threads that have finished so that the 
        last thread to complete its task can wake up the parent thread
        */
        int *visited;
        std::mutex *visited_m;

        Matrix *C; //output
        std::mutex *c_m;

        Matrix *A;
        Matrix *B;

        std::condition_variable *this_var;
        std::condition_variable *parent_var;

        CondVarContainer(int i_, int j_, int *visited, std::mutex *visited_m_, 
                        Matrix *C_, Matrix *A_, Matrix *B_,
                        std::condition_variable *parent_var) {
            i = i_;
            j = j_;
            visited = visited_;
            visited_m = visited_m_;
            C = C_;
            A = A_;
            B = B_;
            parent_var = parent_var_;
        }

        void lock_worker() {
            std::unique_lock< std::mutex > lock(worker_mutex);
            (*this_var).wait(lock);
        }

        void wake_up_worker() {
            sleep = 0;
            (*this_var).notify_one();
        }

        void wake_up_dispatcher() {
            (*parent_var).notify_one();
        }

        void lock_c() {
            (*c_m).lock();
        }

        void unlock_c() {
            (*c_m).unlock();
        }
}

void entry_by_row(CondVarContainer *input) {
    for (;;) {

        int sleep = *(input->sleep);
        while (sleep) {
            // sleep
            input->lock_worker();
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

            input->lock_c();
            C[i][k] = min(C[i][k], sum);
            input->unlock_c();
        }

        (*(input->visited_m)).lock()
        *(input->visited) += 1;
        (*(input->visited_m)).unlock()
        
        Matrix &A = *(input->A);

        if (*(input->visited) >= A[0].size()) {
            input->wake_up_dispatcher();
        }

        sleep = 1; // task is finished so it can sleep
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
    std::vector<CondVarContainer> inputs;

    int m = A.size();
    int n = A[0].size();

    for (int i=0; i<m; i++) {
        std::unique_lock<std::mutex> lk(cv_m);

        // dispatch worker threads
        for (int j=0; j<n; j++) {
            visited = 0;

            if (i == 0) {
                CondVarContainer input(i, j, &visited, &v_m, &C, &A, &B, &cv);

                threads.push(std::thread(&min_plus_product, &input));
                inputs.push(input);
            } else {
                CondVarContainer &input = inputs[j];
                input.i = i;
                input.j = j;
                input->wake_up_worker();
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


void update_path(CondVarContainer *input, int k) {
    for (;;) {

        int sleep = *(input->sleep);
        while (sleep) {
            // sleep
            input->lock_worker();
        }

        // task
        int i = input->i;
        int j = input->j;

        Matrix &C = *(input->C);

        long sum = C[i][k] + C[k][j];
        if (sum < C[i][j]) {
            C[i][j] = sum;
        }

        (*(input->visited_m)).lock()
        *(input->visited) += 1;
        (*(input->visited_m)).unlock()
        
        Matrix &A = *(input->A);

        if (*(input->visited) >= C[0].size()) {
            input->wake_up_dispatcher();
        }

        sleep = 1; // task is finished so it can sleep
    }
}

Matrix &floyd_warshall_concurrent(Matrix &graph) {

    Matrix shortest_paths = graph;

    int size = graph.size();

    std::vector<std::thread> threads;
    std::vector<CondVarContainer> inputs;

    int visited = 0;
    std::mutex *visited_m;

    std::condition_variable parent_var;
    std::mutex parent_mutex;

    for (int k=0; k<size; k++) {
        for (int i=0; i<size; i++) {

            unique_lock<std::mutex> lock(parent_mutex);

            for (int j=0; j<size; j++) {
                visited = 0;

                if (i == 0) {
                    CondVarContainer input(i, j, &visited, &visited_m, &shortest_paths, NULL, NULL, &parent_var);

                    threads.push_back(thread(&update_path, &input));
                    inputs.push_back(input);
                } else {
                    CondVarContainer &input = inputs[j];

                    input.i = i;
                    input.j = j;

                    input->wake_up_worker();
                }
            }

            while (visited < n) {
                parent_var.wait(lock);
            }
        }
    }

    for (int i=0; i<threads.size(); i++) {
        threads[i].terminate();
    }
    
    return shortest_paths;
}

// ###############################################################################

#endif