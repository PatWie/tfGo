#include <iostream>

#include "../src/token_t.h"
#include "../src/field_t.h"
#include "../src/group_t.h"
#include "../src/board_t.h"
#include "../src/sgfbin.h"
#include "board_t.h"



void test_case001(){
    /*
    st, moves = parseboard.parse("d b c . . . .|"
                                 "B W a . . . .|"
                                 ". B . . . . .|"
                                 ". . . . . . .|"
                                 ". . . . . . .|"
                                 ". . . . . W .|")
            st.current_player = BLACK

            # 'a' should catch white in a ladder, but not 'b'
            self.assertTrue(st.is_ladder_capture(moves['a']))
            self.assertFalse(st.is_ladder_capture(moves['b']))

            # 'b' should not be an escape move for white after 'a'
            st.do_move(moves['a'])
            self.assertFalse(st.is_ladder_escape(moves['b']))

            # W at 'b', check 'c' and 'd'
            st.do_move(moves['b'])
            self.assertTrue(st.is_ladder_capture(moves['c']))
            self.assertFalse(st.is_ladder_capture(moves['d']))  # self-atari
    */

    board_t b;

    b.play({1, 0}, black);
    b.play({1, 1}, white);
    b.play({2, 1}, black);

    std::cout << "starting in" <<std::endl << b << std::endl;
      

    b.current_player = black;
    std::cout << b.is_forced_ladder_capture({1, 2}, black, 0) << "vs. 1"<< std::endl;
    std::cout << b.is_forced_ladder_capture({0, 1}, black, 20) << "vs. 0" << std::endl;

      

    std::cout << b << std::endl;
      

}


void test_case002(){
    /*
    st, moves = parseboard.parse("d b c . . . .|"
                                 "B W a . . . .|"
                                 ". B . . . . .|"
                                 ". . . . . . .|"
                                 ". . . . . . .|"
                                 ". . . . . W .|")
            st.current_player = BLACK

            # 'a' should catch white in a ladder, but not 'b'
            self.assertTrue(st.is_ladder_capture(moves['a']))
            self.assertFalse(st.is_ladder_capture(moves['b']))

            # 'b' should not be an escape move for white after 'a'
            st.do_move(moves['a'])
            self.assertFalse(st.is_ladder_escape(moves['b']))

            # W at 'b', check 'c' and 'd'
            st.do_move(moves['b'])
            self.assertTrue(st.is_ladder_capture(moves['c']))
            self.assertFalse(st.is_ladder_capture(moves['d']))  # self-atari
    */

    board_t b;

    b.play({0, 2}, black);
    b.play({0, 1}, white);
    b.play({1, 1}, black);
    b.play({1, 3}, black);
    b.play({2, 3}, black);
    b.play({1, 2}, white);
    b.play({2, 2}, white);
    b.play({4, 1}, white);

    std::cout << "starting in" <<std::endl << b << std::endl;
      

    b.current_player = black;
    std::cout << b.is_forced_ladder_capture({3, 2}, black) << "vs. 0"<< std::endl;
    // std::cout << b.is_forced_ladder_capture({0, 1}, black, 20) << "vs. 0" << std::endl;

      

    std::cout << b << std::endl;
      

}

int main(int argc, char const *argv[]) {
    test_case002();
}

int main2(int argc, char const *argv[]) {
    // test group capturing

    std::string path = std::string("../../data/ladder_capture.sgfbin");
    SGFbin Game(path);
    

    board_t b;

    b.play({4, 13}, black);
    b.play({3, 12}, black);
    b.play({3, 11}, black);
    b.play({4, 12}, white);

    // token_t opponent_player, current_player;
    // int x = 0, y = 0;
    // bool is_white = true, is_move = true, is_pass = true;

    // for (int i =0; i < 4; i++) {
    //     Game.parse(i, &x, &y, &is_white, &is_move, &is_pass);
    //     Game.debug(i);
    //     current_player = is_white ? white : black;
    //     b.play(y, x, current_player);
    // }

    // b.play(3, 1, current_player);

    std::cout << b << std::endl;

    std::cout << b.is_forced_ladder_capture({5, 12}, black, 2) << std::endl;
    // std::cout << b.is_ladder_capture(4, 12, black, black, 2) << std::endl;

    // for(auto &&n : b.fields[4][12].group->neighbors(empty)){
    //     b.play(n.first, n.second, white);
    // }
    std::cout << b << std::endl;



    return 0;
}