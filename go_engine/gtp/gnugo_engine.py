from base_engine import BaseEngine
from bridge import GTPbridge
import numpy as np


class GnuGoEngine(BaseEngine):
    """docstring for GnuGoEngine"""
    def __init__(self, name, args=None, verbose=False):
        super(GnuGoEngine, self).__init__(name)
        self.bridge = GTPbridge(name, ["gnugo", "--mode", "gtp"] + args, verbose=verbose)

    def call_show_board(self, args=None):
        return self.bridge.send('showboard\n')

    def call_version(self, args=None):
        return self.bridge.send('version\n')

    def call_boardsize(self, args=None):
        boardsize = int(args[0])
        return self.bridge.send("boardsize {}\n".format(boardsize))

    def call_genmove(self, args=None):
        color = args[0]
        return self.bridge.send("genmove {}\n".format(color))

    def call_final_score(self, args=None):
        return self.bridge.send("final_score\n")

    def call_play(self, args=None):
        color, move = args[:2]
        return self.bridge.send("play {} {}\n".format(color, move))

    def get_planes(self, from_view_of_white=True):
        board = self.bridge.send('showboard\n')

        def get_board(board):
            lines = board.split('\n')[2:-2]
            lines = [l[3:40].replace('+', '.').replace(' ', '') for l in lines]

            white = np.zeros((19, 19)).astype(np.int32)
            black = np.zeros((19, 19)).astype(np.int32)

            for x, line in enumerate(lines):
                for y, tok in enumerate(line):
                    if tok == 'X':
                        black[x, y] = 1
                    if tok == 'O':
                        white[x, y] = 1

            return white, black

        w, b = get_board(board)

        planes = np.zeros((47, 19, 19), dtype=np.int32)

        if from_view_of_white:
            own_stones = w
            opponent_stones = b
        else:
            own_stones = b
            opponent_stones = w

        planes[0, :, :] = own_stones
        planes[1, :, :] = opponent_stones

        # all empty fields
        planes[2, (w + b) == 0] = 1
        # fill entire ones
        planes[3, ...] = 1
        # liberties
        charset = "ABCDEFGHJKLMNOPQRST"

        liberties_cache = np.zeros((19, 19), dtype=np.int32)
        for x in range(19):
            for y in range(19):
                if planes[2, x, y] == 0:
                    xx = 19 - x
                    yy = charset[y]
                    ans = self.bridge.send('countlib %s%i\n' % (yy, xx)).split('\n')[0].replace('= ', '')
                    liberties_cache[x, y] = int(ans)

        planes[12, np.logical_and((own_stones == 1), (liberties_cache == 1))] = 1
        planes[13, np.logical_and((own_stones == 1), (liberties_cache == 2))] = 1
        planes[14, np.logical_and((own_stones == 1), (liberties_cache == 3))] = 1
        planes[15, np.logical_and((own_stones == 1), (liberties_cache == 4))] = 1
        planes[16, np.logical_and((own_stones == 1), (liberties_cache == 5))] = 1
        planes[17, np.logical_and((own_stones == 1), (liberties_cache == 6))] = 1
        planes[18, np.logical_and((own_stones == 1), (liberties_cache == 7))] = 1
        planes[19, np.logical_and((own_stones == 1), (liberties_cache > 7))] = 1

        planes[20, np.logical_and((opponent_stones == 1), (liberties_cache == 1))] = 1
        planes[21, np.logical_and((opponent_stones == 1), (liberties_cache == 2))] = 1
        planes[22, np.logical_and((opponent_stones == 1), (liberties_cache == 3))] = 1
        planes[23, np.logical_and((opponent_stones == 1), (liberties_cache == 4))] = 1
        planes[24, np.logical_and((opponent_stones == 1), (liberties_cache == 5))] = 1
        planes[25, np.logical_and((opponent_stones == 1), (liberties_cache == 6))] = 1
        planes[26, np.logical_and((opponent_stones == 1), (liberties_cache == 7))] = 1
        planes[27, np.logical_and((opponent_stones == 1), (liberties_cache > 7))] = 1

        liberties_cache = np.zeros((19, 19), dtype=np.int32)

        tmp = 'B'
        if from_view_of_white:
            tmp = 'W'

        legal_cache = np.zeros((19, 19), dtype=np.int32)
        for x in range(19):
            for y in range(19):
                if planes[2, x, y] == 1:
                    xx = 19 - x
                    yy = charset[y]
                    ans = self.bridge.send('is_legal %s %s%i\n' % (tmp, yy, xx)).split('\n')[0].replace('= ', '')
                    legal_cache[x, y] = int(ans)

        planes[44, legal_cache == 1] = 1
        return planes


if __name__ == '__main__':
    eng = GnuGoEngine('test')
    eng.msg_loop()
