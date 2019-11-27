# tgcontest

Trello: https://trello.com/b/36OkBvly/tgcontest

Wiki: https://github.com/IlyaGusev/tgcontest/wiki

Prerequisites: Boost
```
$ sudo apt-get install libboost-all-dev
```

To download code and models:
```
$ git clone https://github.com/IlyaGusev/tgcontest
$ cd tgcontest
$ git lfs fetch
```

To download datasets:
```
$ sh download.sh
```

To build binary (in "tgcontest" dir):
```
$ git submodule init
$ git submodule update
$ mkdir build && cd build && cmake ..
$ make
```

Run on sample:
```
./tgnews languages ../data --lang_detect_model ../models/lang_detect.ftz --cat_detect_model ../models/cat_detect.ftz --news_detect_model ../models/news_detect.ftz --ndocs 1000 --languages ru en
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
