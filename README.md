# A small Go-Game implementation (under work)

This uses tensorpack for training.

# Data + Features

We provide a script to download a few publicity available go games in the SGF format (plain text). 

        ./get_data.sh

This downloads public games to `/tmp/data`. To handle these games efficiently, we convert them to a custom binary format, where each move/set is represented as 16bits and can be read easier in C++ (used for efficient feature computation).

        python convert.py --input /tmp/data/ --out /tmp/out

Now this gives tons of small files (around ). Some files do not work (why?). To collecti all games within a single file, we dump these games to an LMDB file:

        python go-db.py --lmdb /tmo/go.lmdb --pattern '/tmp/out/*.sgfbin'

        cd go-engine && python setup.py install

