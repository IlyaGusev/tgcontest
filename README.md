# tgcontest

Trello: https://trello.com/b/36OkBvly/tgcontest

Wiki: https://github.com/IlyaGusev/tgcontest/wiki

Prerequisites: CMake, Boost, MLPack
```
$ sudo apt-get install cmake libboost-all-dev libmlpack-dev
```

To build Armadillo and MLPack from source:
```
$ wget http://sourceforge.net/projects/arma/files/armadillo-9.800.2.tar.xz
$ tar -xvf armadillo-9.800.2.tar.xz
$ mkdir armadillo-9.800.2/build && cd armadillo-9.800.2/build
$ cmake ../
$ sudo make install
$ cd ../../

$ wget https://www.mlpack.org/files/mlpack-3.2.1.tar.gz
$ tar -xvzpf mlpack-3.2.1.tar.gz
$ mkdir mlpack-3.2.1/build && cd mlpack-3.2.1/build
$ cmake ../
$ make -j4
$ sudo make install
```

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
$ mkdir build && cd build && cmake ..
$ make
```

Run on sample:
```
./tgnews threads ../data/ --lang_detect_model ../models/lang_detect.ftz --news_detect_model ../models/news_detect.ftz  --cat_detect_model ../models/ru_cat_detect.ftz --languages ru --vector_model ../models/tg_lenta.bin --clustering_distance_threshold 0.05
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
