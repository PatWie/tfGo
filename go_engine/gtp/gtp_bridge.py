from subprocess import Popen, PIPE


class GTPbridge(object):
    def __init__(self, name, pipe_args, verbose=True):
        self.name = name
        self.verbose = verbose
        self.subprocess = Popen(pipe_args, stdin=PIPE, stdout=PIPE)
        print("created pipe for {}".format(name))

    def send(self, data):
        if self.verbose:
            print("[{}] receives '{}'".format(self.name, data.replace('\n', '')))
        self.subprocess.stdin.write(data)
        result = ""
        while True:
            data = self.subprocess.stdout.readline()
            if not data.strip():
                break
            result += data
        if self.verbose:
            print("[{}] returns {}".format(self.name, result))
        return result

    def close(self):
        print("quitting {} subprocess".format(self.label))
        self.subprocess.communicate("quit\n")
