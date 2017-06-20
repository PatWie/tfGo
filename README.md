# Policy-Network (SL) from AlphaGO in TensorFlow

Yet another re-implementation of the policy-network (supervised) from Deepmind's AlphaGo. This uses a C++ backend to compute the feature planes presented in the [Nature-Paper](https://gogameguru.com/i/2016/03/deepmind-mastering-go.pdf) and a custom fileformat for efficient storage. To train the network it uses the dataflow and multi-GPU setup of [TensorPack](https://github.com/ppwwyyxx/tensorpack).

# Fileformat

In contrast to chess (8x8) we cannot store a board configuration into `uin64` datatypes (one for each figure). As computing the GO-features for the NN requires to compute liberties of groups all the time, we store the moves into a binary format and replay them. The best I came up with is 2bytes for each single move:

```
---pmcyyyyyxxxxx

p: is action passed ? [1:yes, 0:no]
m: is action a move ? [1:yes, 0:no] (sgf supports 'set' as well)
c: is action from white ? [1:yes, 0:no] (0 means black ;-) )
y: encoded row (1-19)
x: encoded column (a-s)
-: free bits (maybe we can store something useful here)
```

This gives two nice properties:
- Reading a match and its moves in C++ is absolute easy.
- Computing the length of the game is simply `sizeof(file) / 2` (we just ignore the handicap currently)

# Data + Features

A [modified](https://github.com/TheDuck314/go-NN) version of the script file downloads+extracts a few (>1 million) publicity available GO games encoded in the SGF format (plain text) by:

        ./get_data.sh

The files will be downloaded to `/tmp/data`. To handle these games efficiently, we convert them to binary by

        python convert.py --input /tmp/data/ --out /tmp/out

Now this gives several small files (1681414 files here). Some files do not work and raise an exception (why?). I just skip them To merge all games within a single file, we dump these games to an LMDB file. This also gives a 90%/10% (1513267/168141) train/val split of the games:

        python go_db.py --lmdb /tmp/ --pattern '/tmp/out/*.sgfbin' --action create

So all training data is just (1.1GB) and validation is just 120MB. 
To simulate the board position from the encoded moves, we setup the SWIG-Python binding `goplanes` of the C++ implementation by:

        cd go-engine && python setup.py install --user

Reading some feature-planes (currently I only implemented 47 of out 49) from random moves extracted from the db gives a speed of 13794.03 examples/s.

        python go_db.py --action benchmark --lmdb /tmp/go_train.lmdb

To see the actual board positions:

        python go_db.py --action debug --lmdb /tmp/go_train.lmdb

# Training 

To train the version with `128` filters just fire up. 

        python tfgo.py --gpu 0,1 --k 128 --path /tmp # or --gpu 0 for single gpu

This might take a some some some ... time. The output is 

         64%|#######4               |60951/94579[11:08<06:11,20.46it/s], total_costs=4.5, accuracy-top1=0.125, accuracy-top5=0.312

during training. As I saw no big different on a small number of GPUS, this uses the Sync-Training rather than any Async-Training.

It will also create checkpoints for the best performing models from the validation phase.