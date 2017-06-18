import argparse
import numpy as np

READING_NAME = 1
READING_DATA = 2

separators = set(['(', ')', ' ', '\n', '\r', '\t', ';'])

properties_taking_lists = set(['AB',  # add black stone (handicap)
                               'AW',  # add white stone (handicap)
                               ])


def fetch_key(file_data, ptr):
    while file_data[ptr] in separators:
        ptr += 1
        if ptr >= len(file_data):
            return (None, ptr)
    name = ''
    while file_data[ptr] != '[':
        name += file_data[ptr]
        ptr += 1
    return (name, ptr)


def fetch_value(file_data, ptr):
    while file_data[ptr].isspace():
        ptr += 1
    if file_data[ptr] != '[':
        return (None, ptr)
    ptr += 1
    data = ''
    while file_data[ptr] != ']':
        data += file_data[ptr]
        ptr += 1
    ptr += 1
    return (data, ptr)


def fetch_list(file_data, ptr):
    data_list = []
    while True:
        (data, ptr) = fetch_value(file_data, ptr)
        if data is None:
            return (data_list, ptr)
        else:
            data_list.append(data)


def parse_vertex(s):
    if len(s) == 0:
        return None  # pass
    if s == "tt":  # GoGoD sometimes uses this to indicate a pass
        return None  # We are sacrificing >19x19 support here
    x = ord(s[0]) - ord('a')
    y = ord(s[1]) - ord('a')
    return (x, y)


class SGFParser:
    def __init__(self, filename):
        with open(filename, 'r') as f:
            self.file_data = f.read()
        self.ptr = 0

    def __iter__(self):
        return self

    def next(self):
        (k, self.ptr) = fetch_key(self.file_data, self.ptr)
        if k is None:
            raise StopIteration
        elif k in properties_taking_lists:
            (v, self.ptr) = fetch_list(self.file_data, self.ptr)
        else:
            (v, self.ptr) = fetch_value(self.file_data, self.ptr)
        return (k, v)


def encode(color, x, y, move=False):
    """Encode all moves into 16bits

    ----mcyyyyyxxxxx

    x encoded column
    y encoded row
    c color [w=1, b=0]
    m isemove [move=1, add=0]

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

    value = 2**5 * y + x
    if color == 'W':
        value += 2**10
    if move:
        value += 2**11

    byte1 = value % (2**8)
    byte2 = (value - byte1) / (2**8)

    # print '-->', format(value, '#018b')
    # print '-->    ', format(byte1, '#010b'), byte1
    # print '-->    ', format(byte2, '#010b'), byte2
    # return bytearray([byte1, byte2])
    # print value, byte1, byte2, "x", x, "y", y, 'is_white', (color == 'W')
    # print'is_move', move, format(value, '#018b'), format(byte1, '#010b'), format(byte2, '#010b')
    # return bytearray([np.uint8(byte1), np.uint8(byte2)])
    return [byte2, byte1]


def convert2bin(sgf):

    out = sgf + '.bin'
    parser = SGFParser(sgf)
    content = [p for p in parser]

    for k, v in content:
        if k == 'GM':
            if int(v) is not 1:
                print 'missmatch GM'
                return
        if k == 'SZ':
            if int(v) is not 19:
                print 'missmatch SZ', v
                return

    bin_content = []
    # alright lets convert
    for k, v in content:
        if k == 'AB':
            for m in v:
                print "no move"
                x, y = ord(m[0]) - ord('a'), ord(m[1]) - ord('a')
                bin_content += encode('B', x, y, False)
        if k == 'AW':
            for m in v:
                print "no move"
                x, y = ord(m[0]) - ord('a'), ord(m[1]) - ord('a')
                bin_content += encode('W', x, y, False)

        if k in ['B', 'W']:
            print "move"
            x, y = ord(v[0]) - ord('a'), ord(v[1]) - ord('a')
            bin_content += encode(k, x, y, True)

    bin_content = np.array(bin_content).astype(np.uint8)

    with open(out, 'wb') as f:
        f.write(bin_content.tobytes())


def main(sgf):
    convert2bin(sgf)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--sgf', help='path to sgf file', required=True)
    args = parser.parse_args()
    main(args.sgf)
