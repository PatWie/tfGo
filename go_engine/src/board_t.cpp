// Author: Patrick Wieschollek <mail@patwie.com>

/* We use the GNUgo representation and map

        (2, 4) --> (15, C)

cout < board << endl;

------ START internal representation ------------------
   A B C D E F G H J K L M N O P Q R S T 
19 . . . . . . . . . . . . . . . . . . . 19 
18 . . . . . . . . . . . . . . . . . . . 18 
17 . . . . . . . . . . . . . . . . . . . 17 
16 . . . + . . . . . + . . x . . + . . . 16 
15 . . . o . . . . . . . . o x . . . . . 15 
14 . . . . . . . . . . . . . . . . . . . 14 
13 . . . . . . . . . . . . . . . . . . . 13 
12 . . . . . . . . . . . . . . . . . . . 12 
11 . . . . . . . . . . . . . . . . . . . 11 
10 . . . + . . . . . + . . . . . + . . . 10 
 9 . . . . . . . . . . . . . . . . . . .  9 
 8 . . . . . . . . . . . . . . . . . . .  8 
 7 . . . . . . . . . . . . . . . . . . .  7 
 6 . . . . . . . . . . . . . . . . . . .  6 
 5 . . . . . . . . . . . . . . . . . . .  5 
 4 . . . + . . . . . + . . . . . + . . .  4 
 3 . . . . . . . . . . . . . . . . . . .  3 
 2 . . . . . . . . . . . . . . . . . . .  2 
 1 . . . . . . . . . . . . . . . . . . .  1 
   A B C D E F G H J K L M N O P Q R S T
------ END internal representation ------------------



*/



#include <set>
#include <map>
#include <iomanip>

#include "field_t.h"
#include "group_t.h"
#include "board_t.h"


void ident(int depth){
    for (int i = 0; i < depth; ++i)
    {
        std::cout << "  ";
          
    }
}


board_t::board_t() : score_black(0.f), score_white(0.f), played_moves(0) {
    // tell each field its coordinate
    for (int h = 0; h < N; ++h){

        std::vector<field_t> row;
        for (int w = 0; w < N; ++w){
            row.push_back(field_t(h, w, this));
        }
        fields.push_back(row);
    }
    // set counter for group-ids
    groupid = 0;
    current_player = black;
}

board_t::~board_t() {
    for(auto &g : groups)
        delete g.second;
}



std::ostream& operator<< (std::ostream& stream, const board_t& b) {


    stream << "------ START internal representation ------------------" << std::endl;
    stream << std::endl;
    stream << "            WHITE (O) vs BLACK (X) " << std::endl;
    stream << std::endl;
    const char *charset = "ABCDEFGHJKLMNOPQRST";
    stream << "x  ";
    for (int w = 0; w < N; ++w){
        if(w<10)
            stream << w << " ";
        else
            stream << (w % 10) << " ";

        
    }
    stream << std::endl;
    stream << std::endl;
        
    stream << "   ";
    for (int w = 0; w < N; ++w)
        stream << charset[w] << " ";
    stream << "   " << "    y " << std::endl;
    
    for (int h = 0; h < N; ++h) {
        stream << std::setw(2) << (19 - h)  << " ";
        for (int w = 0; w < N; ++w)
            stream  << b.fields[h][w] << " ";
        stream << std::setw(2) << (19 - h)  << " ";
        stream << "   " << std::setw(2) << (h)  << " ";
        stream << std::endl;
    }
    stream << "  ";
    for (int w = 0; w < N; ++w)
        stream << " " << charset[w];
    stream << std::endl;
    stream << "------ END internal representation ------------------" << std::endl;

    return stream;
}


group_t* board_t::find_or_create_group(int id){
    groups_iter = groups.find(id);
    if (groups_iter != groups.end()){
        return groups_iter->second;
    }
    else{
        groups[id] = new group_t(id, this);
        return groups[id];
    }
}


board_t* board_t::clone() const {

    board_t* dest = new board_t();
    dest->played_moves = played_moves;
    dest->groupid = groupid;

    for (int h = 0; h < N; ++h)
        for (int w = 0; w < N; ++w){
            const field_t &src_f = fields[h][w];
            field_t &dest_f = dest->fields[h][w];

            if (src_f.token() != empty)
            {
                dest_f.token(src_f.token());
                dest_f.played_at = src_f.played_at;

                dest_f.group = dest->find_or_create_group(src_f.group->id);
                dest_f.group->add(&dest_f);
            }
            
        }
    dest->current_player = current_player;
    return dest;
}



int board_t::play(std::pair<int, int> pos, token_t tok) {
    return play(pos.first, pos.second, tok);
}
int board_t::play(int x, int y, token_t tok) {
    int r = set(x, y, tok);
    if (r == -1) return -1;
    current_player = opponent(current_player);
    return 0;
}

const field_t const * board_t::field(std::pair<int, int> pos) const{
    return (const field_t const*) &fields[pos.first][pos.second];
}

bool board_t::is_forced_ladder_escape(std::pair<int, int> escape_effort_field,
                               token_t hunter_player,
                               int recursion_depth,
                               group_t* focus){

    // too many recursion
    if(recursion_depth > 50)
        return false;

    // the victim player always tries to escape
    const token_t victim_player = opponent(hunter_player);

    // escape route is illegal
    if(!is_legal(escape_effort_field, current_player))
        return false;
    
    // these groups might help to escape from the current threat
    std::set<group_t *> group_to_check;

    if(focus == nullptr){
        // try to find all groups which belong to a ladder
        auto neighbor_groups_stones = neighbor_fields(escape_effort_field);
        for(auto &&possible_ladder_stones : neighbor_groups_stones){
            if(field(possible_ladder_stones)->token() == victim_player)
                if(field(possible_ladder_stones)->group->liberties() == 1){
                    group_to_check.insert(field(possible_ladder_stones)->group);
                }
        }
    }else{
        group_to_check.insert(focus);
    }

    // we now have all groups in place
    for(auto &&current_check_group_ : group_to_check){

        board_t* tmp = clone();
        tmp->play(escape_effort_field, victim_player);

        auto current_check_group = tmp->field(current_check_group_->stones[0]->pos())->group;

        // we escaped
        if(current_check_group->liberties() >= 3){
            delete tmp;
            return true;
        }

        // not good, try next group
        if(current_check_group->liberties() == 1){
            delete tmp;
            continue;
        }
 
        // only two liberties are left ...
        auto remaining_liberty_fields = current_check_group->neighbors(empty);

        bool any_is_capture = false;
        for(auto && next_move: remaining_liberty_fields){
            
            if(tmp->is_forced_ladder_capture(next_move, hunter_player, recursion_depth + 1,
                                             current_check_group)){
                any_is_capture = true;
                break;
            }
            
        }
        if(any_is_capture){
            delete tmp;
            continue;
        }

        delete tmp;
        return true;
    }

    return false;

}


bool board_t::is_forced_ladder_capture(std::pair<int, int> capture_effort,
                                token_t hunter_player,
                                int recursion_depth, group_t* focus) const{

    const token_t victim_player = opponent(hunter_player);


    // illegal moves are never ladder-captures
    if(!is_legal(capture_effort, hunter_player))
        return false;

    // timeout
    if(recursion_depth > 50)
        return false;

   
     // collect all fields that might be a victim_player of a ladder capture
    std::set<group_t *> group_to_check;

    // called with default args (no particular focus?)
    if(focus == nullptr){
        auto neighbor_groups_stones = neighbor_fields(capture_effort);
        for(auto &&stone : neighbor_groups_stones){
            if(field(stone)->token() == victim_player)
                if(field(stone)->group->liberties() == 2){
                    group_to_check.insert(field(stone)->group);
                }
        }

    }else{
        group_to_check.insert(focus);
    }

    // loop over all victim_player fields
    for(auto &&potential_ladder_capture_group_ : group_to_check){
        board_t* tmp = clone();
        tmp->play(capture_effort, hunter_player);

        auto potential_ladder_capture_group = tmp->field(potential_ladder_capture_group_->stones[0]->pos())->group;


        auto possible_escapes = potential_ladder_capture_group->neighbors(empty);

        for(auto &&possible_escape: possible_escapes){

            for(auto &&hunter_group : field(possible_escape)->neighbors(hunter_player)){
                if(field(hunter_group)->group->liberties() == 1){
                    possible_escapes.insert(possible_escape);
                }
            }
        }

        /*for (capture_effort_x, escape_y) in possible_escape_routes:*/
        bool not_a_single_escape = true;
        for(auto &&next_move : possible_escapes){
            if(!tmp->is_forced_ladder_escape(next_move, hunter_player, recursion_depth + 1,
                                             potential_ladder_capture_group)){
                not_a_single_escape = false;
            }
        }

        if(not_a_single_escape){
            delete tmp;
            return true;
        }

        

        delete tmp;
    }

    return false;
}


int board_t::set(int x, int y, token_t tok) {
    if (!valid_pos(x) || !valid_pos(y)) {
        std::cout << "field is not valid" << std::endl;
        return -1;
    }

    if (fields[x][y].token() != empty) {
        std::cout << "field was not empty" << std::endl;
        return -1;
    }

    if(!is_legal(x, y, tok)){
        std::cout << "move is not legal" << std::endl;
        return -1;
    }

    // place token to field
    fields[x][y].token(tok);
    fields[x][y].played_at = played_moves++;

    // update group structures
    update_groups(x, y);

    // does this move captures some opponent stones?
    int taken = count_and_remove_captured_stones(x, y, opponent(tok));

    // TODO: check suicidal move, i.e. a move which kills its own group
    // TODO: check ko, i.e. the same position should not appear on the board again (hashing?)

    // maintain scores
    if (taken > 0) {
        if (tok == white)
            score_white += taken;
        if (tok == black)
            score_black += taken;
    }
    return 0;
}
 const std::vector<std::pair<int, int> > board_t::neighbor_fields(std::pair<int, int> pos) const {
    return neighbor_fields(pos.first, pos.second);
 }
 const std::vector<std::pair<int, int> > board_t::neighbor_fields(int x, int y) const {
    std::vector<std::pair<int, int> > n;
    if(valid_pos(x - 1))
        n.push_back({x - 1, y});
    if(valid_pos(x + 1))
        n.push_back({x + 1, y});
    if(valid_pos(y - 1))
        n.push_back({x, y - 1});
    if(valid_pos(y + 1))
        n.push_back({x, y + 1});
    return n;
}

/**
 * @brief Update groups (merge, kill)
 * @details Update all neighboring groups (add current field and merge groups)
 * 
 * @param x focused token position
 * @param y focused token position
 */
void board_t::update_groups(int x, int y) {

    token_t current = fields[x][y].token();
    field_t& current_stone = fields[x][y];

    const auto neighbors = neighbor_fields(x, y);
    for(auto &&n : neighbors){
        field_t& other_stone = fields[n.first][n.second];
        if (other_stone.token() == current) {
            if (current_stone.group == nullptr)
                other_stone.group->add(&current_stone);
            else
                current_stone.group->merge(other_stone.group);
        }
    }

    // still single stone ? --> create new group
    if (current_stone.group == nullptr) {
        current_stone.group = find_or_create_group(groupid++);
        current_stone.group->add(&current_stone);
    }

}

const token_t board_t::opponent(token_t tok) const {
    return (tok == white) ? black : white;
}

bool board_t::is_legal(std::pair<int, int> pos, token_t tok) const {
    return is_legal(pos.first, pos.second, tok);
}
bool board_t::is_legal(int x, int y, token_t tok) const {
    // position has already a stone ?
    if (fields[x][y].token() != empty)
        return false;

    // // check own eye (special case from below)
    // bool is_eye = true;
    // if (is_eye && valid_pos(x + 1))
    //     if(fields[x + 1][y].token() != tok)
    //         is_eye = false;
    // if (is_eye && valid_pos(x - 1))
    //     if(fields[x - 1][y].token() != tok)
    //         is_eye = false;
    // if (is_eye && valid_pos(y + 1))
    //     if(fields[x][y + 1].token() != tok)
    //         is_eye = false;
    // if (is_eye && valid_pos(y - 1))
    //     if(fields[x][y - 1].token() != tok)
    //         is_eye = false;

    // if(is_eye)
    //     return false;

    // test self atari
    // placing that stone would cause the group to only have liberty afterwards
    board_t* copy = clone();
    copy->fields[x][y].token(tok);
    copy->update_groups(x, y);
    copy->count_and_remove_captured_stones(x, y, opponent(tok));
    copy->count_and_remove_captured_stones(x, y, tok);
    bool self_atari = (copy->liberties(x, y) == 0);
    // bool self_atari = (copy->fields[x][y].group->liberties(copy) == 0);
    delete copy;
    return !self_atari;

}

int board_t::estimate_captured_stones(int x, int y, token_t color_place, token_t color_count)  const {

    if (fields[x][y].token() != empty)
        return 0;

    board_t* copy = clone();
    copy->fields[x][y].token(color_place);
    copy->update_groups(x, y);
    int scores = copy->count_and_remove_captured_stones(x, y, color_count);
    delete copy;

    return scores;
}

int board_t::count_and_remove_captured_stones(int x, int y, token_t focus) {
    int scores = 0;

    const auto neighbors = neighbor_fields(x, y);
    for(auto &&n : neighbors){
        field_t& other_stone = fields[n.first][n.second];
        if (other_stone.token() == focus)
            if (liberties(n.first, n.second) == 0){
                const int gid = other_stone.group->id;
                scores += other_stone.group->kill();
                groups.erase(gid);
            }
    }

    return scores;
}

// int board_t::liberties(const field_t field) const{
int board_t::liberties(std::pair<int, int> pos) const{
    return liberties(pos.first, pos.second);
}
int board_t::liberties(int x, int y) const{
    if(fields[x][y].token() == empty)
        return 0;
    else
        return fields[x][y].group->liberties();

}

void board_t::feature_planes(int *planes, token_t self) const {
    // see https://gogameguru.com/i/2016/03/deepmind-mastering-go.pdf (Table 2)

    const int NN = 19 * 19;
    const token_t other = opponent(self);

    std::cout << *this << std::endl;
      

    for (int h = 0; h < N; ++h) {
        for (int w = 0; w < N; ++w) {

            // Stone colour 3
            // 1x mark all fields with own tokens
            // 1x mark all fields with opponent tokens
            // 1x mark all empty fields
            if (fields[h][w].token() == self)
                planes[map3line(0, h, w)] = 1;
            else if (fields[h][w].token() == other)
                planes[map3line(1, h, w)] = 1;
            else
                planes[map3line(2, h, w)] = 1;

            // Ones
            // fill entire plane with ones
            planes[map3line(3, h, w)] = 1;

            // Turns since
            // counter number of turns since the token was placed
            if (fields[h][w].token() != empty) {

                const int since = played_moves - fields[h][w].played_at + 1;

                if (since == 1)
                    planes[map3line(4, h, w)] = 1;
                else if (since == 2)
                    planes[map3line(5, h, w)] = 1;
                else if (since == 3)
                    planes[map3line(6, h, w)] = 1;
                else if (since == 4)
                    planes[map3line(7, h, w)] = 1;
                else if (since == 5)
                    planes[map3line(8, h, w)] = 1;
                else if (since == 6)
                    planes[map3line(9, h, w)] = 1;
                else if (since == 7)
                    planes[map3line(10, h, w)] = 1;
                else if (since > 7)
                    planes[map3line(11, h, w)] = 1;
            }

            // Liberties
            // 8x count number of liberties of own groups
            if (fields[h][w].token() == self) {

                const int num_liberties = liberties(h, w);

                if (num_liberties == 1)
                    planes[map3line(12, h, w)] = 1;
                else if (num_liberties == 2)
                    planes[map3line(13, h, w)] = 1;
                else if (num_liberties == 3)
                    planes[map3line(14, h, w)] = 1;
                else if (num_liberties == 4)
                    planes[map3line(15, h, w)] = 1;
                else if (num_liberties == 5)
                    planes[map3line(16, h, w)] = 1;
                else if (num_liberties == 6)
                    planes[map3line(17, h, w)] = 1;
                else if (num_liberties == 7)
                    planes[map3line(18, h, w)] = 1;
                else if (num_liberties > 7)
                    planes[map3line(19, h, w)] = 1;
            }

            // Liberties
            // 8x count number of liberties of opponent groups
            if (fields[h][w].token() == other) {

                const int num_liberties = liberties(h, w);

                if (num_liberties == 1)
                    planes[map3line(20, h, w)] = 1;
                else if (num_liberties == 2)
                    planes[map3line(21, h, w)] = 1;
                else if (num_liberties == 3)
                    planes[map3line(22, h, w)] = 1;
                else if (num_liberties == 4)
                    planes[map3line(23, h, w)] = 1;
                else if (num_liberties == 5)
                    planes[map3line(24, h, w)] = 1;
                else if (num_liberties == 6)
                    planes[map3line(25, h, w)] = 1;
                else if (num_liberties == 7)
                    planes[map3line(26, h, w)] = 1;
                else if (num_liberties > 7)
                    planes[map3line(27, h, w)] = 1;
            }

            // Capture size
            // 8x How many opponent stones would be captured when playing this field?
            if (fields[h][w].token() == empty) {

                const int num_capture = estimate_captured_stones(h, w, self, other);

                if (num_capture == 1)
                    planes[map3line(28, h, w)] = 1;
                else if (num_capture == 2)
                    planes[map3line(29, h, w)] = 1;
                else if (num_capture == 3)
                    planes[map3line(30, h, w)] = 1;
                else if (num_capture == 4)
                    planes[map3line(31, h, w)] = 1;
                else if (num_capture == 5)
                    planes[map3line(32, h, w)] = 1;
                else if (num_capture == 6)
                    planes[map3line(33, h, w)] = 1;
                else if (num_capture == 7)
                    planes[map3line(34, h, w)] = 1;
                else if (num_capture > 7)
                    planes[map3line(35, h, w)] = 1;
            }

            // Self-atari size
            // 8x How many own stones would be captured when playing this field?
            if (fields[h][w].token() == empty) {

                const int num_capture = estimate_captured_stones(h, w, self, self);

                if (num_capture == 1)
                    planes[map3line(36, h, w)] = 1;
                else if (num_capture == 2)
                    planes[map3line(37, h, w)] = 1;
                else if (num_capture == 3)
                    planes[map3line(38, h, w)] = 1;
                else if (num_capture == 4)
                    planes[map3line(39, h, w)] = 1;
                else if (num_capture == 5)
                    planes[map3line(40, h, w)] = 1;
                else if (num_capture == 6)
                    planes[map3line(41, h, w)] = 1;
                else if (num_capture == 7)
                    planes[map3line(42, h, w)] = 1;
                else if (num_capture > 7)
                    planes[map3line(43, h, w)] = 1;
            }

            // TODO Ladder c'mon (not here)
            // Ladder capture : 1 : Whether a move at this point is a successful ladder capture
            // Ladder escape : 1 : Whether a move at this point is a successful ladder escape
            // these two features are probably the most important ones, as they allow to look into the future
            // ... but they are missing here ;-)
            // this would require a small recursion

            // Sensibleness : 1 : Whether a move is legal and does not fill its own eyes
            // TODO legacy !is_legal
            if (is_legal(h, w, self)) {
                planes[map3line(44, h, w)] = 1;
            }

            // Zeros : 1 : A constant plane filled with 0
            planes[map3line(45, h, w)] = 0;

            // Player color :1: Whether current player is black
            const int value = (self == black) ? 1 : 0;
            planes[map3line(46, h, w)] = value;

        }
    }
}

