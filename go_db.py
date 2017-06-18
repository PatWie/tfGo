#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Patrick Wieschollek <mail@patwie.com>


import tensorpack as tp
from tensorpack import *
import glob
import numpy as np
import argparse
from go_engine import goplanes
from tensorpack.utils import get_rng


class GoGamesFromDir(tp.dataflow.DataFlow):
    """Yield frame triplets for phase 0
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


class GoGamesFromDb(tp.dataflow.RNGDataFlow):
    """docstring for GoGamesFromDb"""
    def __init__(self, lmdb):
        super(GoGamesFromDb, self).__init__()
        self.lmdb = lmdb


class GameDecoder(MapData):
        """convert game into feature planes"""
        def __init__(self, df, random=True):

            self.rng = get_rng(self)

            def func(dp):
                raw = dp[0]
                max_moves = len(raw) / 2

                if max_moves < 10:
                    return None

                m = max_moves - 5
                if random:
                    m = self.rng.randint(max_moves - 5)

                planes = np.zeros((14 * 19 * 19), dtype=np.int32)
                next_move = goplanes.planes_from_bytes(raw.tobytes(), planes, m)
                planes = planes.reshape((14, 19, 19)).astype(np.int32)

                assert not np.isnan(planes).any()

                return [planes, int(next_move), int(max_moves)]
            super(GameDecoder, self).__init__(df, func)


class DihedralGroup(imgaug.ImageAugmentor):
    def __init__(self):
        super(DihedralGroup, self).__init__()
        self._init(locals())

    def _get_augment_params(self, img):
        return self.rng.randint(8)

    def _augment(self, img, op):
        # ddihedralgroup D4 = <a, b> where a^2=1, b^4=1
        if op >= 4:
            img = img[:, ::-1, :]
            op -= 4
        # no mirror, just rot
        img = np.array([np.rot90(i, k=op) for i in img]).astype(np.int32)
        return img


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--lmdb', type=str, help='path to lmdb', default='/tmp/')
    parser.add_argument('--pattern', type=str, help='pattern of binary sgf files',
                        default='/tmp/out/*.sgfbin')
    parser.add_argument('--action', type=str, help='action', choices=['create', 'debug', 'benchmark'])
    args = parser.parse_args()

    if args.action == 'create':
        assert args.lmdb is not ''
        assert args.pattern is not ''

        files = glob.glob(args.pattern)
        print('found %i files' % len(files))
        file_idx = list(range(len(files)))
        np.random.shuffle(file_idx)

        split = int(len(file_idx) * 0.9)

        train_idx = file_idx[:split]
        validate_idx = file_idx[split:]

        print('use %i/%i split' % (len(train_idx), len(validate_idx)))

        # train data
        df = GoGamesFromDir([files[i] for i in train_idx])
        dftools.dump_dataflow_to_lmdb(df, args.lmdb + 'go_train.lmdb')

        df = GoGamesFromDir([files[i] for i in validate_idx])
        dftools.dump_dataflow_to_lmdb(df, args.lmdb + 'go_val.lmdb')

    if args.action == 'benchmark':
        df = LMDBDataPoint(args.lmdb, shuffle=False)
        df = GameDecoder(df)
        df.reset_state()
        TestDataSpeed(df, size=50000).start_test()

    if args.action == 'debug':
        df = LMDBDataPoint(args.lmdb, shuffle=False)
        df = GameDecoder(df)
        df.reset_state()

        for dp in df.get_data():
            planes = dp[0]
            bboard = np.zeros((19, 19), dtype=str)
            bboard[planes[0, :, :] == 1] = 'o'
            bboard[planes[1, :, :] == 1] = 'x'
            bboard[planes[2, :, :] == 1] = '.'

            for i in range(19):
                print " ".join(bboard[i, :])
            print "feature-shape", planes.shape
