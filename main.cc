// Author: Patrick Wieschollek <mail@patwie.com>

#include <iostream>
#include <vector>
#include <utility>
#include <array>
#include <bitset>
#include <fstream>
#include <string>

const int N = 19;

enum token_t { empty, white, black };
class field_t;
class group_t;
class board_t;

class field_t
{
public:

    field_t() : token_(empty), group(nullptr), x_(-1), y_(-1) {}

    const token_t token() const{
        return token_;
    }

    void token(token_t tok){
        token_ = tok;
    }

    void pos(int x, int y){
        x_ = x;
        y_ = y;
    }

    const int x() const{ return x_;}
    const int y() const{ return y_;}

    friend std::ostream& operator<< (std::ostream& stream, const field_t& stone) {
        if(stone.token() == empty)
            stream  << ".";
        if(stone.token() == white)
            stream  << "o";
        if(stone.token() == black)
            stream  << "x";
        return stream;
    }

    group_t *group;

private:
    int x_;
    int y_;
    token_t token_;
};

class group_t
{
public:
    group_t(field_t *anchor){
        stones.push_back(anchor);
    }
    ~group_t(){}

    void add(field_t *s){
        // add stone to group
        // check outside if stone belongs to opponent group!
        s->group = this;
        stones.push_back(s);
    }

    const unsigned int size() const{
        return stones.size();
    }

    int kill(){  
        // kill entire group (remove stones from board, destroy group, return score)
        int score = stones.size();
        for(field_t * s : stones){
            s->token(empty);
            s->group = nullptr;
        }
        delete this;
        return score;
    }

    void merge(group_t* opponent){
        // merge two groups (current groupo should own opponent stones)
        for(field_t * s : opponent->stones){
            s->group = this;
        }
        delete opponent;
    }

    // count liberties (see below)
    // TODO: cache result (key should be iteration in game)
    int liberties(board_t *b);

    // collection of pointers to stones
    std::vector<field_t *> stones;
};


class board_t
{
public:
    board_t() : turn(white), score_black(0.f), score_white(0.f){
        // tell each field its coordinate
        for (int h = 0; h < N; ++h)
            for (int w = 0; w < N; ++w)
                fields[h][w].pos(h, w);
    }

    ~board_t(){}

    // prints
    // ------------------------------------------------------------------

    void print(){
        for (int h = 0; h < N; ++h)
        {
            for (int w = 0; w < N; ++w)
                std::cout << fields[h][w] << " ";
            std::cout  << std::endl;
        }
    }

    void debug_groups(){
        for (int h = 0; h < N; ++h)
        {
            for (int w = 0; w < N; ++w)
            {
                if(fields[h][w].group == nullptr)
                    std::cout << "0"<< " "; 
                else
                    std::cout << fields[h][w].group->size()<< " ";
            }
            std::cout  << std::endl;
        }
    }

    void debug_liberties(){
        for (int h = 0; h < N; ++h)
        {
            for (int w = 0; w < N; ++w)
            {
                if(fields[h][w].group == nullptr)
                    std::cout << "0"<< " ";
                else
                    std::cout << fields[h][w].group->liberties(this) << " ";
            }
            std::cout  << std::endl;
        }
    }

    // game logic
    // ------------------------------------------------------------------

    int move(int x, int y, token_t tok){
        // set token
        int r = set(x, y, tok);

        if(r == -1)
            return -1;
        // flip player
        turn = opponent();
        return 0;
    }

    int set(int x, int y, token_t tok){
        // is field empty ?
        if(fields[x][y].token() != empty){
            return -1;
        }

        // place token to field
        fields[x][y].token(tok); 

        // update group structures
        update_groups(x, y);

        // does this move captures some stones?
        int taken = find_captured_stones(x, y); 

        // TODO: check suicidal (taken == 0)
        // TODO: check ko (hashing?)

        // maintain scores
        if(taken > 0){
            if(turn == white)
                score_white += taken;
            if(turn == black)
                score_black += taken;
        }
        return 0;
    }

    void update_groups(int x, int y){

        token_t current = fields[x][y].token();
        field_t& current_stone = fields[x][y];

        if(x - 1 >= 0){
            field_t& other_stone = fields[x - 1][y];
            if(other_stone.token() == current){
                group_t *other_group = other_stone.group;
                if(current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);  
            }
        }

        if(y - 1 >= 0){
            field_t& other_stone = fields[x][y - 1];
            if(other_stone.token() == current){
                group_t *other_group = other_stone.group;
                if(current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);  
            }
        }

        if(x + 1 < N){
            field_t& other_stone = fields[x + 1][y];
            if(other_stone.token() == current){
                group_t *other_group = other_stone.group;
                if(current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);
            }
        }

        if(y + 1 < N){
            field_t& other_stone = fields[x][y + 1];
            if(other_stone.token() == current){
                group_t *other_group = other_stone.group;
                if(current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);
            }
        }

        // still single stone ? --> create new group
        if(current_stone.group == nullptr){
            current_stone.group = new group_t(&current_stone);
        }
                    
    }

    const token_t opponent() const {
        if (turn == white)
            return black;
        return white;
    }

    int find_captured_stones(int x, int y){
        int scores = 0;

        // does any opponent neighbor-group have no liberties?

        if(x-1 >= 0){
            field_t& other_stone = fields[x - 1][y];
            if(other_stone.token() == opponent())
                if(other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        if(y-1 >= 0){  
            field_t& other_stone = fields[x][y-1];
            if(other_stone.token() == opponent())
                if(other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        if(x+1 >= 0){
            field_t& other_stone = fields[x + 1][y];
            if(other_stone.token() == opponent())
                if(other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        if(y+1 >= 0){
            field_t& other_stone = fields[x][y+1];
            if(other_stone.token() == opponent())
                if(other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        return scores;
    }

    std::array<std::array<field_t, N>, N> fields;
    token_t turn;

    float score_black;
    float score_white;
};

// to avoid circular dependency
int group_t::liberties(board_t *b){
    // all fields we already counted
    std::bitset<19*19> filled(0);

    for(field_t * s : stones){

        #define map(x,y) (((x) * 19 + (y)))

        const int x = s->x();
        const int y = s->y();

        if(x - 1 >= 0)
            if(filled[map(x - 1, y)] == false)
                if(b->fields[x - 1][y].token() == empty)
                    filled[map(x - 1, y)] = 1;
        
        if(y - 1 >= 0){
            if(filled[map(x, y - 1)] == false)
                if(b->fields[x][y - 1].token() == empty)
                    filled[map(x, y - 1)] = 1;
        }
        if(x + 1 < N){
            if(filled[map(x + 1, y)] == false)
                if(b->fields[x + 1][y].token() == empty)
                    filled[map(x + 1, y)] = 1;
        }
        if(y + 1 < N){
            if(filled[map(x, y + 1)] == false)
                if(b->fields[x][y + 1].token() == empty)
                    filled[map(x, y + 1)] = 1;
        }
    }
    return filled.count();
}


class SGFbin
{
public:
    SGFbin(std::string path){
        moves_ = read_moves(path.c_str());
    }

    void move(unsigned int step,
              int *x, int *y, bool *is_white, bool *is_move){
        decode((unsigned char)moves_[2*step], 
               (unsigned char)moves_[2*step+1],
                x, y, is_white, is_move);
    }

    /**
     * @brief decodes
     * @details
     * ----mcyyyyyxxxxx
     * 
     */
    void decode(unsigned char m1, unsigned char m2,
                int *x, int *y, bool *is_white, bool *is_move){

        const int byte1 = (int) m1;
        const int byte2 = (int) m2;
        int value = (byte1<<8) + byte2;

        *is_move = value&2048;
        *is_white = value&1024;

        if(*is_move)
            value -= 2048;
        if(*is_white)
            value -= 1024;

        *x = (byte2 % 32);
        *y = (value - *x) / 32;
    }

    const unsigned int iters() const{
        return moves_.size() / 2;
    }

    void dump(){
        int x=0, y=0;
        bool is_move=true, is_white=true;
        std::cout << "(;" << "GM[1]" << std::endl;
        std::cout << "SZ[19]" << std::endl;
          

        for (int i = 0; i < moves_.size(); i +=2)
        {
            decode((unsigned char)moves_[i], 
                   (unsigned char)moves_[i+1],
                    &x,&y, &is_white, &is_move);
            if (is_move)
            {
                if(is_white)
                    std::cout << ";W[";
                else
                    std::cout << ";B[";
                std::cout << (char)('a' + x) << (char)('a' + y) <<"]" << std::endl; 
            }else{
                if(is_white)
                    std::cout << "AW";
                else
                    std::cout << "AB";
                std::cout << "["<<(char)('a' + x) << (char)('a' + y) <<"]" << std::endl; 
            }
        }
        std::cout << ")" << std::endl;
    }

private:
    std::vector<char> read_moves(char const* filename)
    {
        std::ifstream ifs(filename, std::ios::binary|std::ios::ate|std::ios::in);
        std::ifstream::pos_type pos = ifs.tellg();

        std::vector<char>  result(pos);
        ifs.seekg(0, std::ios::beg);
        ifs.read(result.data(), pos);

        return result;
    }  
    std::vector<char> moves_; 
};


int main(int argc, char const *argv[])
{
    std::string path = "../preprocessing/game.sgf.bin";
    SGFbin Game(path);
    board_t b;
    b.score_white = 0.5;

    for (int i = 0; i < Game.iters(); ++i)
    {
        int x = 0, y = 0;
        bool is_white = true, is_move = true;
        token_t turn = white;
        Game.move(i, &x, &y, &is_white, &is_move);
        if(!is_white)
            turn = black;

        if(is_move){
            std::cout << "--> move" << std::endl;
            b.move(x, y, turn);
        }else{
            std::cout << "no move" << std::endl;
            b.set(x, y, turn);
        }
    }


    // b.move(0, 1, black);
    // b.move(0, 2, black);
    // b.move(1, 0, black);
    // b.move(1, 3, black);
    // b.move(1, 1, white);
    // b.move(1, 2, white);
    // b.move(2, 1, black);

    // b.print();
    // b.turn = black;
    // b.move(2, 2, black);

    b.print();

    std::cout << "white " << b.score_white << std::endl;
    std::cout << "black " << b.score_black << std::endl;
      
    // b.move(2, 3, white);
    // b.move(3, 3, empty);

    // std::cout << std::endl;
    // b.debug_groups();

    // std::cout << std::endl;
    // b.debug_liberties();

      
    return 0;
}