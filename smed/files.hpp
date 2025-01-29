#ifndef SMED_FILES_HPP
#define SMED_FILES_HPP

#include <filesystem>
#include <fstream>
#include <omega/core/error.hpp>
#include <omega/util/log.hpp>
#include <omega/util/types.hpp>
#include <string>
#include <vector>

class FileExplorer {
  public:
    FileExplorer(const std::string &root) : root(root) {
        change_directory(root);
    }

    void set_root(const std::string &r) {
        root = r;
        change_directory(r);
    }

    void change_directory(const std::string &path) {
        // set the new cwd and reset
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
            } else {
                OMEGA_ASSERT(false, "SHOULD NEVER OCCUR");
            }
        }

        // save all the entries in this cwd
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

    const std::string &get_current_file() const {
        return cw_file;
    }

    const std::filesystem::path &get_cwd() const {
        return cwd;
    }

  private:
    std::filesystem::path root; // project root
    std::filesystem::path cwd;
    std::string cw_file;
    u32 cwd_size = 0;
    std::vector<std::string> content;
};

#endif // SMED_FILES_HPP
