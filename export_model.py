from tfgo import Model
from tensorpack.tfutils.export import ModelExport

e = ModelExport(Model(), ['board_plhdr'], ['probabilities'])
# e.export('train_log/tfgo-policy_net-1280625-160602/model-4922888', 'export')
e.export('tfgo-policy_net-1280626-222421/checkpoint', 'export')
# e.export('train_log/tfgo-policy_net-128/checkpoint', 'export')
