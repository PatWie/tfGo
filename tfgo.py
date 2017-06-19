#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Author: Patrick Wieschollek <mail@patwie.com>

import argparse
import os
from tensorpack import *
from go_db import GameDecoder, DihedralGroup
from tensorpack.tfutils.symbolic_functions import *
from tensorpack.tfutils.summary import *
import tensorflow as tf
import multiprocessing

"""
Re-Implementation of the Policy-Network from AlphaGo
"""

BATCH_SIZE = 16
SHAPE = 19
NUM_PLANES = 14


class Model(ModelDesc):

    def __init__(self, k=128):
        self.k = k  # match version was 192

    def _get_inputs(self):
        return [InputDesc(tf.int32, (None, NUM_PLANES, SHAPE, SHAPE), 'board'),
                InputDesc(tf.int32, (None, ), 'next_move'),
                InputDesc(tf.int32, (None, ), 'max_move')]

    def _build_graph(self, inputs):
        board, label, _ = inputs
        board = tf.cast(board, tf.float32)

        def pad(x, p, name):
            return tf.pad(x, [[0, 0], [0, 0], [p, p], [p, p]], mode='CONSTANT', name=name)

        # the policy model from https://gogameguru.com/i/2016/03/deepmind-mastering-go.pdf
        # TODO: test dilated convolutions
        net = board
        with argscope([Conv2D], nl=tf.nn.relu, kernel_shape=3, padding='VALID',
                      stride=1, use_bias=False, data_format='NCHW'):
            net = pad(board, p=2, name='pad1')
            net = Conv2D('conv1', net, out_channel=self.k, kernel_shape=5)

            net = Conv2D('conv2', pad(net, p=1, name='pad2'), out_channel=self.k)
            net = Conv2D('conv3', pad(net, p=1, name='pad3'), out_channel=self.k)
            net = Conv2D('conv4', pad(net, p=1, name='pad4'), out_channel=self.k)
            net = Conv2D('conv5', pad(net, p=1, name='pad5'), out_channel=self.k)
            net = Conv2D('conv6', pad(net, p=1, name='pad6'), out_channel=self.k)
            net = Conv2D('conv7', pad(net, p=1, name='pad7'), out_channel=self.k)
            net = Conv2D('conv8', pad(net, p=1, name='pad8'), out_channel=self.k)
            net = Conv2D('conv9', pad(net, p=1, name='pad9'), out_channel=self.k)
            net = Conv2D('conv10', pad(net, p=1, name='pad10'), out_channel=self.k)
            net = Conv2D('conv11', pad(net, p=1, name='pad11'), out_channel=self.k)
            net = Conv2D('conv12', pad(net, p=1, name='pad12'), out_channel=self.k)

            logits = Conv2D('conv_final', net, 1, kernel_shape=1, use_bias=True, nl=tf.identity)

        prop = tf.nn.softmax(logits, name='probabilities')
        logits = tf.identity(batch_flatten(logits), name='logits')

        loss = tf.nn.sparse_softmax_cross_entropy_with_logits(logits=logits, labels=label)
        self.cost = tf.reduce_mean(loss, name='total_costs')

        acc_top1 = accuracy(logits, label, 1, name='accuracy-top1')
        acc_top5 = accuracy(logits, label, 5, name='accuracy-top5')

        summary.add_moving_summary(acc_top1, acc_top5, self.cost)

        # visualization
        v = tf.expand_dims(board[:, 0, :, :] - board[:, 1, :, :], axis=-1)
        tf.summary.image('board', v, max_outputs=max(30, BATCH_SIZE))

        prop = prop[:, 0, :, :]
        prop = tf.reshape(prop, [-1, SHAPE, SHAPE, 1]) * 255.
        expected = tf.one_hot(label, SHAPE * SHAPE)
        expected = tf.reshape(expected, [-1, SHAPE, SHAPE, 1]) * 255.
        viz = tf.concat([prop, expected], axis=2) * 256
        viz = tf.cast(tf.clip_by_value(viz, 0, 255), tf.uint8, name='viz')
        tf.summary.image('probabilities, expected', viz, max_outputs=max(30, BATCH_SIZE))

    def _get_optimizer(self):
        lr = symbolic_functions.get_scalar_var('learning_rate', 0.003, summary=True)
        return tf.train.GradientDescentOptimizer(lr)


def get_data(lmdb, shuffle=False):
    df = LMDBDataPoint(lmdb, shuffle=True)
    df = GameDecoder(df, random=True)
    df = PrefetchData(df, 5000, 1)
    df = AugmentImageComponents(df, [DihedralGroup()], index=[0])
    df = PrefetchDataZMQ(df, min(20, multiprocessing.cpu_count()))
    df = BatchData(df, BATCH_SIZE)
    return df


def get_config(k):
    logger.set_logger_dir(
        os.path.join('train_log',
                     'tfgo-policy_net-{}'.format(k)))

    df_train = get_data('/scratch_shared/wieschol/go/go_train.lmdb', True)
    df_val = get_data('/scratch_shared/wieschol/go/go_val.lmdb', False)

    return TrainConfig(
        model=Model(k),
        dataflow=df_train,
        callbacks=[
            ModelSaver(),
            MaxSaver('validation_accuracy-top1'),
            MaxSaver('validation_accuracy-top5'),
            InferenceRunner(df_val, [ScalarStats('total_costs'),
                                     ScalarStats('accuracy-top1'),
                                     ScalarStats('accuracy-top5')]),
            HumanHyperParamSetter('learning_rate'),
        ],
        extra_callbacks=[
            MovingAverageSummary(),
            ProgressBar(['tower0/total_costs:0', 'tower0/accuracy-top1:0', 'tower0/accuracy-top5:0']),
            MergeAllSummaries(),
            RunUpdateOps()
        ],
        steps_per_epoch=df_train.size(),
        max_epoch=100,
    )


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--gpu', help='comma separated list of GPU(s) to use.', required=True)
    parser.add_argument('--load', help='load model', default='train_log/tfgo-policy_net-128/checkpoint')
    parser.add_argument('--k', type=int, help='number_of_filters', choices=xrange(1, 256))
    args = parser.parse_args()

    NR_GPU = len(args.gpu.split(','))
    with change_gpu(args.gpu):
        config = get_config(args.k)
        config.nr_tower = NR_GPU

        if args.load:
            config.session_init = SaverRestore(args.load)

        SyncMultiGPUTrainer(config).train()
