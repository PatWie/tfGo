from base_engine import BaseEngine
from bridge import GTPbridge
import tensorflow as tf
from tensorflow.python.saved_model import tag_constants
import numpy as np
import re

import sys
sys.path.insert(0, "../python")  # noqa
import goplanes


def board2string(white, black=None):
    """Produce GnuGO like output to verify board position.

    Args:
        white (np.array): array with 1's for white
        black (np.array): array with 1's for black

    Returns:
        str: gnugo like output (without legend)
    """

    if black is None:
        n = np.copy(white)
        white = n < 0
        black = n > 0

    s = ''
    charset = "ABCDEFGHJKLMNOPQRST"
    s += '   '
    for x in xrange(19):
        s = s + charset[x] + ' '
    s += '\n'
    for x in xrange(19):
        s += '%02i ' % (19 - x)
        for y in xrange(19):
            if white[x][y] == 1:
                s += 'O '
            elif black[x][y] == 1:
                s += 'X '
            else:
                s += '. '
        s += ' %02i' % (19 - x)
        s += '\n'
    charset = "ABCDEFGHJKLMNOPQRST"
    s += '   '
    for x in xrange(19):
        s = s + charset[x] + ' '
    s += '\n'
    return s


def clean(s):
    """Remove uninteresting characters from GTP message.
    """
    s = re.sub("[\t\n =]", "", s)
    return s


def tuple2string(pos):
    # 12, 13 from top left --> O7
    charset = "ABCDEFGHJKLMNOPQRST"
    x, y = pos
    xx = 19 - x
    yy = charset[y]
    return '%s%i' % (yy, xx)


def string2tuple(ans):
    # O7 --> 12, 13 from top left
    charset = "ABCDEFGHJKLMNOPQRST"
    x = ans[0]
    x = charset.find(x.upper())
    y = 19 - int(ans[1:])
    return y, x


class TfGoEngine(BaseEngine):
    """docstring for TfGoEngine"""
    def __init__(self, name, export_dir):
        super(TfGoEngine, self).__init__(name)
        self.bridge = GTPbridge('bridge', ["gnugo", "--mode", "gtp"], verbose=False)

        self.sess = tf.Session(graph=tf.Graph(), config=tf.ConfigProto(allow_soft_placement=True))
        tf.saved_model.loader.load(self.sess, [tag_constants.SERVING], export_dir)

        # get input node, output node
        self.features = self.sess.graph.get_tensor_by_name('board_plhdr:0')
        self.prob = self.sess.graph.get_tensor_by_name('probabilities:0')

    def call_show_board(self, args=None):
        return self.bridge.send('showboard\n')

    def call_boardsize(self, args=None):
        boardsize = int(args[0])
        return self.bridge.send("boardsize {}\n".format(boardsize))

    def call_genmove(self, args=None):
        color = args[0].upper()

        # ask GnuGo for a move
        move = self._propose_move(color)
        color_symb = 'O' if (color == 'W') else 'X'
        print "TfGo[assistant] says '%s' for %s" % (move.replace('\n', ''), color_symb)
        if "pass" in move.lower():
            return "= pass"
        if "resign" in move.lower():
            return "= resign"

        # get current board
        white_board, black_board = self._get_board()

        # extract features for this board configuration
        planes = np.zeros((47, 19, 19), dtype=np.int32)
        goplanes.planes_from_position(white_board, black_board, planes, int(color == 'W'))
        planes = planes.reshape((1, 47, 19, 19))
        prob = self.sess.run(self.prob, {self.features: planes})[0][0]

        print board2string(white_board, black_board)
        print self.bridge.send('showboard\n').replace('=', ' ')
        self._debug(prob, planes)

        candidates = np.dstack(np.unravel_index(np.argsort(prob.ravel())[::-1], (19, 19)))

        print "TfGO Top-10 predictions (legal moves)  for %s:" % color_symb

        found_legal_moves = 0
        for i in range(60):
            x, y = candidates[0][i][0], candidates[0][i][1]
            move_description = tuple2string((x, y))
            p2 = prob[x][y]

            if self._is_legal(color, move_description):
                found_legal_moves += 1
                print "%03i:  %s \t(%f)" % (i + 1, move_description, p2)
                if found_legal_moves == 10:
                    break

        for i in range(19 * 19):
            x, y = candidates[0][i][0], candidates[0][i][1]
            move_description = tuple2string((x, y))
            p2 = prob[x][y]
            is_legal = 'legal' if self._is_legal(color, move_description) else 'illegal'

            if is_legal == 'legal':
                print "tfgo plays", color + " " + move_description
                self.bridge.send("play {} {}\n".format(color, move_description))
                return "= " + move_description

        return ""

    def call_final_score(self, args=None):
        return self.bridge.send("final_score\n")

    def call_play(self, args=None):
        color, move = args
        ans = self.bridge.send("play {} {}\n".format(color, move))
        return ans

    def call_quit(self, args=None):
        return "= "

    # private methods
    def _debug(self, prob, planes):
        charset = "ABCDEFGHJKLMNOPQRST"
        legend_a = ['    %s   ' % c for c in charset]
        print('   ' + "".join(legend_a))

        debug_prob = ""
        for ix in range(19):
            debug_prob += '%02i ' % (19 - ix)
            for iy in range(19):
                tok = '.'
                if planes[0, 0, ix, iy] == 1:
                    tok = ' '  # y
                    debug_prob += '    %s   ' % (tok)
                elif planes[0, 1, ix, iy] == 1:
                    tok = ' '  # n
                    debug_prob += '    %s   ' % (tok)
                else:
                    debug_prob += '   .%02i  ' % (int(prob[ix][iy] * 100))
                # debug_prob = "%s %04.2f(%s)" % (debug_prob, prob[ix][iy], tok)

            debug_prob += '  %02i ' % (19 - ix)
            debug_prob += "\n"
        print(debug_prob + '   ' + "".join(legend_a))

    def _get_board(self):
        """Return NumPy arrays of the current board configuration.

        Returns:
            (np.array, np.array): all placed white tokens, all placed black tokens
        """
        board = self.call_show_board()

        # regex = r"[0-9]{1,2}([ .+OX]*)[0-9]{1,2}"
        lines = board.split('\n')[2:-2]
        lines = [l[3:40].replace('+', '.').replace(' ', '') for l in lines]

        # print lines

        # matches = [re.finditer(regex, l)[0] for l in lines]
        # matches = [m.group(1).replace('+', '.').replace(' ', '') for m in matches]

        # print matches

        white = np.zeros((19, 19)).astype(np.int32)
        black = np.zeros((19, 19)).astype(np.int32)

        for x, line in enumerate(lines):
            for y, tok in enumerate(line):
                if tok == 'X':
                    # black[19 - x - 1][y] = 1
                    black[x, y] = 1
                if tok == 'O':
                    # white[19 - x - 1][y] = 1
                    white[x, y] = 1

        return white, black

    def _propose_move(self, color):
        """Propose a move without playing it.

        Args:
            color (str): Description

        Returns:
            TYPE: Description
        """
        assert color in ['W', 'B']
        return self.bridge.send('gg_genmove {}\n'.format(color))

    def _is_legal(self, color, move_descr):
        """Check whether a token can be placed at a field.

        Args:
            color (str): either 'B' or 'W'
            move_descr : e.g. F7

        Returns:
            bool: valid move?
        """
        assert color in ['W', 'B']
        ans = self.bridge.send("is_legal {} {}\n".format(color, move_descr))
        ans = int(clean(ans))
        return (ans == 1)


if __name__ == '__main__':
    eng = TfGoEngine('tfGo', '../../export')
    eng.msg_loop()
