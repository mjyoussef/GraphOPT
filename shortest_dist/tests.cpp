#include <cstdlib>
#include <time.h>

#define T_PARAM 20
#define MAX_ROW 51
#define MAX_COL 51

using namespace std;

void min_plus_test() {
    long avg_diff = 0;
    long error_count = 0;

    for (int i=0; i<T_PARAM; i++) {
        int r1 = rand() % MAX_ROW;
        int r2 = rand() % MAX_COL;

        int r3 = r2;
        int r4 = rand() % MAX_COL;

        Matrix A = generate_matrix(r1, r2);
        Matrix B = generate_matrix(r3, r4);

        Matrix C_1;
        Matrix C_2;

        clock_t s = clock();
        C_1 = min_plus_product(A, B);
        clock_t e = clock();

        clock_t s2 = clock();
        C_2 = min_plus_product_naive(A, B);
        clock_t e2 = clock();

        int error = 0;
        for (int i=0; i<r1; i++) {
            for (int j=0; j<r4; j++) {
                if (C_1[i][j] != C_2[i][j]) {
                    error = 1;
                    break;
                }
            }

            if (error) {
                error = 0;
                error_count += 1;
                break;
            }
        }

        avg_diff += (e2 - s2 - (e - s));
    }

    avg_diff /= T_PARAM;
    cout << "average performance difference: " << avg_diff << endl;
    cout << "number of errors: " << error_count << endl;
}

void all_pairs_test() {
    //
}

int main() {
    min_plus_test();
    all_pairs_test();
}