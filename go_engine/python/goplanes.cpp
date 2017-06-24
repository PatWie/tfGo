// Author: Patrick Wieschollek <mail@patwie.com>

#include "../src/token_t.h"
#include "../src/field_t.h"
#include "../src/group_t.h"
#include "../src/board_t.h"
#include "../src/sgfbin.h"
#include "goplanes.h"


int play_game(SGFbin *Game, int* data, int moves) {

    token_t opponent_player, current_player;

    const int evaluate_until = std::min(moves, (int)Game->num_actions() - 1);

    board_t b;

    int x = 0, y = 0;
    bool is_white = true, is_move = true, is_pass = true;

    for (int i = 0; i < evaluate_until; ++i) {
        // parse move
        Game->parse(i, &x, &y, &is_white, &is_move, &is_pass);

        current_player = is_white ? white : black;
        opponent_player = is_white ? black : white;


        if (!is_pass) {
            if (is_move) {
                b.play(x, y, current_player);
            } else {
                b.set(x, y, current_player);
            }
        }
    }


    b.feature_planes(data, opponent_player);

    Game->parse(evaluate_until, &x, &y, &is_white, &is_move, &is_pass);
    const int next_move = 19 * x + y;

    return next_move;
}


/**
 * @brief return board configuration and next move given a file
 * @details SWIG-Python-binding
 *
 * @param str path to file
 * @param strlen length of that path name
 * @param data pointer of features (length 19*19*14) for 14 feature planes
 * @param len currently 19*19*4
 * @param moves number of moves in match to the current position
 * @return next move on board
 */
int planes_from_file(char *str, int strlen, int* data, int len, int moves) {
    // load game
    std::string path = std::string(str);
    SGFbin Game(path);
    return play_game(&Game, data, moves);
}




/**
 * @brief return board configuration and next move given a file
 * @details SWIG-Python-binding
 *
 * paper: "... Each position consisted of a raw board description s
 *         and the move a selected by the human. ..."
 *
 * @param bytes buffer of moves (each move 2 bytes)
 * @param strlen length of buffer
 * @param data pointer of features (length 19*19*14) for 14 feature planes (this is "s")
 * @param len currently 19*19*4
 * @param moves number of moves in match to the current position
 * @return next move on board (this is "a")
 */
int planes_from_bytes(char *bytes, int byteslen, int* data, int len, int moves) {
    // the SGFbin parser
    SGFbin Game((unsigned char*) bytes, byteslen);
    return play_game(&Game, data, moves);
}


/**
 * @brief return board configuration and next move given a board position
 * @details SWIG-Python-binding
 */
void planes_from_position(int* bwhite, int wm, int wn, 
                          int* bblack, int bm, int bn, 
                          int* data, int len, 
                          int is_white) {

    board_t b;

    for (int x = 0; x < 19; ++x)
    {
        for (int y = 0; y < 19; ++y)
        {
            if(bwhite[19*x + y] == 1){
                b.play(x, y, white);
            }
            if(bblack[19*x + y] == 1){
                b.play(x, y, black);
            }
        }
    }

    // std::cout << "----------------- TFGO --------------------" << std::endl;
    // std::cout << b << std::endl;
    // std::cout << "-------------------------------------------" << std::endl;
      

    token_t tok = (is_white == 1) ? white : black;
    b.feature_planes(data, tok);
}

