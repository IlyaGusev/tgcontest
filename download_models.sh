#!/bin/bash

models_list=(
    "https://www.dropbox.com/s/qq970kin8zagql7/lang_detect.ftz"
    "https://www.dropbox.com/s/9ogedfsbh252dbt/ru_cat_v4.ftz"
    "https://www.dropbox.com/s/kkn63hriknzl1zw/en_cat_v3.ftz"
    "https://www.dropbox.com/s/vttjivmmxw7leea/ru_vectors_v3.bin"
    "https://www.dropbox.com/s/6aaucelizfx7xl6/en_vectors_v3.bin"
    "https://www.dropbox.com/s/0o9xr2pwuqeh17k/pagerank_rating.txt"
    "https://www.dropbox.com/s/e7nktxa0yte7k1d/alexa_rating_2_fixed.txt"
    "https://www.dropbox.com/s/hoapmnvqlknmu6v/lang_detect_v10.ftz"
)

mkdir -p models
cd models
for url in ${models_list[@]}; do
    echo $url
    wget -nc -q $url
done
