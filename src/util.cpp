#include "util.h"

#include <boost/filesystem.hpp>

void ReadFileNames(const std::string& directory, std::vector<std::string>& fileNames, int nDocs) {
    boost::filesystem::path dirPath(directory);
    boost::filesystem::recursive_directory_iterator start(dirPath);
    boost::filesystem::recursive_directory_iterator end;
    for (auto it = start; it != end; it++) {
        if (boost::filesystem::is_directory(it->path())) {
            continue;
        }
        std::string path = it->path().string();
        if (path.substr(path.length() - 5) == ".html") {
            fileNames.push_back(path);
        }
        if (nDocs != -1 && fileNames.size() == static_cast<size_t>(nDocs)) {
            break;
        }
    }
}
