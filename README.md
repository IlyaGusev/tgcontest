# TGNews

[![Build Status](https://travis-ci.com/IlyaGusev/tgcontest.svg?token=9pgxYSDpb2YAVSfz53Nq&branch=master)](https://travis-ci.com/IlyaGusev/tgcontest)

## Links
* Description in English: https://medium.com/@phoenixilya/news-aggregator-in-2-weeks-5b38783b95e3
* Description in Russian: https://habr.com/ru/post/487324/

## Demo
* Russian: [https://ilyagusev.github.io/tgcontest/ru/main.html](https://ilyagusev.github.io/tgcontest/ru/main.html)
* English: [https://ilyagusev.github.io/tgcontest/en/main.html](https://ilyagusev.github.io/tgcontest/en/main.html)

## Install
Prerequisites: CMake, Boost
```
$ sudo apt-get install cmake libboost-all-dev build-essential libjsoncpp-dev uuid-dev protobuf-compiler libprotobuf-dev
```

For MacOS
```
$ brew install boost jsoncpp ossp-uuid protobuf
```


If you got zip archive, just go to building binary

To download code and models:
```
$ git clone https://github.com/IlyaGusev/tgcontest
$ cd tgcontest
$ git submodule update --init --recursive
$ bash download_models.sh
$ wget https://download.pytorch.org/libtorch/cpu/libtorch-cxx11-abi-shared-with-deps-1.5.0%2Bcpu.zip
$ unzip libtorch-cxx11-abi-shared-with-deps-1.5.0+cpu.zip
```

For MacOS use https://download.pytorch.org/libtorch/cpu/libtorch-macos-1.5.0.zip

To build binary (in "tgcontest" dir):
```
$ mkdir build && cd build && Torch_DIR="../libtorch" cmake -DCMAKE_BUILD_TYPE=Release .. && make -j4
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
* XLM-RoBERTa pseudo-labeling for categorization: [![Open In Colab](https://colab.research.google.com/assets/colab-badge.svg)](https://colab.research.google.com/drive/1fEmNPi41mnxLrc4hqamqi72xDCCH0Ima)

## Models
* Language detection model (2 round): [lang_detect_v10.ftz](https://www.dropbox.com/s/hoapmnvqlknmu6v/lang_detect_v10.ftz)
* Russian FastText vectors (2 round): [ru_vectors_v3.bin](https://www.dropbox.com/s/vttjivmmxw7leea/ru_vectors_v3.bin)
* Russian categories detection model (2 round): [ru_cat_v5.ftz](https://www.dropbox.com/s/luh60dd0uw8p9ar/en_cat_v5.ftz)
* English FastText vectors (2 round): [en_vectors_v3.bin](https://www.dropbox.com/s/6aaucelizfx7xl6/en_vectors_v3.bin)
* English categories detection model (2 round): [en_cat_v5.ftz](https://www.dropbox.com/s/luh60dd0uw8p9ar/en_cat_v5.ftz)
* PageRank-based agency rating: [pagerank_rating.txt](https://www.dropbox.com/s/0o9xr2pwuqeh17k/pagerank_rating.txt)
* Alexa agency rating: [alexa_rating_4_fixed.txt](https://www.dropbox.com/s/fry1gsd1mans9jm/alexa_rating_4_fixed.txt)
* XLM-RoBERTa for categorization (pytorch-lightning checkpoint): [xlmr_en_ru_cat_v1.tar.gz](https://www.dropbox.com/s/y6leppzf2l43eqo/xlmr_en_ru_cat_v1.tar.gz)

## Data
* Russian news from all archives, except 1117: [ru_tg_1101_0510.jsonl.tar.gz](https://www.dropbox.com/s/r8iqi6h6x1w0pzv/ru_tg_1101_0510.jsonl.tar.gz)
* Russian news from 1117 archive: [ru_tg_0511_0517.jsonl.tar.gz](https://www.dropbox.com/s/zvv1qvm1yidvc2p/ru_tg_0511_0517.jsonl.tar.gz)
* English news from 1821, 2225, 29 and 09 archives: [en_tg_test.tar.gz](https://www.dropbox.com/s/rw674iic8x5udb3/en_tg_test.tar.gz)
* Data for training Russian vectors: [ru_unsupervised_train.tar.gz](https://www.dropbox.com/s/gsn9fire2hdaz81/ru_unsupervised_train.tar.gz)
* Data for training English vectors: [en_unsupervised_train.tar.gz](https://www.dropbox.com/s/7c8ey9sqomiqsas/en_unsupervised_train.tar.gz)

## Markup
* Russian categories raw train markup: [ru_cat_v4_train_raw_markup.tsv](https://www.dropbox.com/s/24rsyxxp00kxjzr/ru_cat_v4_train_raw_markup.tsv)
* Russian categories aggregated train markup: [ru_cat_v4_train_annot.json](https://www.dropbox.com/s/2rpsabep7tstmkq/ru_cat_v4_train_annot.json)
* Russian categories aggregated train markup in fastText format: [ft_ru_cat_v4_train.txt](https://www.dropbox.com/s/tdz4k44o0jmrpi5/ft_ru_cat_v4_train.txt)
* Russian categories manual train markup: [ru_cat_v4_train_manual_annot.json](https://www.dropbox.com/s/fibw7remhk2bodl/ru_cat_v4_train_manual_annot.json)
* Russian categoreis manual train markup in fastText format: [ft_ru_cat_v4_train_manual.txt](https://www.dropbox.com/s/y9jg50rck1pg1w1/ft_ru_cat_v4_train_manual.txt)
* Russian categoreis raw test markup: [ru_cat_v4_test_raw_markup.tsv](https://www.dropbox.com/s/9cbubupcht00kqn/ru_cat_v4_test_raw_markup.tsv)
* Russian categories aggregated test markup: [ru_cat_v4_test_annot.json](https://www.dropbox.com/s/ur7jhiyi22tmzxd/ru_cat_v4_test_annot.json)
* Russian categories aggregated test markup in fastText format: [ft_ru_cat_v4_test.txt](https://www.dropbox.com/s/89opmh9alx7tfy3/ft_ru_cat_v4_test.txt)
* English categories aggregated train markup: [en_cat_v4_train_annot.json](https://www.dropbox.com/s/fysoyx1mz8rf6rs/en_cat_v4_train_annot.json)
* English categories aggregated train markup in fastText format: [ft_en_cat_v4_train.txt](https://www.dropbox.com/s/7a2k2tmkf61nsks/ft_en_cat_v4_train.txt)
* English categories aggregated test markup: [en_cat_v4_test_annot.json](https://www.dropbox.com/s/ucwzhucwgtuy8k1/en_cat_v4_test_annot.json)
* English categories aggregated test markup in fastText format: [ft_en_cat_v4_test.txt](https://www.dropbox.com/s/yga8i06hqv0pvqc/ft_en_cat_v4_test.txt)
* Russian clustering pairs: [ru_pairs_raw_markup.tsv](https://www.dropbox.com/s/jugcl80vfd4wg0h/ru_pairs_raw_markup.tsv)
* English clustering pairs: [en_pairs_raw_markup.tsv](https://www.dropbox.com/s/1zs05c3frm8cygq/en_pairs_raw_markup.tsv)

## Misc
* Flamegraph: https://ilyagusev.github.io/tgcontest/flamegraph.svg

## Other contestants
* Round 2
  * II place
    * Daring Frog: https://github.com/a-l-e-x-k/data_clustering_contest, article: https://medium.com/@alexkuznetsov/2nd-place-solution-for-telegram-data-clustering-contest-f28d55b98d30
    * Swift Skunk: https://github.com/sorrge/tg_news_cluster
  * III place
    * Mindful Kitten: https://danlark.org/2020/07/31/news-aggregator-from-scratch-in-2-weeks/
  * IV place
    * Bossy Gnu: https://github.com/maxoodf/tgnews
  * Other:
    * Large Crab: https://github.com/ilya-ustinov/tgcontest
* Round 1
  * III place
    * Kooky Dragon: https://github.com/nick-baliesnyi/tgnews
  * IV place
    * Sharp Sloth: https://github.com/thehemen/telegram-data-clustering
  * Other
    * Desert Python: https://github.com/crazyleg/telegram_data_clustering_2019
    * Funky Peacock: https://github.com/Stepka/telegram_clustering_contest
    * Unknown animal: https://github.com/roman-rybalko/telegram-data-clustering-contest
    * Unknown animal: https://github.com/MarcoBuster/data-clustering-contest
    * Unknown animal: https://github.com/sudevschiz/tgnews
    * Unknown animal: https://github.com/crazyleg/telegram_data_clustering_2019
    * Unknown animal: https://github.com/77ph/tgnews
    * Unknown animal: https://github.com/akash-joshi/telegram-cluster
    * Unknown animal: https://github.com/dremovd/telegram-clustering

## Contacts
* Telegram: [@YallenGusev](https://t.me/YallenGusev)
