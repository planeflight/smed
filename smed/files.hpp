#ifndef SMED_FILES_HPP
#define SMED_FILES_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "omega/core/error.hpp"
#include "omega/util/log.hpp"
#include "omega/util/types.hpp"

class FileExplorer {
  public:
    FileExplorer(const std::string &root) : root(root) {
        change_directory(root);
    }

    void change_directory(const std::string &path) {
        cwd = path;
        cwd_size = 0;
        content.clear();
        // if we're going down a directory remove the ..
        if (path.ends_with("..")) {
            cwd = path.substr(0, path.length() - 2);
            // find second to last index of /
            i32 last_index = path.find_last_of("/");
            if (last_index != std::string::npos) {
                cwd = path.substr(0, last_index);
                // find the actual second to last index of /
                cwd = path.substr(0, cwd.string().find_last_of("/"));
                OMEGA_DEBUG("{}", cwd);
            } else {
                OMEGA_ASSERT(false, "SHOULD NEVER OCCUR");
            }
        }

        for (const auto &entry : std::filesystem::directory_iterator(cwd)) {
            cwd_size++;
            content.push_back(entry.path());
        }
        // add go up if this is not project root
        if (root != cwd) {
            cwd_size++;
            content.push_back(cwd / "..");
        }
    }

    // change directory if it's a directory, otherwise cwd is sam
    bool open(const std::string &path, std::string &content) {
        if (std::filesystem::is_directory(path)) {
            change_directory(path);
            return true;
        }
        cw_file = path;
        std::ifstream ifs(path);
        content = std::string((std::istreambuf_iterator<char>(ifs)),
                              (std::istreambuf_iterator<char>()));
        return false;
    }

    const std::vector<std::string> &get_cwd_ls() const {
        return content;
    }

    u32 get_cwd_size() const {
        return cwd_size;
    }

  private:
    std::filesystem::path root; // project root
    std::filesystem::path cwd;
    std::string cw_file;
    u32 cwd_size = 0;
    std::vector<std::string> content;
};

#endif // SMED_FILES_HPP
