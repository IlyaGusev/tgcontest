#!/bin/bash

# Models
mkdir models
cd models
wget -O lang_detect.ftz https://dl.fbaipublicfiles.com/fasttext/supervised-models/lid.176.ftz
wget -O news_detect.ftz https://www.dropbox.com/s/squ9rnm6a4ixkza/news_detect.ftz
wget -O cat_detect.ftz https://www.dropbox.com/s/3xwj6rxsk6eykcr/cat_detect.ftz
cd ..

# Datasets

data_list=(
    "https://data-static.usercontent.dev/DataClusteringSample0107.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringSample0817.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringSample1821.tar.gz"
)

mkdir data
for url in ${data_list[@]}; do
    wget -qO - $url | tar -xvz -C data
done
