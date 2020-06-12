#!/bin/bash

data_train_list=(
    "https://data-static.usercontent.dev/DataClusteringSample0107.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringSample0817.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringSample1821.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringSample2225.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringDataset.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringDataset1209.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringDataset0131.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringDataset0214.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringSample2703.tar.gz"
)

data_test_list=(
    "https://data-static.usercontent.dev/DataClusteringSample0410.tar.gz"
    "https://data-static.usercontent.dev/DataClusteringSample1117.tar.gz"
)

mkdir data
for url in ${data_train_list[@]}; do
    echo $url
    wget -qO - $url | tar -xz -C data
done

mkdir data_test
for url in ${data_test_list[@]}; do
    echo $url
    wget -qO - $url | tar -xz -C data_test
done
