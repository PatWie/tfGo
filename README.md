# A small Go-Game implementation

# data
We provide a script to download a few publicity available go games in the SGF format (plain text). 

        ./get_data.sh

To handle these games efficiently, we convert them to a custom binary format, where each move/set is represented as 16bits and can be read easier in C++. 

# features

        cd go-engine && python setup.py install

To provide the feature-planes