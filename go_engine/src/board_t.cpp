// Author: Patrick Wieschollek <mail@patwie.com>

/* We use the GNUgo representation (output) and map internally

        (x:4, y:2) <--> (15, C)

cout < board << endl;

y  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 

   A B C D E F G H J K L M N O P Q R S T        x 
19 o o o o x . . . . . . . . . . . . . . 19     0 
18 . x o x . . . . . . . . . . . . . . . 18     1 
17 . x o x . . . . . . . . . . . . . . . 17     2 
16 . . x + . . . . . + . . . . . + . . . 16     3 
15 . o . . . . . . . . . . . . . . . . . 15     4 
14 . . . . . . . . . . . . . . . . . . . 14     5 
13 . . . . . . . . . . . . . . . . . . . 13     6 
12 . . . . . . . . . . . . . . . . . . . 12     7 
11 . . . . . . . . . . . . . . . . . . . 11     8 
10 . . . + . . . . . + . . . . . + . . . 10     9 
 9 . . . . . . . . . . . . . . . . . . .  9    10 
 8 . . . . . . . . . . . . . . . . . . .  8    11 
 7 . . . . . . . . . . . . . . . . . . .  7    12 
 6 . . . . . . . . . . . . . . . . . . .  6    13 
 5 . . . . . . . . . . . . . . . . . . .  5    14 
 4 . . . + . . . . . + . . . . . + . . .  4    15 
 3 . . . . . . . . . . . . . . . . . . .  3    16 
 2 . . . . . . . . . . . . . . . . . . .  2    17 
 1 . . . . . . . . . . . . . . . . . . .  1    18 
   A B C D E F G H J K L M N O P Q R S T


*/


#include <set>
#include <map>
#include <iomanip>

#include "hash_t.h"
#include "field_t.h"
#include "group_t.h"
#include "board_t.h"



board_t::board_t() : score_black(0.f), score_white(0.f), moves_counter(0), current_hash(0) {
    // create all fields with ptr to this board and tell them their position
    for (int h = 0; h < N; ++h){
        std::vector<field_t> row;
        for (int w = 0; w < N; ++w)
            row.push_back(field_t(h, w, this));
        fields.push_back(row);
    }

    // to identify groups, we give them unique names (auto-incemenr like)
    groupid = 0;

    // history fo ko-rule
    ko = {-1, -1};

    // we do not maintain the "current_player"
}

board_t::~board_t() {
    // proper deletion of groups
    for(auto &g : groups)
        delete g.second;
}



std::ostream& operator<< (std::ostream& stream, const board_t& b) {
    stream << "------ START internal representation ------------------" << std::endl;
    stream << std::endl;
    stream << "            WHITE (O) vs BLACK (X) " << std::endl;
    stream << std::endl;
    const char *charset = "ABCDEFGHJKLMNOPQRST";
    stream << "y  ";
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
    stream << "   " << "    x " << std::endl;
    
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
    // try to get group by id
    groups_iter = groups.find(id);
    if (groups_iter != groups.end()){
        return groups_iter->second;
    }
    else{
        // group with id does not exists --> create a new group with that id
        groups[id] = new group_t(id, this);
        return groups[id];
    }
}


board_t* board_t::clone() const {
    // a deep clone

    board_t* dest = new board_t();
    dest->moves_counter = moves_counter;
    dest->groupid = groupid;

    // this hurts, but I currently have no idea what a more efficient way looks like
    for (int h = 0; h < N; ++h)
        for (int w = 0; w < N; ++w){
            const field_t &src_f = fields[h][w];
            
            if (src_f.token() != empty)
            {
                field_t &dest_f = dest->fields[h][w];

                dest_f.token(src_f.token());
                dest_f.played_at = src_f.played_at;

                // ouch, again (TODO: think about iterating instead over groups)
                dest_f.group = dest->find_or_create_group(src_f.group->id);
                dest_f.group->add(&dest_f);
            }
            
        }

    dest->current_hash = current_hash;
    dest->hash_history = hash_history;
    dest->ko = ko;
    return dest;
}



bool board_t::play(coord_t pos, token_t tok) {
    const int x = pos.first;
    const int y = pos.second;

    if (!valid_pos(x) || !valid_pos(y)) {
        std::cerr << "field is not valid" << std::endl;
        // *(char *)0 = 0; for debugging
        return false;
    }

    if (fields[x][y].token() != empty) {
        std::cerr << "field was not empty" << std::endl;
        return false;
    }

    if(!is_legal({x, y}, tok)){
        std::cerr << "move is not legal" << std::endl;
        return false;
    }

    // remove ko
    ko = {-1, -1};

    // place token to field
    fields[x][y].token(tok);
    fields[x][y].played_at = moves_counter++;

    // update group structures
    update_groups(pos);

    // does this move captures some opponent stones?
    int taken = count_and_remove_captured_stones(x, y, opponent(tok));

    // move was legal --> update history
    current_hash = rehash({x, y}, tok);
    hash_history.insert(current_hash);

    // maintain scores
    if (taken > 0) {
        if (tok == white)
            score_white += taken;
        if (tok == black)
            score_black += taken;

    }

    return true;
}

const field_t* board_t::field(coord_t pos) const{
    return &fields[pos.first][pos.second];
}


bool board_t::is_forced_ladder_capture(coord_t capture_effort,
                                token_t hunter_player,
                                int recursion_depth, group_t* focus) const{

    const token_t defender_player = opponent(hunter_player);

    // illegal moves are never ladder-captures
    if(!is_legal(capture_effort, hunter_player))
        return false;

    // timeout
    if(recursion_depth > 100)
        return true;

    // collect all fields that might be a defender_player of a ladder capture
    std::set<group_t *> groups_to_check;

    // called with default args (no particular focus?)
    if(focus == nullptr){
        auto neighbor_groups_stones = neighbor_fields(capture_effort);
        for(auto &&stone : neighbor_groups_stones)
            if(field(stone)->token() == defender_player)
                if(field(stone)->group->liberties() == 2)
                    groups_to_check.insert(field(stone)->group);
    }else{
        groups_to_check.insert(focus);
    }

    // loop over all defender_player fields
    for(auto &&potential_ladder_capture_group_ : groups_to_check){
        board_t* tmp = clone();
        tmp->play(capture_effort, hunter_player);

        // get group of deep copy tmp ("tmp" and "this" are different objects with different groups)
        auto potential_ladder_capture_group = tmp->field(potential_ladder_capture_group_->stones[0]->pos())->group;
        auto possible_escapes = potential_ladder_capture_group->neighbors(empty);

        // escape by capture hunter groups in atari
        for(auto &&hunter_field : potential_ladder_capture_group->neighbors(hunter_player)){
            auto hunter_group = tmp->field(hunter_field)->group;
            if(hunter_group->liberties() == 1){
                // hunter_group is in atari --> possible escape route
                for (auto&& e : hunter_group->neighbors(empty))
                    possible_escapes.insert(e);
            }
        }

        // check possible escape routes from defender player
        // if there exists an escape route --> we cannot force the capture
        // --> check next possible move
        bool no_escape_anymore = true;
        for(auto &&next_move : possible_escapes){
            if(tmp->is_forced_ladder_escape(next_move, hunter_player, recursion_depth + 1,
                                               potential_ladder_capture_group)){
                no_escape_anymore = false;
                break;
            }
        }

        if(no_escape_anymore){
            delete tmp;
            return true;
        }

        

        delete tmp;
    }

    return false;
}

std::uint64_t board_t::rehash(coord_t pos, token_t player) const{
    if(player == empty)
        return current_hash;
    const int x = pos.first;
    const int y = pos.second;

    // see https://en.wikipedia.org/wiki/Zobrist_hashing
    const std::uint64_t lut_entry = hash_t[player - 1][x][y];
    return current_hash^lut_entry;
}

const bool board_t::looks_like_an_eye(coord_t pos, token_t player) const{
    for(auto &&p : neighbor_fields(pos))
        if(field(p)->token() != player )
            return false;
    return true;
}


bool board_t::is_forced_ladder_escape(coord_t escape_effort_field,
                               token_t hunter_player,
                               int recursion_depth,
                               group_t* focus)  const{

    // too many recursion
    if(recursion_depth > 100)
        return false;

    // the defender player always tries to escape
    const token_t defender_player = opponent(hunter_player);

    // escape route is illegal
    if(!is_legal(escape_effort_field, defender_player))
        return false;
    
    // these groups might help to escape from the current threat
    std::set<group_t *> groups_to_check;

    if(focus == nullptr){
        // try to find all groups which belong to a ladder
        auto neighbor_groups_stones = neighbor_fields(escape_effort_field);
        for(auto &&possible_ladder_stones : neighbor_groups_stones){
            if(field(possible_ladder_stones)->token() == defender_player)
                if(field(possible_ladder_stones)->group->liberties() == 1){
                    groups_to_check.insert(field(possible_ladder_stones)->group);
                }
        }
    }else{
        groups_to_check.insert(focus);
    }

    // we now have all groups in place
    for(auto &&current_check_group_ : groups_to_check){

        board_t* tmp = clone();
        tmp->play(escape_effort_field, defender_player);

        // get group of deep copy tmp ("tmp" and "this" are different objects with different groups)
        auto current_check_group = tmp->field(current_check_group_->stones[0]->pos())->group;

        // more than 3 liberties --> hunter cannot capture this group anymore
        // --> defender can escape
        if(current_check_group->liberties() >= 3){
            delete tmp;
            return true;
        }

        // not good (in atari)
        // hunter will definitive be able to capture this group
        // --> try next group
        if(current_check_group->liberties() == 1){
            delete tmp;
            continue;
        }


        // only two liberties are left --> check if they belong to a ladder (recursion)
        auto remaining_liberty_fields = current_check_group->neighbors(empty);
        auto it = remaining_liberty_fields.begin();
        auto move_first = *it; it++;
        auto move_second = *it;

        bool first_is_capture = tmp->is_forced_ladder_capture(move_first, hunter_player,
                                        recursion_depth + 1, current_check_group);
   
        bool second_is_capture = tmp->is_forced_ladder_capture(move_second, hunter_player,
                                        recursion_depth + 1, current_check_group);


        // can hunter still force the capture? --> try next possible escape move
        if(first_is_capture || second_is_capture){
            delete tmp;
            continue;
        }

        // hunter cannot capture the defender anymore --> stop here --> we can escape
        if(!first_is_capture && !second_is_capture){
            delete tmp;
            return true;
        }

        delete tmp;
        
    }

    return false;

}

 const std::vector<coord_t > board_t::neighbor_fields(coord_t pos) const {
    const int x = pos.first;
    const int y = pos.second;

    std::vector<coord_t > n;
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


void board_t::update_groups(coord_t pos) {
    const int x = pos.first;
    const int y = pos.second;

    token_t current = fields[x][y].token();
    field_t& current_stone = fields[x][y];

    const auto neighbors = neighbor_fields({x, y});
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

bool board_t::is_legal(coord_t pos, token_t tok) const {
    const int x = pos.first;
    const int y = pos.second;

    if (!valid_pos(x) || !valid_pos(y))
        return false;

    // position has already a stone ?
    if (fields[x][y].token() != empty)
        return false;

    // check ko rule
    if(ko == pos)
        return false;

    // check super-ko
    if(contains(hash_history, rehash(pos, tok)))
        return false;

    // test suicide
    board_t* copy = clone();
    copy->fields[x][y].token(tok);
    copy->update_groups(pos);
    copy->count_and_remove_captured_stones(x, y, opponent(tok));
    copy->count_and_remove_captured_stones(x, y, tok);
    bool self_suicide = (copy->liberties(x, y) == 0);
    delete copy;

    return !self_suicide;

}

int board_t::estimate_captured_stones(int x, int y, token_t color_place, token_t color_count)  const {
    if (!valid_pos(x) || !valid_pos(y))
        return 0;

    if (fields[x][y].token() != empty)
        return 0;

    if(!is_legal({x, y}, color_place))
        return 0;

    board_t* copy = clone();
    copy->fields[x][y].token(color_place);
    copy->update_groups({x, y});
    int scores = copy->count_and_remove_captured_stones(x, y, color_count);
    delete copy;

    return scores;
}

int board_t::count_and_remove_captured_stones(int x, int y, token_t focus) {
    int scores = 0;

    const auto neighbors = neighbor_fields({x, y});
    for(auto &&n : neighbors){
        field_t& other_stone = fields[n.first][n.second];
        if (other_stone.token() == focus)
            if (liberties(n) == 0){
                const int gid = other_stone.group->id;
                // update ko for testing later legal_moves
                if(other_stone.group->stones.size() == 1){
                    ko = other_stone.group->stones[0]->pos();
                }
                // this will also change the hash of the current board
                scores += other_stone.group->kill(this);
                groups.erase(gid);
            }
    }

    return scores;
}


int board_t::liberties(coord_t pos) const{
    return liberties(pos.first, pos.second);
}

int board_t::liberties(int x, int y) const{
    // we keep this version for the features-planes
    if(fields[x][y].token() == empty)
        return 0;
    else
        return fields[x][y].group->liberties();

}

void board_t::feature_planes(int *planes, token_t self) const {
    // see https://gogameguru.com/i/2016/03/deepmind-mastering-go.pdf (Table 2, p. 31)
    /*
    This method assumes we regard the current board from the perspective of "self",
    which can be either "black" or "white".
    */


    const token_t other = opponent(self);


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
            // fill entire plane with ones (mark area to play)
            planes[map3line(3, h, w)] = 1;

            // Turns since
            // counter number of turns since the token was placed
            if (fields[h][w].token() != empty) {

                const int since = moves_counter - fields[h][w].played_at + 1;

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

            // Ladder capture : 1 : Whether a move at this point is a successful ladder capture
            if (fields[h][w].token() == empty) {
                bool is_ladder_capture = is_forced_ladder_capture({h, w}, self);
                if(is_ladder_capture)
                    planes[map3line(44, h, w)] = 1;
            }

            // Ladder escape : 1 : Whether a move at this point is a successful ladder escape
            if (fields[h][w].token() == empty) {
                bool is_ladder_capture = is_forced_ladder_escape({h, w}, self);
                if(is_ladder_capture)
                    planes[map3line(45, h, w)] = 1;
            }

            // Sensibleness : 1 : Whether a move is legal does not fill its own eyes
            if (is_legal({h, w}, self) && !looks_like_an_eye({h, w}, self)) {
                planes[map3line(46, h, w)] = 1;
            }

            // Zeros : 1 : A constant plane filled with 0
            planes[map3line(47, h, w)] = 0;

            // Player color :1: Whether current player is black
            const int value = (self == black) ? 1 : 0;
            planes[map3line(48, h, w)] = value;

        }
    }
}

