#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Patrick Wieschollek <mail@patwie.com>


import tensorpack as tp
from tensorpack import *
import glob
import numpy as np
import argparse
from go_engine.python import goplanes
from tensorpack.utils import get_rng
import multiprocessing

FEATURE_LEN = 49


class GoGamesFromDir(tp.dataflow.DataFlow):
    """Yield GO game moves buffer from directory (I expect this to be slow.)
    """
    def __init__(self, files):
        super(GoGamesFromDir, self).__init__()
        self.files = files
        self.len = len(self.files)

    def get_data(self):
        for file in self.files:
            raw = np.fromfile(file, dtype=np.int8)
            yield [raw]

    def reset_state(self):
        pass


class GameDecoder(MapData):
    """Decode SGFbin and play game until a position.

    bytes ---> [features, next_move]
    """
    def __init__(self, df, random_move=True, until=None):
        """Yield a board configuration and next move from a LMDB data point

        Args:
            df: dataflow of LMDB entries
            random_move (bool, optional): pick random_move move in match
        """
        rng = get_rng(self)

        def func(dp):
            raw = dp[0]
            max_moves = len(raw) / 2

            # game is too short -> skip
            if max_moves < 10:
                return None

            # all move up to the last one (we want to predict at least one move)
            move_id = 1 + max_moves - 2
            if random_move:
                move_id = rng.randint(1, max_moves - 1)
            else:
                if until:
                    move_id = until

            features = np.zeros((FEATURE_LEN, 19, 19), dtype=np.int32)
            next_move = goplanes.planes_from_bytes(raw.tobytes(), features, move_id)

            assert not np.isnan(features).any()

            return [features, int(next_move)]
        super(GameDecoder, self).__init__(df, func)


class DihedralGroup(MapData):
    """Apply several transformations to the board.

    [features, next_move] ---> [D4(features), D4(next_move), D4(next_move_as_plane)]

    Returns all (left) actions of the DihedralGroup D4 = <a, b> where a^2=1, b^4=1, (ab)^2=1
    to board and label

    Remarks:
        We could do this directly in the TF-graph. However, tf.image.rotate, tf.map_fn seems to use single threaded
        CPU implementations. So we do it here instead.
    """

    def __init__(self, df):

        def mapping_func(dp):

            # compute sparse representation of next move (should be the same as "labels_2d")
            def rot90_scalar(x, y, s, k=0):
                for _ in range(k):
                    x, y = y, x          # transpose
                    x, y = s - 1 - x, y  # reverse
                return x, y

            def sparse_move_to_place(next_move):
                x = next_move % 19
                y = next_move // 19
                labels_2d = np.zeros([1, 19, 19], dtype=np.int32)
                labels_2d[0][x][y] = 1
                return labels_2d

            original_board = dp[0]
            original_next_move = dp[1]

            transformed_boards = []
            transformed_next_moves = []
            transformed_next_moves_plane = []

            def apply(x):
                versions = []
                versions.append(x)
                mirror = x[:, ::-1, :]
                versions.append(mirror)
                for op in range(1, 4):
                    versions.append(np.array([np.rot90(i, k=op) for i in x]).astype(np.int32))
                    versions.append(np.array([np.rot90(i, k=op) for i in mirror]).astype(np.int32))
                return np.concatenate(versions, axis=0)

            transformed_boards = apply(original_board)
            transformed_next_moves_plane = apply(sparse_move_to_place(original_next_move))

            x = original_next_move % 19
            y = original_next_move // 19

            for k in range(4):
                xx, yy = rot90_scalar(x, y, 19, k)
                transformed_next_moves.append(19 * yy + xx)
                xx, yy = rot90_scalar(19 - x - 1, y, 19, k)
                transformed_next_moves.append(19 * yy + xx)

            transformed_next_moves = np.array(transformed_next_moves, dtype=np.int32)

            return [transformed_boards, transformed_next_moves, transformed_next_moves_plane]
        super(DihedralGroup, self).__init__(df, mapping_func)


def tuple2string(pos):
    # 12, 13 from top left --> O7
    charset = "ABCDEFGHJKLMNOPQRST"
    x, y = pos
    xx = 19 - x
    yy = charset[y]
    return '%s%i' % (yy, xx)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--lmdb', type=str, help='path to lmdb', default='/home/patwie/godb/')
    parser.add_argument('--split', type=int, help='split', default=1)
    parser.add_argument('--pattern', type=str, help='pattern of binary sgf files',
                        default='/home/patwie/godb/Database/*/*.sgfbin')
    parser.add_argument('--action', type=str, help='action', choices=['create', 'debug1', 'debug2', 'benchmark'])
    args = parser.parse_args()

    if args.action == 'create':
        assert args.lmdb is not ''
        assert args.pattern is not ''

        files = glob.glob(args.pattern)
        print('found %i files' % len(files))

        if args.split == 1:
            file_idx = list(range(len(files)))
            np.random.seed(seed=42)
            np.random.shuffle(file_idx)

            split_train = int(len(file_idx) * 0.9)
            split_val = (len(files) - split_train) // 2
            split_test = len(files) - split_train - split_val

            train_idx = file_idx[:split_train]
            validate_idx = file_idx[split_train:split_train + split_val]
            test_idx = file_idx[split_train + split_val:]

            print('use %i/%i/%i split' % (len(train_idx), len(validate_idx), len(test_idx)))

            # train data
            df = GoGamesFromDir([files[i] for i in train_idx])
            dftools.dump_dataflow_to_lmdb(df, args.lmdb + 'go_train.lmdb')

            df = GoGamesFromDir([files[i] for i in validate_idx])
            dftools.dump_dataflow_to_lmdb(df, args.lmdb + 'go_val.lmdb')

            df = GoGamesFromDir([files[i] for i in test_idx])
            dftools.dump_dataflow_to_lmdb(df, args.lmdb + 'go_test.lmdb')
        else:
            df = GoGamesFromDir([file for file in files])
            dftools.dump_dataflow_to_lmdb(df, args.lmdb + 'go.lmdb')

    if args.action == 'benchmark':
        df = LMDBDataPoint(args.lmdb, shuffle=False)
        df = PrefetchData(df, 5000, 1)
        df = GameDecoder(df)
        df = PrefetchDataZMQ(df, min(20, multiprocessing.cpu_count()))
        df.reset_state()
        TestDataSpeed(df, size=50000).start_test()

    if args.action == 'debug1':
        df = LMDBDataPoint(args.lmdb, shuffle=False)
        df = GameDecoder(df, random_move=False, until=81)
        df.reset_state()

        for dp in df.get_data():
            planes, next_move = dp

            x = next_move % 19
            y = next_move // 19

            bboard = np.zeros((19, 19), dtype=str)
            bboard[planes[0, :, :] == 1] = 'x'  # own stones
            bboard[planes[1, :, :] == 1] = '!'  # opponent stones
            bboard[planes[2, :, :] == 1] = '.'  # empty fields

            bboard[x, y] = '$'  # next move (new own stone)

            for i in range(19):
                print " ".join(bboard[i, :])

            print "next move", next_move, x, y, tuple2string((x, y))

    if args.action == 'debug2':
        df = LMDBDataPoint(args.lmdb, shuffle=False)
        df = GameDecoder(df, random_move=False, until=81)
        df = DihedralGroup(df)
        df.reset_state()

        for dp in df.get_data():
            D4_planes, D4_next_move, D4_next_move_as_plane = dp

            print D4_planes.shape
            print D4_next_move.shape
            print D4_next_move_as_plane.shape

            for version in range(D4_next_move_as_plane.shape[0]):

                print "\n\n\nversion %i \n" % version

                next_move = D4_next_move[version]
                x = next_move % 19
                y = next_move // 19
                print "next move", next_move, x, y, tuple2string((x, y))
                planes = D4_planes[version * 47:version * 47 + 48]

                bboard = np.zeros((19, 19), dtype=str)
                bboard[planes[0, :, :] == 1] = 'x'  # own stones
                bboard[planes[1, :, :] == 1] = '!'  # opponent stones
                bboard[planes[2, :, :] == 1] = '.'  # empty fields

                bboard[x, y] = '$'  # next move (new own stone)

                for i in range(19):
                    print " ".join(bboard[i, :])

                print D4_next_move_as_plane[version]
