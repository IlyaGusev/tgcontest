#!/bin/bash

# Models
mkdir models
cd models
wget -O lang_detect.ftz https://dl.fbaipublicfiles.com/fasttext/supervised-models/lid.176.ftz
cd ..

# Datasets
mkdir data
cd data
wget https://data-static.usercontent.dev/DataClusteringSample0107.tar.gz
tar -xzvf DataClusteringSample0107.tar.gz
wget https://data-static.usercontent.dev/DataClusteringSample0817.tar.gz
tar -xzvf DataClusteringSample0817.tar.gz
cd ..
