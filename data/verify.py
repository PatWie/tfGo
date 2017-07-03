
import sys
import numpy as np
sys.path.insert(0, "../go_engine/gtp/")  # noqa
sys.path.insert(0, "../go_engine/python/")  # noqa

from gnugo_engine import GnuGoEngine
import goplanes

NUM_FEATURES = 49

until = 81
next_is_white = (until % 2 == 0)

if next_is_white:
    print "situation for white"
else:
    print "situation for black"

fn = "2p0y-gokifu-20170527-Ke_Jie-AlphaGo.sgf"

gnugo = GnuGoEngine('gnugo', ["--infile", fn, '-L', str(until)], verbose=True)
gnugo.call_show_board()
gnugo_planes = gnugo.get_planes(next_is_white)


tfgo_planes = np.zeros((NUM_FEATURES, 19, 19), dtype=np.int32)
next_move = goplanes.planes_from_file(fn + 'bin', tfgo_planes, until)
x = next_move % 19
y = next_move // 19


prob = np.zeros((19, 19), dtype=np.int32)
prob[x, y] = 100
i = 0
candidates = np.dstack(np.unravel_index(np.argsort(prob.ravel())[::-1], (19, 19)))
x, y = candidates[0][i][0], candidates[0][i][1]


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

print "tuple2string", (x, y)
print "tuple2string", tuple2string((x, y))
print "tuple2string", string2tuple(tuple2string((x, y)))

for i in [0, 1, 2, 3] + range(12, 28) + range(20, 28):
    print "plane %i:" % i, np.sum(gnugo_planes[i, :, :] - tfgo_planes[i, :, :])


if next_is_white:
    white_board = gnugo_planes[0, ...]
    black_board = gnugo_planes[1, ...]
else:
    white_board = gnugo_planes[1, ...]
    black_board = gnugo_planes[0, ...]

tfgo_planes_from_position = np.zeros((NUM_FEATURES, 19, 19), dtype=np.int32)
goplanes.planes_from_position(white_board, black_board, tfgo_planes_from_position, int(next_is_white))

print "next move", next_move, x, y, tuple2string((x, y))

for i in [0, 1, 2, 3] + range(12, 28) + range(20, 28) + [46]:
    print "diff (gnugo vs. tfgo) in plane %i:" % i, np.sum(gnugo_planes[i, :, :] - tfgo_planes_from_position[i, :, :])

print "legal moves"
print tfgo_planes_from_position[46, :, :]


gnugo.call_show_board()


if next_is_white:
    print "situation for white"
else:
    print "situation for black"
# def get_gnugo_board(fn, until=None):
#     """Use GnuGO to compute final board of game.

#     Args:
#         fn (str): path to -sgf file
#         until (str, optional): number of moves to play (exclude handicap stones)

#     Returns:
#         (np.aray, np.aray): board white, black
#     """
#     if until is not None:
#         print('[gnugo] play %i moves' % until)
#         until = ['-L', str(until)]
#     else:
#         until = []
#     bridge = GTPbridge('name', ["gnugo",
#                                 "--infile", fn,
#                                 "--mode", "gtp"] + until, verbose=True)

#     board = bridge.send('showboard\n')

#     def get_board(board):
#         """Return NumPy arrays of the current board configuration.

#         Returns:
#             (np.array, np.array): all placed white tokens, all placed black tokens
#         """

#         lines = board.split('\n')[2:-2]
#         lines = [l[3:40].replace('+', '.').replace(' ', '') for l in lines]

#         white = np.zeros((19, 19)).astype(np.int32)
#         black = np.zeros((19, 19)).astype(np.int32)

#         for x, line in enumerate(lines):
#             for y, tok in enumerate(line):
#                 if tok == 'X':
#                     black[x, y] = 1
#                 if tok == 'O':
#                     white[x, y] = 1

#         return white, black

#     planes = np.zeros((NUM_FEATURES, 19, 19))

#     ans = get_board(board)
#     bridge.close()
#     return ans


# expected_w, expected_b = get_gnugo_board(fn, 50)
# print expected_w

# def print_board(white, black=None):
#     """Produce GnuGO like output to verify board position.

#     Args:
#         white (np.array): array with 1's for white
#         black (np.array): array with 1's for black

#     Returns:
#         str: gnugo like output (without legend)
#     """

#     if black is None:
#         n = np.copy(white)
#         white = n < 0
#         black = n > 0

#     s = ''
#     charset = "ABCDEFGHJKLMNOPQRST"
#     s += '   '
#     for x in xrange(19):
#         s = s + charset[x] + ' '
#     s += '\n'
#     for x in xrange(19):
#         s += '%02i ' % x
#         for y in xrange(19):
#             if white[x][y] == 1:
#                 s += '0 '
#             elif black[x][y] == 1:
#                 s += 'X '
#             else:
#                 s += '. '
#         s += ' %02i' % x
#         s += '\n'
#     charset = "ABCDEFGHJKLMNOPQRST"
#     s += '   '
#     for x in xrange(19):
#         s = s + charset[x] + ' '
#     s += '\n'
#     return s


# def get_own_board(fn, until=None):
#     """Use goplanes library from this repository for board representation.

#     Remarks:
#         This uses the feature-plane code (input for NN) to create the board positions.
#         We just use it for testing. So far all final board configurations from `GoGoD`
#         matches the ones from GnuGO.

#     Args:
#         fn (str): path to *.sgfbin file
#         until (int, optional): number of moves to play (with handicap stones)

#     Returns:
#         (np.array, np.array): all placed white tokens, all placed black tokens
#     """
#     max_moves = os.path.getsize(fn + 'bin') / 2
#     print('[own] has %i moves' % max_moves)
#     planes = np.zeros((NUM_FEATURES, 19, 19), dtype=np.int32)
#     if until is not None:
#         until = max(0, until + 2)
#     else:
#         until = -1
#     print('[own] play %i moves' % until)
#     goplanes.planes_from_file(fn + 'bin', planes, until)

#     from_black = (planes[-1][0][0] == 1)
#     if from_black:
#         actual_b = planes[0]
#         actual_w = planes[1]
#     else:
#         actual_b = planes[1]
#         actual_w = planes[0]

#     return actual_w, actual_b


# for fn in glob.glob('/home/patwie/godb/Database/*/*.sgf'):
# # for fn in glob.glob('/home/patwie/godb/Database/1998/1998-03-09d.sgf'):
# # for fn in glob.glob('/home/patwie/godb/Database/1998/1998-01-09k.sgf'):

#     # filter not converted games (like incorrect and amateur)
#     if not os.path.isfile(fn):
#         continue
#     if not os.path.isfile(fn + 'bin'):
#         continue
#     print fn
#     moves = None  # means all
#     # moves = 50
#     # moves = os.path.getsize(fn + 'bin') / 2
#     expected_w, expected_b = get_gnugo_board(fn, moves)
#     actual_w, actual_b = get_own_board(fn, moves)

#     # if True:
#     if (expected_w - actual_w).sum() > 0 or (expected_b - actual_b).sum() > 0:
#         # there is a difference between GnuGO and GoPlanes
#         print fn
#         print print_board(expected_w, expected_b)
#         print print_board(actual_w, actual_b)
#         print (expected_w - actual_w).sum()
#         print (expected_b - actual_b).sum()

#         print print_board(expected_w - actual_w)
#         print print_board(expected_b - actual_b)
#         raw_input("prompt")
