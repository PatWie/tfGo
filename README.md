# TF-GO (Policy-Network from AlphaGO)

Yet another re-implementation of the policy-network from Deepmind's AlphaGo. But different to [go-NN](https://github.com/TheDuck314/go-NN). I use a C++ backend to compute the feature planes presented in the [Nature-paper](https://gogameguru.com/i/2016/03/deepmind-mastering-go.pdf) and a custom fileformat for efficient storage.

# Fileformat

In contrast to chess (8x8) we cannot store a board configuration into `uin64` datatypes (one for each figure). As GO requires to compute liberties all the time, we store the moves into a binary format. The best I came up with is 16bits (2bytes) for each move:

```
---pmcyyyyyxxxxx

p: is passed ? [1:yes, 0:no]
m: is move ? [1:yes, 0:no] (sgf supports 'set' as well)
c: is white ? [1:yes, 0:no] (0 means black ;-) )
y: encoded row (1-19)
x: encoded column (a-s)
-: free bits (maybe we can store something useful here)
```

This gives serveral nice properties:
- reading in C++ is absolute easy
- computing the length of the game is simply `sizeof(file) / 2`

TODO: store result as quantized float ?

# Data + Features

I provide a modified script to download+extract a few (>1 million) publicity available GO games in the SGF format (plain text) by:

        ./get_data.sh

The files are located in `/tmp/data`. To handle these games efficiently, we convert them to the custom binary format by

        python convert.py --input /tmp/data/ --out /tmp/out

Now this gives severalsmall files (1681414 files here). Some files do not work and raise an exception (why?). To merge all games within a single file, we dump these games to an LMDB file. This also gives a 90%/10% (1513267/168141) train/val split of the games:

        python go_db.py --lmdb /tmp/ --pattern '/tmp/out/*.sgfbin' --action create

To simulate the board position from the encoded moves, we setup the goplanes C++ implementation by:

        cd go-engine && python setup.py install --user

So all training data is just (1.1GB) and validation is just 120MB. Generating some feature-planes (currently I only implemented 14 due to time reasons) from random moves from random games gives a speed of 13794.03it/s.

        python go_db.py --action benchmark --lmdb /tmp/go_train.lmdb

To see the actual board positions:

        python go_db.py --action debug --lmdb /tmp/go_train.lmdb

# Training 

        python tfgo.py --gpu 0  # or 0,1,... for multi-gpu