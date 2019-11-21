# tgcontest

Trello: https://trello.com/b/36OkBvly/tgcontest

Wiki: https://github.com/IlyaGusev/tgcontest/wiki

FastText installation (for training):
```
$ git clone https://github.com/facebookresearch/fastText.git
$ cd fastText
$ mkdir build && cd build && cmake ..
$ make && make install
```

To build binary:
```
$ git clone https://github.com/IlyaGusev/tgcontest.git
$ cd tgcontest
$ mkdir build && cd build && cmake ..
$ make
```

To download models and datasets:
```
sh download.sh
```
