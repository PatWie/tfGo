#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Patrick Wieschollek <your@email.com>

import argparse
from tensorpack import *
import tensorflow as tf
from go_db import GameDecoder
from tensorpack.tfutils.symbolic_functions import *
from tensorpack.tfutils.summary import *

"""
Re-Implementation of the Policy-Network from AlphaGo
"""

BATCH_SIZE = 16
SHAPE = 19
CHANNELS = 14


class Model(ModelDesc):
    def _get_inputs(self):
        return [InputDesc(tf.int32, (None, CHANNELS, SHAPE, SHAPE), 'board'),
                InputDesc(tf.int32, (None), 'next_move'),
                InputDesc(tf.int32, (None), 'max_move')]

    def _build_graph(self, inputs):
        board, label, max_moves = inputs
        board = tf.cast(board, tf.float32)

        print label.get_shape()

        k = 128  # match version was 192

        layers = []

        # pad to 23x23
        with argscope([Conv2D], nl=tf.nn.relu, kernel_shape=3, padding='VALID',
                      stride=1, use_bias=False, data_format='NCHW'):
            layers.append(tf.pad(board, [[0, 0], [0, 0], [2, 2], [2, 2]], mode='CONSTANT', name='pad1'))
            layers.append(Conv2D('conv1', layers[-1], k, kernel_shape=5))

            for i in range(2, 12):
                layers.append(tf.pad(layers[-1], [[0, 0], [0, 0], [1, 1], [1, 1]], mode='CONSTANT',
                                     name='pad%i' % i))
                layers.append(Conv2D('conv%i' % i, layers[-1], k))

            logits = Conv2D('conv_final', layers[-1], 1, kernel_shape=1, use_bias=True, nl=tf.identity)

        logits = tf.identity(batch_flatten(logits), name='logits')

        loss = tf.nn.sparse_softmax_cross_entropy_with_logits(logits=logits, labels=label)
        loss = tf.reduce_mean(loss, name='xentropy-loss')

        wrong = prediction_incorrect(logits, label, 1, name='wrong-top1')
        add_moving_summary(tf.reduce_mean(wrong, name='train-error-top1'))

        wrong = prediction_incorrect(logits, label, 5, name='wrong-top5')
        add_moving_summary(tf.reduce_mean(wrong, name='train-error-top5'))

        self.cost = tf.identity(loss, name='total_costs')
        summary.add_moving_summary(self.cost)

    def _get_optimizer(self):
        lr = symbolic_functions.get_scalar_var('learning_rate', 0.003, summary=True)
        return tf.train.AdamOptimizer(lr)


def get_data(lmdb, suffle=False):
    df = LMDBDataPoint(lmdb, shuffle=suffle)
    df = GameDecoder(df)
    df = PrefetchDataZMQ(df, 2)
    df = BatchData(df, BATCH_SIZE)
    return df


def get_config():
    logger.auto_set_dir()

    ds_train = get_data('/tmp/go_train.lmdb', True)
    ds_val = get_data('/tmp/go_val.lmdb', False)

    return TrainConfig(
        model=Model(),
        dataflow=ds_train,
        callbacks=[
            ModelSaver(),

            InferenceRunner(ds_val, [ScalarStats('total_costs')]),
        ],
        steps_per_epoch=ds_train.size(),
        max_epoch=100,
    )


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--gpu', help='comma separated list of GPU(s) to use.', required=True)
    parser.add_argument('--load', help='load model')
    args = parser.parse_args()

    NR_GPU = len(args.gpu.split(','))
    with change_gpu(args.gpu):
        config = get_config()
        config.nr_tower = NR_GPU

        if args.load:
            config.session_init = SaverRestore(args.load)

        SyncMultiGPUTrainer(config).train()
