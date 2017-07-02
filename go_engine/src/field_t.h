// Author: Patrick Wieschollek <mail@patwie.com>

#ifndef FIELD_T_H
#define FIELD_T_H

#include <iostream>
#include <memory>
#include <set>
#include <vector>

#include "token_t.h"

class group_t;
class board_t;

class field_t {
  public:

    field_t(int h, int w, const board_t const * board);

    const token_t token() const;

    void token(const token_t tok);
    void pos(int x, int y);
    std::pair<int, int> pos();

    const int x() const;
    const int y() const;

    friend std::ostream& operator<< (std::ostream& stream, const field_t& stone);

    const std::set<std::pair<int, int> > neighbors(const token_t filter)  const;


    group_t* group;
    int played_at;


  private:
    int x_;
    int y_;
    token_t token_;
    const board_t const * board;
};

#endif