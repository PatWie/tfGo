#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Patrick Wieschollek <mail@patwie.com>


import tensorpack as tp
from tensorpack import *
import glob
import numpy as np
import argparse
from go_engine import goplanes


class GoGamesFromDir(tp.dataflow.DataFlow):
    """Yield frame triplets for phase 0
    """
    def __init__(self, pattern):
        super(GoGamesFromDir, self).__init__()
        self.files = glob.glob(pattern)
        self.len = len(self.files)

    def get_data(self):
        file_idx = list(range(len(self.files)))

        for file_id in file_idx:
            raw = np.fromfile(self.files[file_id], dtype=np.int8)
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
        def __init__(self, df):
            def func(dp):
                raw = dp[0]
                max_moves = len(raw) / 2

                planes = np.zeros((14 * 19 * 19), dtype=np.int32)
                goplanes.planes_from_bytes(raw.tobytes(), planes, min(50, max_moves - 1))
                planes = planes.reshape((14, 19, 19))

                return [planes, max_moves]
            super(GameDecoder, self).__init__(df, func)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--lmdb', type=str, help='path to lmdb', default='/tmp/')
    parser.add_argument('--pattern', type=str, help='pattern of binary sgf files',
                        default='/tmp/out/*.sgfbin')
    parser.add_argument('--action', type=str, help='action', choices=['create', 'debug'])
    args = parser.parse_args()

    if args.action == 'create':
        assert args.lmdb is not ''
        assert args.pattern is not ''

        files = glob.glob(args.pattern)
        file_idx = list(range(len(files)))
        np.random.shuffle(file_idx)

        split = int(len(file_idx) * 0.8)

        train_idx = file_idx[:split]
        validate_idx = file_idx[split:]

        # train data
        df = GoGamesFromDir(files[train_idx])
        dftools.dump_dataflow_to_lmdb(df, args.lmdb + 'go_train.lmdb')

        df = GoGamesFromDir(files[validate_idx])
        dftools.dump_dataflow_to_lmdb(df, args.lmdb + 'go_val.lmdb')

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
