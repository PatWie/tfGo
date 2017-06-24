// Author: Patrick Wieschollek <mail@patwie.com>

#ifndef ENGINE_BOARD_T_H
#define ENGINE_BOARD_T_H

#include "misc.h"
#include "token_t.h"
#include "field_t.h"

#include <array>
#include <map>



class board_t {
  public:
    
    /**
     * @brief Create new board representation.
     */
    board_t();
    ~board_t();

    /**
     * @brief Just for convenience.
     * 
     * @param stream board configuration
     * @param b stream
     */
    friend std::ostream& operator<< (std::ostream& stream, const board_t& b);

    /**
     * @brief Set a stone and update groups.
     * 
     * @param x vertical axis (top -> bottom)
     * @param y horizontal axis (left ->right)
     * @param tok color of stone
     * @return 0 if success
     */
    int play(int x, int y, token_t tok);


    int set(int x, int y, token_t tok);

    void update_groups(int x, int y);
    /**
     * @brief Return a group for a given field.
     * @details This creates a new group for the field if the field was without a group.
     * 
     * @param id [description]
     * @return [description]
     */
    group_t* find_or_create_group(int id);

    const token_t opponent(token_t tok) const;

    bool check_move_legal(int x, int y, token_t tok) const ;
    int estimate_captured_stones(int x, int y, token_t color_place, token_t color_count) const ;

    int find_captured_stones(int x, int y, token_t focus);
    void feature_planes(int *planes, token_t self)const;

    /**
     * @brief Create a deep copy of current board configuration.
     * @details This copies all properties (fields, groups, counters)
     * @return new board (do not forget to delete this when not used anymore)
     */
    board_t* clone() const ;


    std::array<std::array<field_t, N>, N> fields;

    std::map<int, group_t*> groups;
    std::map<int, group_t*>::iterator groups_iter;

    int groupid;
    int played_moves = 0;
    float score_black;
    float score_white;

};

#endif