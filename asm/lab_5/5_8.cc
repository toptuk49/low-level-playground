#include <iostream>
#include <vector>

using namespace std;

void replaceMatrixRow(vector<vector<int>>& matrix, int row) {
    int cols = matrix[0].size();
    for (int j = 0; j < cols; j++) {
        matrix[row][j] = -1;
    }
}

void printMatrix(const vector<vector<int>>& matrix) {
    int rows = matrix.size();
    int cols = matrix[0].size();

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cout << matrix[i][j] << " ";
        }
        cout << endl;
    }
}

void replaceRowWithAssembler(vector<vector<int>>& matrix, int row) {
    int rows = matrix.size();
    int cols = matrix[0].size();
    int* matrix = &matrix[0][0];

    asm (
        // Вычисляем указатель на первый элемент нужной строки
        "imull %%rdx, %%rsi\n"
        "shll $2, %%rsi\n"
        "addq %%rsi, %%rdi\n"

        "movl %1, %%ecx\n" // счетчик цикла

        "loop_start:\n"
            "movl $-1, (%%rdi)\n" // matrix[row][j] = -1
            "addq $4, %%rdi\n" // переход к следующему элементу строки
            "loop loop_start\n"
        : // Нет выходных операндов
        : "i" (cols), "D" (matrix), "S" (row), "d" (cols)
        : "memory", "%rax", "%rcx", "%rsi", "%rdi"
    );
}


int main() {
    vector<vector<int>> matrix = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };

    cout << "Исходная матрица:" << endl;
    printMatrix(matrix);

    replaceMatrixRow(matrix, 1);

    cout << "\nМатрица после замены строки 1 на -1:" << endl;
    printMatrix(matrix);

    replaceRowWithAssembler(matrix, 0);

    cout << "\nМатрица после замены столбца 0 на -1 с помощью ассемблера:" << endl;
    printMatrix(matrix);
    
    return 0;
}