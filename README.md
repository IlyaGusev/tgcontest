# TGNews

[![Build Status](https://travis-ci.com/IlyaGusev/tgcontest.svg?token=9pgxYSDpb2YAVSfz53Nq&branch=master)](https://travis-ci.com/IlyaGusev/tgcontest)

## Links
* Description in English: https://medium.com/@phoenixilya/news-aggregator-in-2-weeks-5b38783b95e3
* Description in Russian: https://habr.com/ru/post/487324/

## Demo
* Russian: [https://ilyagusev.github.io/tgcontest2/ru/main.html](https://ilyagusev.github.io/tgcontest2/ru/main.html)
* English: [https://ilyagusev.github.io/tgcontest2/en/main.html](https://ilyagusev.github.io/tgcontest2/en/main.html)

## Install
Prerequisites: CMake, Boost
```
$ sudo apt-get install cmake libboost-all-dev build-essential libjsoncpp-dev uuid-dev protobuf-compiler
```

For MacOS
```
$ brew install boost jsoncpp ossp-uuid protobuf
```


If you got zip archive, just go to building binary

To download code and models:
```
$ git clone https://github.com/IlyaGusev/tgcontest2
$ cd tgcontest2
$ git submodule update --init --recursive
$ bash download_models.sh
$ wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.5.0%2Bcpu.zip
$ unzip libtorch-cxx11-abi-shared-with-deps-1.5.0+cpu.zip
```

For MacOS use https://download.pytorch.org/libtorch/cpu/libtorch-macos-1.5.0.zip

To build binary (in "tgcontest2" dir):
```
$ mkdir build && cd build && Torch_DIR="../libtorch" cmake -DCMAKE_BUILD_TYPE=Release .. && make
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

* Russian FastText vectors training:
[VectorsRu.ipynb](https://github.com/IlyaGusev/tgcontest/blob/master/scripts/VectorsRu.ipynb)
[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1QeyhqsHy5MO3yzvsn446LsqK_PqOjIVb)
* Russian fasttext category classifier training:
[CatTrainRu.ipynb](https://github.com/IlyaGusev/tgcontest/blob/master/scripts/CatTrainRu.ipynb)
[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1U7Wxm5eDnrBRWE_logCSJIq6DzTFV0Zo)
* Russian sentence embedder training:
[SimilarityRu.ipynb](https://github.com/IlyaGusev/tgcontest/blob/master/scripts/SimilarityRu.ipynb)
[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1ZqSUP51J1xbVk2VxyZwDhpW3VKKok4sx)
* English FastText vectors training:
[VectorsEn.ipynb](https://github.com/IlyaGusev/tgcontest/blob/master/scripts/VectorsEn.ipynb)
[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1lbmgJ_iGBdwKdkU_1l1-WZuO7XbYZlWQ)
* English fasttext category classifier training:
[CatTrainEn.ipynb](https://github.com/IlyaGusev/tgcontest/blob/master/scripts/CatTrainEn.ipynb)
[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1ayg5dtA_KdhzVehN4-_EiyIcwRhBVSob)
* English sentence embedder training:
[SimilarityEn.ipynb](https://github.com/IlyaGusev/tgcontest/blob/master/scripts/SimilarityEn.ipynb)
[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1QDescCBI2I7bCJr4EplTyxCOp7eD9qBS)
* PageRank rating calculation:
[PageRankRating.ipynb](https://github.com/IlyaGusev/tgcontest/blob/master/scripts/PageRankRating.ipynb)
[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1bd35S0rl_Uysiuz_7fmkYRArzNcP-wZB)

* Russian **ELMo-based** sentence embedder training:
[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1Q0S5OvramxxqQZnaSIH8xWfmOsWeKhIz)
* Russian sentence embedder with **triplet loss** training:
[![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1G-1GWGsfL5ariy_87FhadMUPDi9OkX_B)

## Models
* Language detection model: [lang_detect.ftz](https://www.dropbox.com/s/qq970kin8zagql7/lang_detect.ftz)
* Russian FastText vectors: [ru_vectors_v2.bin](https://www.dropbox.com/s/2nx97d8nzbzusee/ru_vectors_v2.bin)
* Russian categories detection model: [ru_cat_v2.ftz](https://www.dropbox.com/s/55vonqnblz6ng28/ru_cat_v2.ftz)
* English FastText vectors: [en_vectors_v2.bin](https://www.dropbox.com/s/no7x1n8acl5ykif/en_vectors_v2.bin)
* English categories detection model: [en_cat_v2.ftz](https://www.dropbox.com/s/z5szjputp35a6yu/en_cat_v2.ftz)
* PageRank-based agency rating: [pagerank_rating.txt](https://www.dropbox.com/s/0o9xr2pwuqeh17k/pagerank_rating.txt)

## Data
* Russian news from 0107 and 0817 archives: [ru_tg_train.tar.gz](https://www.dropbox.com/s/1ecl9orr2tagcgi/ru_tg_train.tar.gz)
* English news from 0107 and 0817 archives: [en_tg_train.tar.gz](https://www.dropbox.com/s/umd8tyx4wz1wquq/en_tg_train.tar.gz)
* Russian news from 1821, 2225, 29 and 09 archives: [ru_tg_test.tar.gz](https://www.dropbox.com/s/gvfk6t4g7kxw9ae/ru_tg_test.tar.gz)
* English news from 1821, 2225, 29 and 09 archives: [en_tg_test.tar.gz](https://www.dropbox.com/s/rw674iic8x5udb3/en_tg_test.tar.gz)
* Data for training Russian vectors: [ru_unsupervised_train.tar.gz](https://www.dropbox.com/s/gsn9fire2hdaz81/ru_unsupervised_train.tar.gz)
* Data for training English vectors: [en_unsupervised_train.tar.gz](https://www.dropbox.com/s/7c8ey9sqomiqsas/en_unsupervised_train.tar.gz)

## Markup
* Russian categories train markup: [ru_cat_train_raw_markup.tsv](https://www.dropbox.com/s/amua7p1rt1dcvy0/ru_cat_train_raw_markup.tsv)
* Russian categories test markup: [ru_cat_test_raw_markup.tsv](https://www.dropbox.com/s/xia50d1h28e87x4/ru_cat_test_raw_markup.tsv)
* Russian not_news additional markup: [ru_not_news.txt](https://www.dropbox.com/s/wwptzqhgxvtjhbd/ru_not_news.txt)
* English categories train markup: [en_cat_train_raw_markup.tsv](https://www.dropbox.com/s/7qpfgf8bz77h2ss/en_cat_train_raw_markup.tsv)
* English categories test markup: [en_cat_test_raw_markup.tsv](https://www.dropbox.com/s/bszwshgwbrt328k/en_cat_test_raw_markup.tsv)

## Misc
* Flamegraph: https://ilyagusev.github.io/tgcontest/flamegraph.svg

## TODO:
* Framework for complex NN
* Proper clustering markup
* Error analysis for categories classifiers
* Alternatives for PageRank
* "Ugly" titles
