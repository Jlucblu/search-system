#include "read_input_functions.h"


// ввод текста
std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

// кол-во получаемых строк
int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}