from base_engine import BaseEngine
from bridge import GTPbridge


class GnuGoEngine(BaseEngine):
    """docstring for GnuGoEngine"""
    def __init__(self, name):
        super(GnuGoEngine, self).__init__(name)
        self.bridge = GTPbridge(name, ["gnugo", "--mode", "gtp"], verbose=False)

    def call_show_board(self, args):
        return self.bridge.send('showboard\n')

    def call_version(self, args):
        return self.bridge.send('version\n')

    def call_boardsize(self, args):
        boardsize = int(args[0])
        return self.bridge.send("boardsize {}\n".format(boardsize))

    def call_genmove(self, args):
        color = args[0]
        return self.bridge.send("genmove {}\n".format(color))

    def call_final_score(self, args):
        return self.bridge.send("final_score\n")

    def call_play(self, args):
        color, move = args[:2]
        return self.bridge.send("play {} {}\n".format(color, move))


if __name__ == '__main__':
    eng = GnuGoEngine('test')
    eng.msg_loop()
