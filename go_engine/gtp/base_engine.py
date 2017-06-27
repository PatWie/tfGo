import sys


class BaseEngine(object):
    """docstring for BaseEngine"""
    def __init__(self, name):
        super(BaseEngine, self).__init__()
        self.name = name
        self.online = True

        self.cmds = [field[5:] for field in dir(self) if field.startswith("call_")]

    def call_protocol_version(self, args):
        return "= 2"

    def call_version(self, args):
        return "= 0.0.1"

    def call_name(self, args):
        return "= " + self.name

    def call_list_commands(self, args):
        return "= " + "\n".join(self.cmds)

    def call_boardsize(self, args):
        return "= "

    def call_clear_board(self, args):
        return "= "

    def call_play(self, args):
        return "= "

    def call_quit(self, arguments):
        self.disconnect = True
        return "= "

    def handle_cmd(self, cmd):
        try:
            s = cmd.split(' ')
            cmd = s[0]
            args = s[1:]
            args = [x for x in args if x]
        except:
            cmd = cmd
            args = ''

        if cmd in self.cmds:
            ans = getattr(self, "call_" + cmd)(args)
            return ans

    def msg_loop(self):
        while self.online:
            inpt = raw_input()
            try:
                recv = inpt.split("\n")
            except:
                recv = [inpt]
            for cmd in recv:
                ans = self.handle_cmd(cmd)
                sys.stdout.write('%s\n' % ans)
                sys.stdout.flush()
                sys.stdout.write("\n")
                sys.stdout.flush()

if __name__ == '__main__':
    eng = BaseEngine('test')
    eng.msg_loop()
