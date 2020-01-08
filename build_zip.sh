mkdir temp
cd temp
cp -R ../src src
cp -R ../models models
cp -R ../scripts scripts
cp -R ../thirdparty thirdparty
rm -rf thirdparty/fasttext/.git
rm -rf thirdparty/eigen/.git
cp ../LICENSE LICENSE
cp ../NOTICE NOTICE
cp ../README.md README.md
cp ../CMakeLists.txt CMakeLists.txt
cp ../deb-packages.txt deb-packages.txt
cp ../build_zip.sh build_zip.sh
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cd ..
mv build/tgnews tgnews
chmod +x tgnews
cp tgnews ../tgnews
zip -r submission.zip src scripts models thirdparty LICENSE NOTICE README.md CMakeLists.txt deb-packages.txt tgnews build_zip.sh
mv submission.zip ../submission.zip
cd ..
rm -rf temp
