// Author: Patrick Wieschollek <mail@patwie.com>

#include <set>
#include <map>
#include <iomanip>

#include "field_t.h"
#include "group_t.h"
#include "board_t.h"


board_t::board_t() : score_black(0.f), score_white(0.f), played_moves(0) {
    // tell each field its coordinate
    for (int h = 0; h < N; ++h)
        for (int w = 0; w < N; ++w)
            fields[h][w].pos(h, w);
    // set counter for group-ids
    groupid = 0;
}

board_t::~board_t() {}


std::ostream& operator<< (std::ostream& stream, const board_t& b) {

    const char *charset = "ABCDEFGHJKLMNOPQRST";

    stream << "   ";
    for (int w = 0; w < N; ++w)
        stream << charset[w] << " ";
    stream << std::endl;
    
    for (int h = N - 1; h >= 0; --h) {
        stream << std::setw(2) << (h + 1)  << " ";
        for (int w = 0; w < N; ++w)
            stream  << b.fields[h][w] << " ";
        stream << std::setw(2) << (h + 1)  << " ";
        stream << std::endl;
    }
    stream << "  ";
    for (int w = 0; w < N; ++w)
        stream << " " << charset[w];
    stream << std::endl;

    return stream;
}


group_t* board_t::find_or_create_group(int id){
    groups_iter = groups.find(id);
    if (groups_iter != groups.end()){
        return groups_iter->second;
    }
    else{
        group_t *g = new group_t(id);
        groups[id] = g;
        return g;
    }
}


board_t * board_t::clone() const {

    board_t *dest = new board_t();
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
    return dest;
}



int board_t::play(int x, int y, token_t tok) {
    int r = set(x, y, tok);
    if (r == -1) return -1;
    return 0;
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

    if(!check_move_legal(x, y, tok)){
        std::cout << "move is not legal" << std::endl;
        return -1;
    }

    // place token to field
    fields[x][y].token(tok);
    fields[x][y].played_at = played_moves++;

    // update group structures
    update_groups(x, y);

    // does this move captures some opponent stones?
    int taken = find_captured_stones(x, y, opponent(tok));

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

    if (valid_pos(x - 1)) {
        field_t& other_stone = fields[x - 1][y];
        if (other_stone.token() == current) {
            if (current_stone.group == nullptr)
                other_stone.group->add(&current_stone);
            else
                current_stone.group->merge(other_stone.group);
        }
    }

    if (valid_pos(y - 1)) {
        field_t& other_stone = fields[x][y - 1];
        if (other_stone.token() == current) {
            if (current_stone.group == nullptr)
                other_stone.group->add(&current_stone);
            else
                current_stone.group->merge(other_stone.group);
        }
    }

    if (valid_pos(x + 1)) {
        field_t& other_stone = fields[x + 1][y];
        if (other_stone.token() == current) {
            if (current_stone.group == nullptr)
                other_stone.group->add(&current_stone);
            else
                current_stone.group->merge(other_stone.group);
        }
    }

    if (valid_pos(y + 1)) {
        field_t& other_stone = fields[x][y + 1];
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

bool board_t::check_move_legal(int x, int y, token_t tok) const {
    // position has already a stone ?
    if (fields[x][y].token() != empty)
        return false;

    // check own eye (special case from below)
    bool is_eye = true;
    if (is_eye && valid_pos(x + 1))
        if(fields[x + 1][y].token() != tok)
            is_eye = false;
    if (is_eye && valid_pos(x - 1))
        if(fields[x - 1][y].token() != tok)
            is_eye = false;
    if (is_eye && valid_pos(y + 1))
        if(fields[x][y + 1].token() != tok)
            is_eye = false;
    if (is_eye && valid_pos(y - 1))
        if(fields[x][y - 1].token() != tok)
            is_eye = false;

    if(is_eye)
        return false;

    // test self atari
    // placing that stone would cause the group to only have liberty afterwards
    board_t *copy = clone();
    copy->fields[x][y].token(tok);
    copy->update_groups(x, y);

    bool self_atari = (copy->fields[x][y].group->liberties(copy) < 2);
    delete copy;

    return !self_atari;

}

int board_t::estimate_captured_stones(int x, int y, token_t color_place, token_t color_count)  const {

    if (fields[x][y].token() != empty)
        return 0;

    board_t *copy = clone();
    copy->fields[x][y].token(color_place);
    copy->update_groups(x, y);
    int scores = copy->find_captured_stones(x, y, color_count);
    delete copy;

    return scores;
}

int board_t::find_captured_stones(int x, int y, token_t focus) {
    int scores = 0;

    // does any opponent neighbor-group has no liberties?
    if (valid_pos(x - 1)) {
        field_t& other_stone = fields[x - 1][y];
        if (other_stone.token() == focus)
            if (other_stone.group->liberties(this) == 0)
                scores += other_stone.group->kill();
    }
    if (valid_pos(y - 1)) {
        field_t& other_stone = fields[x][y - 1];
        if (other_stone.token() == focus)
            if (other_stone.group->liberties(this) == 0)
                scores += other_stone.group->kill();
    }
    if (valid_pos(x + 1)) {
        field_t& other_stone = fields[x + 1][y];
        if (other_stone.token() == focus)
            if (other_stone.group->liberties(this) == 0)
                scores += other_stone.group->kill();
    }
    if (valid_pos(y + 1)) {
        field_t& other_stone = fields[x][y + 1];
        if (other_stone.token() == focus)
            if (other_stone.group->liberties(this) == 0)
                scores += other_stone.group->kill();
    }
    return scores;
}

void board_t::feature_planes(int *planes, token_t self) const {
    // see https://gogameguru.com/i/2016/03/deepmind-mastering-go.pdf (Table 2)

    const int NN = 19 * 19;
    const token_t other = opponent(self);

    for (int h = 0; h < N; ++h) {
        for (int w = 0; w < N; ++w) {

            // Stone colour 3
            // 1x mark all fields with own tokens
            // 1x mark all fields with opponent tokens
            // 1x mark all empty fields
            if (fields[h][w].token() == self)
                planes[0 * NN + map2line(h, w)] = 1;
            else if (fields[h][w].token() == other)
                planes[1 * NN + map2line(h, w)] = 1;
            else
                planes[2 * NN + map2line(h, w)] = 1;

            // Ones
            // fill entire plane with ones
            planes[3 * NN + map2line(h, w)] = 1;

            // Turns since
            // counter number of turns since the token was placed
            if (fields[h][w].token() != empty) {

                const int since = played_moves - fields[h][w].played_at + 1;

                if (since == 1)
                    planes[4 * NN + map2line(h, w)] = 1;
                else if (since == 2)
                    planes[5 * NN + map2line(h, w)] = 1;
                else if (since == 3)
                    planes[6 * NN + map2line(h, w)] = 1;
                else if (since == 4)
                    planes[7 * NN + map2line(h, w)] = 1;
                else if (since == 5)
                    planes[8 * NN + map2line(h, w)] = 1;
                else if (since == 6)
                    planes[9 * NN + map2line(h, w)] = 1;
                else if (since == 7)
                    planes[10 * NN + map2line(h, w)] = 1;
                else if (since > 7)
                    planes[11 * NN + map2line(h, w)] = 1;
            }

            // Liberties
            // 8x count number of liberties of own groups
            if (fields[h][w].token() == self) {

                const int num_liberties = fields[h][w].group->liberties(this);

                if (num_liberties == 1)
                    planes[12 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 2)
                    planes[13 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 3)
                    planes[14 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 4)
                    planes[15 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 5)
                    planes[16 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 6)
                    planes[17 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 7)
                    planes[18 * NN + map2line(h, w)] = 1;
                else if (num_liberties > 7)
                    planes[19 * NN + map2line(h, w)] = 1;
            }

            // Liberties
            // 8x count number of liberties of opponent groups
            if (fields[h][w].token() == other) {

                const int num_liberties = fields[h][w].group->liberties(this);

                if (num_liberties == 1)
                    planes[20 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 2)
                    planes[21 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 3)
                    planes[22 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 4)
                    planes[23 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 5)
                    planes[24 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 6)
                    planes[25 * NN + map2line(h, w)] = 1;
                else if (num_liberties == 7)
                    planes[26 * NN + map2line(h, w)] = 1;
                else if (num_liberties > 7)
                    planes[27 * NN + map2line(h, w)] = 1;
            }

            // Capture size
            // 8x How many opponent stones would be captured when playing this field?
            if (fields[h][w].token() == empty) {

                const int num_capture = estimate_captured_stones(h, w, self, other);

                if (num_capture == 1)
                    planes[28 * NN + map2line(h, w)] = 1;
                else if (num_capture == 2)
                    planes[29 * NN + map2line(h, w)] = 1;
                else if (num_capture == 3)
                    planes[30 * NN + map2line(h, w)] = 1;
                else if (num_capture == 4)
                    planes[31 * NN + map2line(h, w)] = 1;
                else if (num_capture == 5)
                    planes[32 * NN + map2line(h, w)] = 1;
                else if (num_capture == 6)
                    planes[33 * NN + map2line(h, w)] = 1;
                else if (num_capture == 7)
                    planes[34 * NN + map2line(h, w)] = 1;
                else if (num_capture > 7)
                    planes[35 * NN + map2line(h, w)] = 1;
            }

            // Self-atari size
            // 8x How many own stones would be captured when playing this field?
            if (fields[h][w].token() == empty) {

                const int num_capture = estimate_captured_stones(h, w, self, self);

                if (num_capture == 1)
                    planes[36 * NN + map2line(h, w)] = 1;
                else if (num_capture == 2)
                    planes[37 * NN + map2line(h, w)] = 1;
                else if (num_capture == 3)
                    planes[38 * NN + map2line(h, w)] = 1;
                else if (num_capture == 4)
                    planes[39 * NN + map2line(h, w)] = 1;
                else if (num_capture == 5)
                    planes[40 * NN + map2line(h, w)] = 1;
                else if (num_capture == 6)
                    planes[41 * NN + map2line(h, w)] = 1;
                else if (num_capture == 7)
                    planes[42 * NN + map2line(h, w)] = 1;
                else if (num_capture > 7)
                    planes[43 * NN + map2line(h, w)] = 1;
            }

            // TODO Ladder c'mon (not here)
            // Ladder capture : 1 : Whether a move at this point is a successful ladder capture
            // Ladder escape : 1 : Whether a move at this point is a successful ladder escape
            // these two features are probably the most important ones, as they allow to look into the future
            // ... but they are missing here ;-)

            // Sensibleness : 1 : Whether a move is legal and does not fill its own eyes
            if (!check_move_legal(h, w, self)) {
                planes[44 * NN + map2line(h, w)] = 1;
            }

            // Zeros : 1 : A constant plane filled with 0
            planes[45 * NN + map2line(h, w)] = 0;

            // Player color :1: Whether current player is black
            const int value = (self == black) ? 1 : 0;
            planes[46 * NN + map2line(h, w)] = value;

        }
    }
}

