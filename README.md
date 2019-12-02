# tgcontest

Trello: https://trello.com/b/36OkBvly/tgcontest

Wiki: https://github.com/IlyaGusev/tgcontest/wiki

Prerequisites: CMake, Boost
```
$ sudo apt-get install cmake libboost-all-dev build-essential
```

If you got zip archive, just go to building binary

Before cloning install git-lfs. To download code and models:
```
$ git clone https://github.com/IlyaGusev/tgcontest
$ cd tgcontest
$ git submodule init
$ git submodule update
```

To download datasets:
```
$ bash download.sh
```

To build binary (in "tgcontest" dir):
```
$ mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

Run on sample:
```
./build/tgnews threads data --ndocs 10000
```

FastText installation (for training):
```
$ git clone https://github.com/facebookresearch/fastText.git
$ cd fastText
$ mkdir build && cd build && cmake ..
$ make && make install
```

Classifier training and compression:
```
$ fasttext supervised -input markup/ru_cat_all_train.txt -output models/my_model -lr 1.0 -epoch 50 -minCount 15 -pretrainedVectors models/tg_lenta.vec -dim 50
$ fasttext quantize -input markup/ru_cat_all_train.txt -output models/my_mode -qnorm
```
