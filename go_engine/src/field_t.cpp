// Author: Patrick Wieschollek <mail@patwie.com>

#include "field_t.h"
#include "board_t.h"
#include "misc.h"

field_t::field_t(int h, int w, const board_t* b) 
: token_(empty), group(nullptr), x_(h), y_(w), played_at(0), board(b) {}

const token_t field_t::token() const {
    return token_;
}

void field_t::token(const token_t tok) {
    token_ = tok;
}

void field_t::pos(int x, int y) {
    x_ = x;
    y_ = y;
}

const int field_t::x() const { return x_;}
const int field_t::y() const { return y_;}

std::pair<int, int> field_t::pos(){
  return {x_, y_};
}


const std::set<std::pair<int, int> > field_t::neighbors(const token_t filter)  const {
    std::set<std::pair<int, int> > n;
    if(valid_pos(x_ - 1))
        if(board->fields[x_ - 1][y_].token() == filter)
          n.insert({x_ - 1, y_});
    if(valid_pos(x_ + 1))
      if(board->fields[x_ + 1][y_].token() == filter)
        n.insert({x_ + 1, y_});
    if(valid_pos(y_ - 1))
      if(board->fields[x_][y_-1].token() == filter)
        n.insert({x_, y_ - 1});
    if(valid_pos(y_ + 1))
      if(board->fields[x_][y_+1].token() == filter)
        n.insert({x_, y_ + 1});
    return n;
}

std::ostream& operator<< (std::ostream& stream, const field_t& stone) {
    if (stone.token() == empty){
        if(   ((stone.x() == 3) && (stone.y() == 3))   ||
              ((stone.x() == 3) && (stone.y() == 9))   ||
              ((stone.x() == 3) && (stone.y() == 15))  ||
              ((stone.x() == 9) && (stone.y() == 3))   ||
              ((stone.x() == 9) && (stone.y() == 9))   ||
              ((stone.x() == 9) && (stone.y() == 15))  ||
              ((stone.x() == 15) && (stone.y() == 3))  ||
              ((stone.x() == 15) && (stone.y() == 9))  ||
              ((stone.x() == 15) && (stone.y() == 15)) 
          ){
            stream  << "+";
        }
        else{
            stream  << ".";
        }
    }
    if (stone.token() == white)
        stream  << "o";
    if (stone.token() == black)
        stream  << "x";
    return stream;
}