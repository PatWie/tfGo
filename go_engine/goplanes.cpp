// Author: Patrick Wieschollek <mail@patwie.com>
/*
TODO: 
- deep-copy (std::map(groups) + read)
- test capturing of stones
- find illegal moves
- cache liberties (board.state_dirty flag + board.notify(each group) + b.each_group(compute_liberties for cache))
*/

#include <iostream>
#include <vector>
#include <utility>
#include <array>
#include <bitset>
#include <fstream>
#include <string>
#include <iomanip>

#define map2line(x,y) (((x) * 19 + (y)))
#define valid_pos(x) (((x>=0) && (x < N)))

const int N = 19;

enum token_t { empty, white, black };
class field_t;
class group_t;
class board_t;


class field_t {
  public:

    field_t() : token_(empty), group(nullptr), x_(-1), y_(-1) {}

    const token_t token() const {
        return token_;
    }

    void token(token_t tok) {
        token_ = tok;
    }

    void pos(int x, int y) {
        x_ = x;
        y_ = y;
    }

    const int x() const { return x_;}
    const int y() const { return y_;}

    friend std::ostream& operator<< (std::ostream& stream, const field_t& stone) {
        if (stone.token() == empty)
            stream  << ".";
        if (stone.token() == white)
            stream  << "o";
        if (stone.token() == black)
            stream  << "x";
        return stream;
    }

    group_t *group;

  private:
    int x_;
    int y_;
    token_t token_;
};

class group_t {
  public:
    group_t(field_t *anchor, int groupid) {
        stones.push_back(anchor);
        id = groupid;
    }
    ~group_t() {}

    void add(field_t *s) {
        // add stone to group
        // check (before calling) if stone "s" belongs to opponent group!
        s->group = this;
        stones.push_back(s);
    }

    const unsigned int size() const {
        return stones.size();
    }

    int kill() {
        // kill entire group (remove stones from board, destroy group, return score)
        int score = stones.size();
        for (field_t * s : stones) {
            s->token(empty);
            s->group = nullptr;
        }
        delete this;
        return score;
    }

    void merge(group_t* other) {
        // never merge a group with itself
        if (other->id == id)
            return;

        // merge two groups (current group should own other stones)
        for (field_t * s : other->stones) {
            s->group = this;
            stones.push_back(s);
        }
        delete other;
    }

    // count liberties (see below)
    // TODO: cache result (key should be iteration in game)
    int liberties(board_t *b);

    // collection of pointers to stones
    std::vector<field_t *> stones;
    int id;
};


class board_t {
  public:
    int groupid;


    board_t() : turn(white), score_black(0.f), score_white(0.f) {
        // tell each field its coordinate
        for (int h = 0; h < N; ++h)
            for (int w = 0; w < N; ++w)
                fields[h][w].pos(h, w);
        groupid = 0;
    }

    ~board_t() {}

    // prints
    // ------------------------------------------------------------------

    void print() {
        for (int h = 0; h < N; ++h) {
            for (int w = 0; w < N; ++w)
                std::cout << fields[h][w] << " ";
            std::cout  << std::endl;
        }
    }

    void debug_groups() {
        for (int h = 0; h < N; ++h) {
            for (int w = 0; w < N; ++w) {
                if (fields[h][w].group == nullptr)
                    std::cout << "0" << " ";
                else
                    std::cout << fields[h][w].group->size() << " ";
            }
            std::cout  << std::endl;
        }
    }

    void debug_liberties() {
        for (int h = 0; h < N; ++h) {
            for (int w = 0; w < N; ++w) {
                if (fields[h][w].group == nullptr)
                    std::cout << "0" << " ";
                else {

                    if (fields[h][w].token() != empty)
                        std::cout << fields[h][w].group->liberties(this) << " ";
                    else
                        std::cout << "!";

                }
            }
            std::cout  << std::endl;
        }
    }

    // game logic
    // ------------------------------------------------------------------

    int play(int x, int y, token_t tok) {
        turn = tok;
        // set token
        int r = set(x, y, turn);

        // try, catch ?
        if (r == -1)
            return -1;

        return 0;
    }

    void pass() {
        turn = opponent();
    }

    int set(int x, int y, token_t tok) {
        // is field empty ?

        if (!valid_pos(x) || !valid_pos(y)) {
            std::cout << "field is not valid" << std::endl;
            return -1;
        }

        if (fields[x][y].token() != empty) {
            std::cout << "field was not empty" << std::endl;
            return -1;
        }

        // place token to field
        fields[x][y].token(tok);

        // update group structures
        update_groups(x, y);

        // does this move captures some stones?
        int taken = find_captured_stones(x, y);

        // TODO: check suicidal move, i.e. a move which kills its own group
        // TODO: check ko, i.e. the same position should not appear on the board again (hashing?)

        // maintain scores
        if (taken > 0) {
            if (turn == white)
                score_white += taken;
            if (turn == black)
                score_black += taken;
        }
        return 0;
    }

    void update_groups(int x, int y) {

        token_t current = fields[x][y].token();
        field_t& current_stone = fields[x][y];

        if (valid_pos(x - 1)) {
            field_t& other_stone = fields[x - 1][y];
            if (other_stone.token() == current) {
                group_t *other_group = other_stone.group;
                if (current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);
            }
        }

        if (valid_pos(y - 1)) {
            field_t& other_stone = fields[x][y - 1];
            if (other_stone.token() == current) {
                group_t *other_group = other_stone.group;
                if (current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);
            }
        }

        if (valid_pos(x + 1)) {
            field_t& other_stone = fields[x + 1][y];
            if (other_stone.token() == current) {
                group_t *other_group = other_stone.group;
                if (current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);
            }
        }

        if (valid_pos(y + 1)) {
            field_t& other_stone = fields[x][y + 1];
            if (other_stone.token() == current) {
                group_t *other_group = other_stone.group;
                if (current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);
            }
        }

        // still single stone ? --> create new group
        if (current_stone.group == nullptr) {
            current_stone.group = new group_t(&current_stone, groupid++);
        }

    }

    const token_t opponent() const {
        return (turn == white) ? black : white;
    }

    int find_captured_stones(int x, int y) {
        int scores = 0;

        // does any opponent neighbor-group has no liberties?
        if (valid_pos(x - 1)) {
            field_t& other_stone = fields[x - 1][y];
            if (other_stone.token() == opponent())
                if (other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        if (valid_pos(y - 1)) {
            field_t& other_stone = fields[x][y - 1];
            if (other_stone.token() == opponent())
                if (other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        if (valid_pos(x + 1)) {
            field_t& other_stone = fields[x + 1][y];
            if (other_stone.token() == opponent())
                if (other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        if (valid_pos(y + 1)) {
            field_t& other_stone = fields[x][y + 1];
            if (other_stone.token() == opponent())
                if (other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        return scores;
    }

    void feature_planes(int *planes) {
        // see https://gogameguru.com/i/2016/03/deepmind-mastering-go.pdf (Table 2)
        const int NN = 19 * 19;

        for (int h = 0; h < N; ++h) {
            for (int w = 0; w < N; ++w) {
                // Stone colour : 3 : Player stone / opponent stone / empty
                if (fields[h][w].token() == turn)
                    planes[0 * NN + map2line(h, w)] = 1;
                else if (fields[h][w].token() == opponent())
                    planes[1 * NN + map2line(h, w)] = 1;
                else
                    planes[2 * NN + map2line(h, w)] = 1;

                // Ones : 1 : A constant plane filled with 1
                planes[3 * NN + map2line(h, w)] = 1;

                // Liberties :8: Number of liberties (empty adjacent points)
                if (fields[h][w].token() != empty) {

                    const int num_liberties = fields[h][w].group->liberties(this);

                    if (num_liberties == 1)
                        planes[4 * NN + map2line(h, w)] = 1;
                    else if (num_liberties == 2)
                        planes[5 * NN + map2line(h, w)] = 1;
                    else if (num_liberties == 3)
                        planes[6 * NN + map2line(h, w)] = 1;
                    else if (num_liberties == 4)
                        planes[7 * NN + map2line(h, w)] = 1;
                    else if (num_liberties == 5)
                        planes[8 * NN + map2line(h, w)] = 1;
                    else if (num_liberties == 6)
                        planes[9 * NN + map2line(h, w)] = 1;
                    else if (num_liberties == 7)
                        planes[10 * NN + map2line(h, w)] = 1;
                    else
                        planes[11 * NN + map2line(h, w)] = 1;
                }

                // TODO next planes (isn't this the ideal case for ConvLSTM?)
                // this requires something like undo and history or deep-copy
                // ...

                // Zeros : 1 : A constant plane filled with 0
                planes[12 * NN + map2line(h, w)] = 0;

                // Player color :1: Whether current player is black
                // TODO_ can't we just normalize the input, such that black is always the current player ?
                const int value = (turn == black) ? 1 : 0;
                planes[13 * NN + map2line(h, w)] = value;

            }
        }
    }

    std::array<std::array<field_t, N>, N> fields;
    token_t turn;

    float score_black;
    float score_white;

};

// to avoid circular dependency
int group_t::liberties(board_t *b) {
    // TODO: this really needs a caching!!!
    // local memory
    std::bitset<19 * 19> already_processed(0);

    for (field_t * s : stones) {

        const int x = s->x();
        const int y = s->y();

        if (valid_pos(y - 1))
            if (!already_processed[map2line(x, y - 1)])
                if (b->fields[x][y - 1].token() == empty)
                    already_processed[map2line(x, y - 1)] = 1;

        if (valid_pos(x - 1))
            if (!already_processed[map2line(x - 1, y)])
                if (b->fields[x - 1][y].token() == empty)
                    already_processed[map2line(x - 1, y)] = 1;

        if (valid_pos(y + 1))
            if (!already_processed[map2line(x, y + 1)])
                if (b->fields[x][y + 1].token() == empty)
                    already_processed[map2line(x, y + 1)] = 1;

        if (valid_pos(x + 1))
            if (!already_processed[map2line(x + 1, y)])
                if (b->fields[x + 1][y].token() == empty)
                    already_processed[map2line(x + 1, y)] = 1;

    }
    return already_processed.count();
}

/**
 * @brief Reader for binary SGF files
 * @details It seems to be easier to load a binary version of GO-specific SGF files
 *          which are converted from ASCII SGF files by a python script.
 */
class SGFbin {
  public:
    /**
     * @brief load binary SGF file given filename
     * 
     * @param path path to binary file
     */
    SGFbin(std::string path) {
        moves_ = read_moves(path.c_str());
    }

    /**
     * @brief load binary SGF from a buffer
     * @details This is mostly for SWIG+Python bindings, where python serves np.arrays(np.uint8)
     * 
     * @param char buffer containing the moves from binary SGFfile
     * @param len length of buffer
     */
    SGFbin(unsigned char* buffer, int len) {
        moves_.assign(buffer, buffer + len);
    }

    /**
     * @brief read move from SGFbin description
     * 
     * @param x position [1-19]
     * @param y position [A-S]
     * @param is_white move was from player "white"
     * @param is_move move was really "playing a move" and not a "set stone" (see SGF format for details)
     * @param is_pass player passed this move
     */
    void move(unsigned int step,
              int *x, int *y, bool *is_white, bool *is_move, bool *is_pass) {
        decode((unsigned char)moves_[2 * step],
               (unsigned char)moves_[2 * step + 1],
               x, y, is_white, is_move, is_pass);
    }

    /**
     * @brief decodes a move from 2 bytes
     * @details Each move is represented by 2 bytes which can be divided into
     *    format:       ---pmcyy|yyyxxxxx (low bit)
     *    p: is passed ? [1:yes, 0:no]
     *    m: is move ? [1:yes, 0:no] (sgf supports 'set' as well)
     *    c: is white ? [1:yes, 0:no] (0 means black ;-) )
     *    y: encoded row (1-19)
     *    x: encoded column (a-s)
     *    -: free bits (maybe we can store something useful here, current sun position or illuminati-code?)
     */
    void decode(unsigned char m1, unsigned char m2,
                int *x, int *y, bool *is_white, bool *is_move, bool *is_pass) {

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

    /**
     * @brief number of total actions for game (including "set")
     */
    const unsigned int num_actions() const {
        return moves_.size() / 2;
    }

    /**
     * @brief reconstruct plain ASCII version of SGF file (only important things)
     */
    void ascii() {
        int x = 0, y = 0;
        bool is_move = true, is_white = true, is_pass = true;
        std::cout << "(;" << "GM[1]" << std::endl;
        std::cout << "SZ[19]" << std::endl;


        for (int i = 0; i < moves_.size(); i += 2) {
            decode((unsigned char)moves_[i],
                   (unsigned char)moves_[i + 1],
                   &x, &y, &is_white, &is_move, &is_pass);
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

  private:
    /**
     * @brief reading from file o buffer
     * @param filename path to SGFbin file
     */
    std::vector<char> read_moves(char const* filename) {
        std::ifstream ifs(filename, std::ios::binary | std::ios::ate | std::ios::in);
        std::ifstream::pos_type pos = ifs.tellg();

        std::vector<char>  result(pos);
        ifs.seekg(0, std::ios::beg);
        ifs.read(result.data(), pos);

        return result;
    }
    std::vector<char> moves_;
};


/**
 * @brief Just an example how to load the data
 */
int main(int argc, char const *argv[]) {
    std::string path = "../preprocessing/game.sgf.bin";
    SGFbin Game(path);

    board_t b;

    // replay all actions from SGFbin file
    for (int i = 0; i < Game.num_actions(); ++i) {
        int x = 0, y = 0;
        bool is_white = true, is_move = true, is_pass = true;
        token_t turn = white;

        Game.move(i, &x, &y, &is_white, &is_move, &is_pass);
        if (!is_white)
            turn = black;

        if (!is_pass) {
            if (is_move) {
                std::cout << "--> move" << std::endl;
                b.play(x, y, turn);
            } else {
                std::cout << "no move" << std::endl;
                b.set(x, y, turn);
            }

        }

    }

    b.print();

    std::cout << "white " << b.score_white << std::endl;
    std::cout << "black " << b.score_black << std::endl;

    return 0;
}

/**
 * @brief Just an example to test the capturing of stones
 */
int main2(int argc, char const *argv[]) {
    // test group capturing

    board_t b;

    b.play(0, 1, black);
    b.play(0, 2, black);
    b.play(1, 0, black);
    b.play(1, 3, black);
    b.play(1, 1, white);
    b.play(1, 2, white);
    b.play(2, 1, black);

    b.print();
    b.turn = black;
    b.play(2, 2, black);

    return 0;
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

    // create board
    board_t b;

    for (int i = 0; i < std::min(moves, (int)Game.num_actions() - 1); ++i) {
        int x = 0, y = 0;
        bool is_white = true, is_move = true, is_pass = true;
        Game.move(i, &x, &y, &is_white, &is_move, &is_pass);

        token_t turn = is_white ? white : black;
        // std::cout << "x " << x
        // << "\ty " << y
        // << "\tis_white " << is_white
        // << "\tis_move " << is_move
        // << "\tis_pass " << is_pass
        // << std::endl;


        if (!is_pass) {
            if (is_move) {
                b.play(x, y, turn);
            } else {
                b.set(x, y, turn);
            }
        } else {
            b.pass();
        }
    }
    b.feature_planes(data);


    int x = 0, y = 0;
    bool is_white = true, is_move = true, is_pass = true;
    token_t turn = white;
    Game.move(moves, &x, &y, &is_white, &is_move, &is_pass);
    const int next_move = 19 * x + y;

    return next_move;
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

    // create board representation
    board_t b;

    const int moves_to_play = std::min(moves, (int)Game.num_actions() - 1);

    for (int i = 0; i < moves_to_play; ++i) {
        int x = 0, y = 0;
        bool is_white = true, is_move = true, is_pass = true;
        Game.move(i, &x, &y, &is_white, &is_move, &is_pass);

        token_t turn = is_white ? white : black;

        if (!is_pass) {
            if (is_move) {
                b.play(x, y, turn);
            } else {
                b.set(x, y, turn);
            }
        } else {
            b.pass();
        }
    }

    // compute inputs for neural network
    b.feature_planes(data);

    // the move we want the network to predict
    int x = 0, y = 0;
    bool is_white = true, is_move = true, is_pass = true;
    Game.move(moves, &x, &y, &is_white, &is_move, &is_pass);
    const int next_move = 19 * x + y;

    return next_move;
}