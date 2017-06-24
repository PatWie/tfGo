import os
import logging
import sys
import glob
import argparse
import numpy as np
logging.basicConfig(stream=sys.stdout, level=logging.ERROR)

logger = logging.getLogger('sgfreader')


class SGFMeta(object):
    """docstring for SGFMeta"""
    def __init__(self, tokens):
        super(SGFMeta, self).__init__()
        self.tokens = tokens
        self.info = dict()

        keys = ['SZ', 'PW', 'WR', 'PB', 'BR', 'FF',
                'EV', 'RO', 'DT', 'PC', 'KM', 'RE',
                'GC', 'US', 'RU', 'WT', 'BT', 'TM']

        for k in keys:
            self.info[k] = ''

        for k, v in tokens:
            if k in keys:
                self.info[k] = str(v)

    def summarize(self):
        d = self.info
        print '%s (%s) vs. %s (%s)' % (d['PW'], d['WR'], d['PB'], d['BR'])
        print 'on %s board size %s' % (d['EV'], d['SZ'])
        print 'comment: %s' % (d['GC'])
        print 'has timelimit: %s' % (d['TM'] is not '')

    @property
    def correct(self):
        if self.info['SZ'] != '19':
            return False
        if 'illegal' in self.info['GC'].lower():
            return False
        if 'corrupt' in self.info['GC'].lower():
            return False
        if 'time' in self.info['RE'].lower():
            return False
        if 'resign' in self.info['RE'].lower():
            return False
        return True


class SGFReader(object):
    """docstring for SGFReader"""
    def __init__(self, fn):
        assert os.path.isfile(fn)
        self.fn = fn
        super(SGFReader, self).__init__()
        with open(fn, 'r') as f:
            self.content = f.read()
        self.meta = None
        self.moves = []
        self.move_len = 0
        self.result = ''
        self.parse()

    @property
    def amateur(self):
        return 'amateur' in self.content.lower()

    def parse(self):
        separators = set(['(', ')', '\n', '\r', '\t', ';', ']'])

        tokens = []
        k, v, is_key = '', '', True
        for c in self.content:
            if c in separators:
                if k is not '' and v is not '':
                    tokens.append((k.strip(), v.strip()))
                k, v, is_key = '', '', True
                continue

            if c == '[':
                is_key = False
                continue

            if is_key:
                k += str(c)
            else:
                v += str(c)

        """
        Move Properties             B, KO, MN, W
        Setup Properties            AB, AE, AW, PL
        Node Annotation Properties  C, DM, GB, GW, HO, N, UC, V
        Move Annotation Properties  BM, DO, IT, TE
        Markup Properties           AR, CR, DD, LB, LN, MA, SL, SQ, TR
        Root Properties             AP, CA, FF, GM, ST, SZ
        Game Info Properties        AN, BR, BT, CP, DT, EV, GN, GC, ON, OT, PB, PC, PW, RE, RO, RU, SO, TM, US, WR, WT
        Timing Properties           BL, OB, OW, WL
        Miscellaneous Properties    FG, PM, VW
        """

        valid_keys = ['B', 'KO', 'MN', 'W',
                      'AB', 'AE', 'AW', 'PL',
                      'C', 'DM', 'GB', 'GW', 'HO', 'N', 'UC', 'V',
                      'BM', 'DO', 'IT', 'TE',
                      'AR', 'CR', 'DD', 'LB', 'LN', 'MA', 'SL', 'SQ', 'TR',
                      'AP', 'CA', 'FF', 'GM', 'ST', 'SZ',
                      'AN', 'BR', 'BT', 'CP', 'DT', 'EV', 'GN', 'GC', 'ON', 'OT', 'PB', 'PC', 'PW', 'RE', 'RO', 'RU', 'SO', 'TM', 'US', 'WR', 'WT',  # noqa
                      'BL', 'OB', 'OW', 'WL',
                      'FG', 'PM', 'VW',
                      'KM', 'OH', 'HA', 'MULTIGOGM']  # unkown

        for k, v in tokens:
            if k not in valid_keys:
                logger.warn('{} is not a valid key in {}'.format(k, self.fn))
                logger.warn('{}'.format(v))
                sys.exit(0)

            if k == 'KO':
                logger.error('illegal move in %s' % self.fn)

            if k in ['AB', 'AW', 'B', 'w']:
                self.moves.append((k, v))
            if k in ['B', 'w']:
                self.move_len += 1

            if k == 'RE':
                # parse result of game
                if v.strip() == '0':
                    self.result = 'drawn'
                if 'drawn' in v.strip().lower():
                    self.result = 'drawn'
                if 'jigo' in v.strip().lower():
                    self.result = 'drawn'
                if 't+' in v.strip().lower():
                    logger.error('time resign')
                ok = True
                if not v.strip().lower().startswith('b'):
                    if not v.strip().lower().startswith('w'):
                        # logger.error(v)
                        ok = False
                if ok:
                    if v.strip().lower().startswith('b'):
                        self.result = 'black'
                    else:
                        self.result = 'white'

        self.meta = SGFMeta(tokens)
        # print fn


def debug(pattern):
    corrupted_files = []
    good_files = []
    amateur_files = []
    total_moves = 0

    files = glob.glob(pattern)
    for fn in files:
        if '1700-99' in fn:
            corrupted_files.append(fn)
            logger.warn('{} is too old'.format(fn))
            continue
        if '0196-1699' in fn:
            corrupted_files.append(fn)
            logger.warn('{} is too old'.format(fn))
            continue

        s = SGFReader(fn)
        if s.meta.correct:
            if s.amateur:
                amateur_files.append(fn)
            else:
                # the interesting ones
                total_moves += s.move_len
                good_files.append(fn)
        else:
            logger.warn('{} is not correct'.format(fn))
            logger.info('--> %s' % s.meta.info['GC'])
            corrupted_files.append(fn)

    def out_of(caption, needle, haystack):
        print '%i out of %i are %s ~%f%%' % (len(needle), len(files), caption, len(needle) / float(len(haystack)) * 100)

    out_of('corrupt', corrupted_files, files)
    out_of('amateur', amateur_files, files)

    print 'total moves %i' % total_moves
    print 'avg moves %i' % (total_moves / float(len(good_files)))


def encode(color, x, y, move=False, pass_move=False):
    """Encode all moves into 16bits (easier to read in cpp and binary!!)

    ---pmcyyyyyxxxxx (4bits left for additional information)

    x encoded column
    y encoded row
    c color [w=1, b=0]
    m isemove [move=1, add=0]
    p ispass  [pass=1, nopass=0]

    TODO:
        handle "pass move"

    Args:
        color (TYPE): Description
        x (TYPE): Description
        y (TYPE): Description
        move (bool, optional): Description

    Returns:
        TYPE: Description
    """
    assert color in ['W', 'B']
    # assert x < 19
    # assert y < 19

    value = 32 * y + x
    if color == 'W':
        value += 1024
    if move:
        value += 2048
    if pass_move:
        value += 4096

    byte1 = value % (2**8)
    byte2 = (value - byte1) / (2**8)

    return [byte2, byte1]


def convert(pattern):
    files = glob.glob(pattern)
    for fn in files:
        if '1700-99' in fn:
            logger.warn('{} is too old'.format(fn))
            continue
        if '0196-1699' in fn:
            logger.warn('{} is too old'.format(fn))
            continue

        s = SGFReader(fn)
        if s.meta.correct:
            if not s.amateur:
                # the interesting ones
                bin_content = []
                for k, v in s.moves:

                    is_set = False
                    if k in ['AB', 'AW']:
                        is_set = True

                    if k in ['B', 'AB']:
                        color = 'B'
                    else:
                        color = 'W'

                    if len(v.strip()) == 0 or v.strip().lower() == 'tt':
                        # pass
                        bin_content += encode(color, 0, 0, move=False, pass_move=True)
                    else:
                        if len(v) != 2:
                            print k, v, fn
                        else:
                            # ok move
                            x, y = v.lower()
                            x, y = ord(x) - ord('a'), ord(y) - ord('a')
                            bin_content += encode(color, x, y, move=not is_set, pass_move=False)
                yield [bin_content, fn]
        else:
            logger.warn('{} is not correct'.format(fn))
            logger.info('--> %s' % s.meta.info['GC'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--action', help='', default='debug', type=str)
    parser.add_argument('--pattern', help='', default='/home/patwie/godb/Database/*/*.sgf')
    args = parser.parse_args()

    if args.action == 'debug':
        debug(args.pattern)
        sys.exit(0)

    if args.action == 'count':
        total = len(glob.glob(args.pattern + 'bin'))
        val_len = total // 10
        train_len = total - val_len

        print total, train_len, val_len
        sys.exit(0)

    if args.action == 'convert':
        for bin_content, fn in convert(args.pattern):
            # print bin_content, fn
            bin_content = np.array(bin_content).astype(np.uint8)
            with open('%sbin' % fn, 'wb') as f:
                f.write(bin_content.tobytes())

            # print bin_content
        sys.exit(0)
