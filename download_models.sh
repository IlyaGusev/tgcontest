#!/bin/bash

models_list=(
    "https://www.dropbox.com/s/qq970kin8zagql7/lang_detect.ftz"
    "https://www.dropbox.com/s/sejjqu4vu0p31wz/ru_cat_v3.ftz"
    "https://www.dropbox.com/s/z5szjputp35a6yu/en_cat_v2.ftz"
    "https://www.dropbox.com/s/2nx97d8nzbzusee/ru_vectors_v2.bin"
    "https://www.dropbox.com/s/no7x1n8acl5ykif/en_vectors_v2.bin"
    "https://www.dropbox.com/s/0o9xr2pwuqeh17k/pagerank_rating.txt"
)

mkdir -p models
cd models
for url in ${models_list[@]}; do
    echo $url
    wget -nc -q $url
done
