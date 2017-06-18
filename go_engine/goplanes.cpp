// Author: Patrick Wieschollek <mail@patwie.com>

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
    group_t(field_t *anchor, int groupid){
        stones.push_back(anchor);
        id = groupid;
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

    void merge(group_t* other){      
        // merge two groups (current groupo should own other stones) 

        // do not merge a group with itself
        if(other->id == id)
            return;
          
        for(field_t * s : other->stones){
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


class board_t
{
public:
    int groupid;


    board_t() : turn(white), score_black(0.f), score_white(0.f){
        // tell each field its coordinate
        for (int h = 0; h < N; ++h)
            for (int w = 0; w < N; ++w)
                fields[h][w].pos(h, w);
        groupid = 0;
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
                else{

                    if(fields[h][w].token() != empty)
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

    int move(int x, int y){
        // set token
        int r = set(x, y, turn);

        if(r == -1)
            return -1;
        // flip player
        turn = opponent();
        return 0;
    }

    void pass(){
        turn = opponent();
    }

    int set(int x, int y, token_t tok){
        // is field empty ?

        if(!valid_pos(x)){
            std::cout << "field was not empty" << std::endl;
            return -1;
        }

        if(!valid_pos(y)){
            std::cout << "field was not empty" << std::endl;
            return -1;
        }

        if(fields[x][y].token() != empty){
            std::cout << "field was not empty" << std::endl;
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
        // TODO: remove group-id ???
        // std::cout << "update-groups" << std::endl;
          

        token_t current = fields[x][y].token();
        field_t& current_stone = fields[x][y];

        if(valid_pos(x - 1)){
            field_t& other_stone = fields[x - 1][y];
            if(other_stone.token() == current){
                group_t *other_group = other_stone.group;
                if(current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);  
            }
        }

        if(valid_pos(y - 1)){
            field_t& other_stone = fields[x][y - 1];
            if(other_stone.token() == current){
                group_t *other_group = other_stone.group;
                if(current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);  
            }
        }

        if(valid_pos(x + 1)){
            field_t& other_stone = fields[x + 1][y];
            if(other_stone.token() == current){
                group_t *other_group = other_stone.group;
                if(current_stone.group == nullptr)
                    other_stone.group->add(&current_stone);
                else
                    current_stone.group->merge(other_stone.group);  
            }
        }

        if(valid_pos(y + 1)){
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
            current_stone.group = new group_t(&current_stone, groupid++);
        }
                    
    }

    const token_t opponent() const {
        if (turn == white)
            return black;
        return white;
    }

    int find_captured_stones(int x, int y){
        // std::cout << "find_captured_stones" << std::endl;
        int scores = 0;

        // does any opponent neighbor-group have no liberties?
        if(valid_pos(x - 1)){
            field_t& other_stone = fields[x - 1][y];
            if(other_stone.token() == opponent())
                if(other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        if(valid_pos(y - 1)){  
            field_t& other_stone = fields[x][y-1];
            if(other_stone.token() == opponent())
                if(other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        if(valid_pos(x + 1)){
            field_t& other_stone = fields[x + 1][y];
            if(other_stone.token() == opponent())
                if(other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        if(valid_pos(y + 1)){
            field_t& other_stone = fields[x][y+1];
            if(other_stone.token() == opponent())
                if(other_stone.group->liberties(this) == 0)
                    scores += other_stone.group->kill();
        }
        return scores;
    }

    void feature_planes(int *planes){
        // std::cout << "feature_planes" << std::endl;
        const int NN = 19*19;
        
        for (int h = 0; h < N; ++h)
        {
            for (int w = 0; w < N; ++w){
                // Stone colour : 3 : Player stone / opponent stone / empty
                if(fields[h][w].token() == turn)
                    planes[0*NN + map2line(h, w)] = 1;
                else if (fields[h][w].token() == opponent())
                    planes[1*NN + map2line(h, w)] = 1;
                else
                    planes[2*NN + map2line(h, w)] = 1;

                // Ones : 1 : A constant plane filled with 1
                planes[3*NN + map2line(h, w)] = 1;

                // Liberties :8: Number of liberties (empty adjacent points)
                if(fields[h][w].token() != empty){
                      
                    const int num_liberties = fields[h][w].group->liberties(this);

                    if(num_liberties == 1)
                        planes[4*NN + map2line(h, w)] = 1;
                    else if(num_liberties == 2)
                        planes[5*NN + map2line(h, w)] = 1;
                    else if(num_liberties == 3)
                        planes[6*NN + map2line(h, w)] = 1;
                    else if(num_liberties == 4)
                        planes[7*NN + map2line(h, w)] = 1;
                    else if(num_liberties == 5)
                        planes[8*NN + map2line(h, w)] = 1;
                    else if(num_liberties == 6)
                        planes[9*NN + map2line(h, w)] = 1;
                    else if (num_liberties == 7)
                        planes[10*NN + map2line(h, w)] = 1;
                    else
                        planes[11*NN + map2line(h, w)] = 1;
                }

                // TODO next planes (isn't this the idea case for ConvLSTM?)
                // this requires something like undo and history or deep-copy

                // Zeros : 1 : A constant plane filled with 0
                planes[12*NN + map2line(h, w)] = 0;

                // Player color :1: Whether current player is black
                const int value = (turn == black) ? 1: 0;
                planes[13*NN + map2line(h, w)] = value;

            }
        }
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

        const int x = s->x();
        const int y = s->y();

        if(valid_pos(y - 1))
            if(filled[map2line(x, y - 1)] == false)
                if(b->fields[x][y - 1].token() == empty)
                    filled[map2line(x, y - 1)] = 1;

        if(valid_pos(x - 1))
            if(filled[map2line(x - 1, y)] == false)
                if(b->fields[x - 1][y].token() == empty)
                    filled[map2line(x - 1, y)] = 1;

        if(valid_pos(y + 1))
            if(filled[map2line(x, y + 1)] == false)
                if(b->fields[x][y + 1].token() == empty)
                    filled[map2line(x, y + 1)] = 1;

        if(valid_pos(x + 1))
            if(filled[map2line(x + 1, y)] == false)
                if(b->fields[x + 1][y].token() == empty)
                    filled[map2line(x + 1, y)] = 1;

    }
    return filled.count();
}


class SGFbin
{
public:
    SGFbin(std::string path){
        moves_ = read_moves(path.c_str());
    }

    SGFbin(unsigned char* buffer, int len){
        moves_.assign(buffer, buffer+len);
    }

    void move(unsigned int step,
              int *x, int *y, bool *is_white, bool *is_move, bool *is_pass){
        decode((unsigned char)moves_[2*step], 
               (unsigned char)moves_[2*step+1],
                x, y, is_white, is_move, is_pass);
    }

    /**
     * @brief decodes
     * @details
     * ----mcyyyyyxxxxx
     * 
     */
    void decode(unsigned char m1, unsigned char m2,
                int *x, int *y, bool *is_white, bool *is_move, bool *is_pass){

        const int byte1 = (int) m1;
        const int byte2 = (int) m2;
        int value = (byte1<<8) + byte2;

        *is_white = value&1024;
        *is_move = value&2048;
        *is_pass = value&4096;

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
        bool is_move=true, is_white=true, is_pass=true;
        std::cout << "(;" << "GM[1]" << std::endl;
        std::cout << "SZ[19]" << std::endl;
          

        for (int i = 0; i < moves_.size(); i +=2)
        {
            decode((unsigned char)moves_[i], 
                   (unsigned char)moves_[i+1],
                    &x,&y, &is_white, &is_move, &is_pass);
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
        bool is_white = true, is_move = true, is_pass=true;
        token_t turn = white;
        Game.move(i, &x, &y, &is_white, &is_move, &is_pass);
        if(!is_white)
            turn = black;

        if(!is_pass){
            if(is_move){
                std::cout << "--> move" << std::endl;
                b.move(x, y);
            }else{
                std::cout << "no move" << std::endl;
                b.set(x, y, turn);
            }
            
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



int planes_from_file(char *str, int strlen, int* data, int len, int moves){
    // load game
    std::string path = std::string(str);
    SGFbin Game(path);

    // create board
    board_t b;

    for (int i = 0; i < std::min(moves, (int)Game.iters() - 1); ++i)
    {
        int x = 0, y = 0;
        bool is_white = true, is_move = true, is_pass=true;
        token_t turn = white;
        Game.move(i, &x, &y, &is_white, &is_move, &is_pass);

        // std::cout << "x " << x 
        // << "\ty " << y 
        // << "\tis_white " << is_white 
        // << "\tis_move " << is_move 
        // << "\tis_pass " << is_pass 
        // << std::endl;
          
        if(!is_white)
            turn = black;



        if(!is_pass){
            if(is_move){
                b.move(x, y);
            }else{
                b.set(x, y, turn);
            }
        }else{
            b.pass();
        }
    }

    int rsl = 0;
    {
        int x = 0, y = 0;
        bool is_white = true, is_move = true, is_pass=true;
        token_t turn = white;
        Game.move(moves, &x, &y, &is_white, &is_move, &is_pass);
        rsl = 19*x + y;

    }

    // TODO skip all pass/set situation to real moves


    b.feature_planes(data);    
    return rsl;
}

int planes_from_bytes(char *bytes, int byteslen, int* data, int len, int moves){

    SGFbin Game((unsigned char*) bytes, byteslen);

    // create board
    board_t b;


    for (int i = 0; i < std::min(moves, (int)Game.iters() - 1); ++i)
    {
        int x = 0, y = 0;
        bool is_white = true, is_move = true, is_pass=true;
        token_t turn = white;
        Game.move(i, &x, &y, &is_white, &is_move, &is_pass);

        // std::cout << "x " << x 
        // << "\ty " << y 
        // << "\tis_white " << is_white 
        // << "\tis_move " << is_move 
        // << "\tis_pass " << is_pass 
        // << std::endl;
          
        if(!is_white)
            turn = black;



        if(!is_pass){
            if(is_move){
                b.move(x, y);
            }else{
                b.set(x, y, turn);
            }
        }else{
            b.pass();
        }
    }

    b.feature_planes(data);  

    int rsl = 0;
    {
        int x = 0, y = 0;
        bool is_white = true, is_move = true, is_pass=true;
        token_t turn = white;
        Game.move(moves, &x, &y, &is_white, &is_move, &is_pass);
        rsl = 19*x + y;

    }

    // TODO skip all pass/set situation to real moves

    return rsl;

}