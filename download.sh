#!/bin/bash

data_list=(
    "https://data-static.usercontent.dev/DataClusteringSample0107.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringSample0817.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringSample1821.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringSample2225.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringDataset.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringDataset1209.tar.gz"
)

mkdir data
for url in ${data_list[@]}; do
    echo $url
    wget -qO - $url | tar -xz -C data
done
