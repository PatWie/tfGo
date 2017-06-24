// Author: Patrick Wieschollek <mail@patwie.com>

#include <fstream>

#include "sgfbin.h"

SGFbin::SGFbin(std::string path) {
    moves_ = read_moves(path.c_str());
}


SGFbin::SGFbin(unsigned char* buffer, int len) {
    moves_.assign(buffer, buffer + len);
}


void SGFbin::parse(unsigned int step,
                   int *x, int *y, bool *is_white,
                   bool *is_move, bool *is_pass) {
    parse((unsigned char)moves_[2 * step],(unsigned char)moves_[2 * step + 1],
           x, y, 
           is_white, is_move, is_pass);
}


void SGFbin::parse(unsigned char m1, unsigned char m2,
                   int *x, int *y, 
                   bool *is_white, bool *is_move, bool *is_pass) {

    const int byte1 = (int) m1;
    const int byte2 = (int) m2;
    int value = (byte1 << 8) + byte2;

    *is_white = value & 1024;
    *is_move = value & 2048;
    *is_pass = value & 4096;

    if (*is_white)
        value -= 1024;
    if (*is_move)
        value -= 2048;
    if (*is_pass)
        value -= 4096;

    *x = (byte2 % 32);
    *y = (value - *x) / 32;
}


const unsigned int SGFbin::num_actions() const {
    return moves_.size() / 2;
}


void SGFbin::ascii() {
    int x = 0, y = 0;
    bool is_move = true, is_white = true, is_pass = true;
    std::cout << "(;" << "GM[1]" << std::endl;
    std::cout << "SZ[19]" << std::endl;


    for (unsigned int i = 0; i < moves_.size(); i += 2) {
        parse((unsigned char)moves_[i], (unsigned char)moves_[i + 1],
               &x, &y, 
               &is_white, &is_move, &is_pass);
        if (is_move) {
            if (is_white)
                std::cout << ";W[";
            else
                std::cout << ";B[";
            std::cout << (char)('a' + x) << (char)('a' + y) << "]" << std::endl;
        } else {
            if (is_white)
                std::cout << "AW";
            else
                std::cout << "AB";
            std::cout << "[" << (char)('a' + x) << (char)('a' + y) << "]" << std::endl;
        }
    }
    std::cout << ")" << std::endl;
}


std::vector<char> SGFbin::read_moves(char const* filename) {
    std::ifstream ifs(filename, std::ios::binary | std::ios::ate | std::ios::in);
    std::ifstream::pos_type pos = ifs.tellg();

    std::vector<char>  result(pos);
    ifs.seekg(0, std::ios::beg);
    ifs.read(result.data(), pos);

    return result;
}
