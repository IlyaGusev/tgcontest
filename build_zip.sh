mkdir temp
cd temp
cp -R ../src src
cp -R ../models models
cp -R ../scripts scripts
cp -R ../thirdparty thirdparty
cp -R ../configs configs
cp -R ../test test
cp -R ../libtorch libtorch
rm -rf thirdparty/fasttext/.git
rm -rf thirdparty/eigen/.git
rm -rf thirdparty/drogon/.git
rm -rf thirdparty/onmt_tokenizer/.git
rm -rf thirdparty/rocksdb/.git
cp ../LICENSE LICENSE
cp ../NOTICE NOTICE
cp ../README.md README.md
cp ../CMakeLists.txt CMakeLists.txt
cp ../download_data.sh download_data.sh
cp ../download_models.sh download_models.sh
cp ../test_canonical.sh test_canonical.sh
cp ../deb-packages.txt deb-packages.txt
cp ../build_zip.sh build_zip.sh
cp ../tgnews.sh tgnews
mkdir build
cd build
Torch_DIR="../libtorch" cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
cd ..
chmod +x build/tgnews
chmod +x tgnews
zip -r submission.zip src scripts models thirdparty configs test libtorch LICENSE NOTICE README.md CMakeLists.txt deb-packages.txt tgnews build_zip.sh download_data.sh download_models.sh test_canonical.sh
mv submission.zip ../submission.zip && cd ..
rm -rf temp
