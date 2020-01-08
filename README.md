# TGNews

[![Build Status](https://travis-ci.com/IlyaGusev/tgcontest.svg?token=9pgxYSDpb2YAVSfz53Nq&branch=master)](https://travis-ci.com/IlyaGusev/tgcontest)

## Install
Prerequisites: CMake, Boost
```
$ sudo apt-get install cmake libboost-all-dev build-essential
```

If you got zip archive, just go to building binary

To download code and models:
```
$ git clone https://github.com/IlyaGusev/tgcontest
$ cd tgcontest
$ git submodule init
$ git submodule update
$ bash download_models.sh
```

To build binary (in "tgcontest" dir):
```
$ mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

To download datasets:
```
$ bash download_data.sh
```

Run on sample:
```
./build/tgnews top data --ndocs 10000
```

## Training

Russian fasttext vectors training: [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1QeyhqsHy5MO3yzvsn446LsqK_PqOjIVb)

Russian fasttext category classifier training: [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1U7Wxm5eDnrBRWE_logCSJIq6DzTFV0Zo)

Russian sentence embedder training: [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1ZqSUP51J1xbVk2VxyZwDhpW3VKKok4sx)

English fasttext vectors training: [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1lbmgJ_iGBdwKdkU_1l1-WZuO7XbYZlWQ)

English fasttext category classifier training: [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1ayg5dtA_KdhzVehN4-_EiyIcwRhBVSob)

English sentence embedder training: [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1QDescCBI2I7bCJr4EplTyxCOp7eD9qBS)

PageRank rating calculation: [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1bd35S0rl_Uysiuz_7fmkYRArzNcP-wZB)
