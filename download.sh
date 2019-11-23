#!/bin/bash

# Models
mkdir models
cd models
wget -O lang_detect.ftz https://dl.fbaipublicfiles.com/fasttext/supervised-models/lid.176.ftz
wget -O news_detect.ftz https://www.dropbox.com/s/squ9rnm6a4ixkza/news_detect.ftz
cd ..

# Datasets
mkdir data
cd data

wget https://data-static.usercontent.dev/DataClusteringSample0107.tar.gz
tar -xzvf DataClusteringSample0107.tar.gz
rm DataClusteringSample0107.tar.gz

wget https://data-static.usercontent.dev/DataClusteringSample0817.tar.gz
tar -xzvf DataClusteringSample0817.tar.gz
rm DataClusteringSample0817.tar.gz

cd ..
