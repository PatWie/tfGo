from tfgo import Model
from tensorpack.tfutils.export import ModelExport

e = ModelExport(Model(), ['board_plhdr'], ['probabilities'])
e.export('train_log/tfgo-policy_net-128/checkpoint', 'export')
