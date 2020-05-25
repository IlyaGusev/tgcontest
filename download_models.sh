#!/bin/bash

models_list=(
    "https://www.dropbox.com/s/qq970kin8zagql7/lang_detect.ftz"
    "https://www.dropbox.com/s/23x35wuet280eh6/ru_cat_v5.ftz"
    "https://www.dropbox.com/s/luh60dd0uw8p9ar/en_cat_v5.ftz"
    "https://www.dropbox.com/s/vttjivmmxw7leea/ru_vectors_v3.bin"
    "https://www.dropbox.com/s/6aaucelizfx7xl6/en_vectors_v3.bin"
    "https://www.dropbox.com/s/0o9xr2pwuqeh17k/pagerank_rating.txt"
    "https://www.dropbox.com/s/fry1gsd1mans9jm/alexa_rating_4_fixed.txt"
    "https://www.dropbox.com/s/hoapmnvqlknmu6v/lang_detect_v10.ftz"
)

cd models
for url in ${models_list[@]}; do
    echo $url
    wget -nc -q $url
done
