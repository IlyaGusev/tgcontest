# TGNews

[![Build Status](https://travis-ci.com/IlyaGusev/tgcontest.svg?token=9pgxYSDpb2YAVSfz53Nq&branch=master)](https://travis-ci.com/IlyaGusev/tgcontest)

## Install
Prerequisites: CMake, Boost
```
$ sudo apt-get install cmake libboost-all-dev build-essential
```

If you got zip archive, just go to building binary

Install [git-lfs](https://git-lfs.github.com/) before cloning repo. To download code and models:
```
$ git clone https://github.com/IlyaGusev/tgcontest
$ cd tgcontest
$ git submodule init
$ git submodule update
```

To build binary (in "tgcontest" dir):
```
$ mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

To download datasets:
```
$ bash download.sh
```

Run on sample:
```
./build/tgnews top data --ndocs 10000
```

## Training

Russian fasttext vectors training: [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1QeyhqsHy5MO3yzvsn446LsqK_PqOjIVb)

FastText installation (for training):
```
$ git clone https://github.com/facebookresearch/fastText.git
$ cd fastText
$ mkdir build && cd build && cmake ..
$ make && make install
```

Classifier training and compression:
```
$ fasttext supervised -input markup/ru_cat_all_train.txt -output models/my_model -lr 1.0 -epoch 50 -minCount 15 -pretrainedVectors models/ru_tg_lenta_vector_model.vec -dim 50
$ fasttext quantize -input markup/ru_cat_all_train.txt -output models/my_model -qnorm
```

Or just use autotune options
