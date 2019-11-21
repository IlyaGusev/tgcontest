# tgcontest

Trello: https://trello.com/b/36OkBvly/tgcontest

Wiki: https://github.com/IlyaGusev/tgcontest/wiki

To download models and datasets:
```
$ sh download.sh
```

To build binary:
```
$ git clone https://github.com/IlyaGusev/tgcontest.git
$ cd tgcontest
$ git submodule init
$ git submodule update
$ mkdir build && cd build && cmake ..
$ make
```

Run on sample:
```
./tgnews languages ../data --lang_detect_model ../models/lang_detect.ftz --ndocs 1000 
```

FastText installation (for training):
```
$ git clone https://github.com/facebookresearch/fastText.git
$ cd fastText
$ mkdir build && cd build && cmake ..
$ make && make install
```
