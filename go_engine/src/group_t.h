// Author: Patrick Wieschollek <mail@patwie.com>

#ifndef ENGINE_GROUP_T_H
#define ENGINE_GROUP_T_H

#include <vector>
#include <set>
#include <memory>

#include "token_t.h"

class field_t;
class board_t;

class group_t {
  public:
    group_t(int groupid, const board_t const * board);
    ~group_t();

    void add(field_t *s);

    const unsigned int size() const;

    int kill();

    void merge(group_t* other);

    // count liberties (see below)
    // TODO: cache result (key should be iteration in game)
    int liberties()  const;
    const std::set<std::pair<int, int> > neighbors(const token_t filter)  const;

    // collection of pointers to stones
    std::vector<field_t *> stones;
    const board_t const * board;
    int id;
};

#endif