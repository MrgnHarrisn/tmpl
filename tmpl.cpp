#include <iostream>
#include <filesystem>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

#if defined(_WIN32) || defined(_WIN64)
    #define OS_WINDOWS
    #include <windows.h>
#endif

/*
Template Manager (tmpl):
A command-line tool for saving, creating, listing, and deleting file system templates with tag support.

Usage:
  tmpl save <template_name> <directory_to_save> [--tags tag1,tag2,...]
      - Saves the contents of the specified directory as a template with optional tags.

  tmpl make <template_name> <destination>
      - Creates a new project from the specified template in the given destination directory.

  tmpl list [--tags tag1,tag2,...]
      - Lists all available templates, optionally filtering by tags.

  tmpl delete <template_name>
      - Deletes the specified template.

  tmpl tag add|remove <template_name> <tag1,tag2,...>
      - Adds or removes tags from a specified template.

  tmpl help
      - Displays help instructions.

  tmpl version
      - Displays the version of the program.
*/

namespace fs = std::filesystem;

#define VERSION "1.0.3"

// Function to get the user's home directory in a cross-platform way
fs::path get_home_directory() {
#ifdef OS_WINDOWS
    const char* home_drive = getenv("HOMEDRIVE");
    const char* home_path = getenv("HOMEPATH");
    if (home_drive && home_path) {
        return fs::path(std::string(home_drive) + std::string(home_path));
    } else if (const char* user_profile = getenv("USERPROFILE")) {
        return fs::path(user_profile);
    } else {
        std::cerr << "Unable to determine home directory." << std::endl;
        exit(EXIT_FAILURE);
    }
#else
    if (const char* home = getenv("HOME")) {
        return fs::path(home);
    } else {
        std::cerr << "Unable to determine home directory." << std::endl;
        exit(EXIT_FAILURE);
    }
#endif
}

// Directory where templates are stored
const fs::path TEMPLATE_DIR = get_home_directory() / ".templates";

/**
 * @brief Reads tags from a template's .meta file.
 *
 * @param template_path Path to the template directory.
 * @return A vector of tags.
 */
std::vector<std::string> read_tags(const fs::path& template_path) {
    std::vector<std::string> tags;
    std::ifstream meta_file(template_path / ".meta");
    if (meta_file) {
        std::string line;
        while (std::getline(meta_file, line)) {
            if (!line.empty() && line.substr(0, 5) == "Tags:") {
                std::string tags_str = line.substr(5); // Remove "Tags:" prefix
                std::stringstream ss(tags_str);
                std::string tag;
                while (std::getline(ss, tag, ',')) {
                    tag.erase(std::remove_if(tag.begin(), tag.end(), ::isspace), tag.end()); // Trim whitespace
                    tags.push_back(tag);
                }
            }
        }
    }
    return tags;
}

/**
 * @brief Writes tags to a template's .meta file.
 *
 * @param template_path Path to the template directory.
 * @param tags A vector of tags to write.
 */
void write_tags(const fs::path& template_path, const std::vector<std::string>& tags) {
    std::ofstream meta_file(template_path / ".meta");
    if (!tags.empty()) {
        meta_file << "Tags:";
        for (size_t i = 0; i < tags.size(); ++i) {
            meta_file << tags[i];
            if (i < tags.size() - 1)
                meta_file << ",";
        }
        meta_file << std::endl;
    }
}

/**
 * @brief Custom recursive copy function that excludes .meta files.
 *
 * @param src Source path.
 * @param dst Destination path.
 */
void copy_template(const fs::path& src, const fs::path& dst) {
    fs::create_directories(dst);
    for (const auto& entry : fs::directory_iterator(src)) {
        const auto& path = entry.path();
        auto relative_path = fs::relative(path, src);
        auto dst_path = dst / relative_path;

        if (entry.is_directory()) {
            copy_template(path, dst_path);
        } else if (entry.is_regular_file()) {
            if (path.filename() == ".meta") {
                continue; // Skip copying .meta files
            }
            fs::copy_file(path, dst_path, fs::copy_options::overwrite_existing);
        }
    }
}

/**
 * @brief Saves the contents of a directory as a new template with optional tags.
 *
 * @param t_name Name of the template to save.
 * @param src_dir Path to the directory to be saved as a template.
 * @param tags Optional vector of tags to associate with the template.
 */
void save_template(const std::string& t_name, const std::string& src_dir, const std::vector<std::string>& tags = {}) {
    fs::path template_path = TEMPLATE_DIR / t_name;
    if (fs::exists(template_path)) {
        std::cerr << "Template with that name already exists!\n";
        return;
    }

    fs::create_directories(template_path); // Create the template directory if it doesn't exist
    fs::copy(src_dir, template_path, fs::copy_options::recursive | fs::copy_options::directories_only); // Copy directory structure

    // Copy files individually to exclude .meta files
    for (const auto& entry : fs::recursive_directory_iterator(src_dir)) {
        const auto& path = entry.path();
        auto relative_path = fs::relative(path, src_dir);
        auto dst_path = template_path / relative_path;

        if (entry.is_regular_file()) {
            fs::copy_file(path, dst_path, fs::copy_options::overwrite_existing);
        }
    }

    if (!tags.empty()) {
        write_tags(template_path, tags);
    }

    std::cout << "Template saved successfully!\n";
}

/**
 * @brief Creates a new project from a saved template.
 *
 * @param t_name Name of the template to use.
 * @param dest Destination directory where the new project will be created.
 */
void make_project(const std::string& t_name, const std::string& dest) {
    if (!fs::exists(TEMPLATE_DIR)) {
        std::cout << "No templates found in: " << TEMPLATE_DIR << std::endl;
        return;
    }

    fs::path template_path = TEMPLATE_DIR / t_name;
    if (!fs::exists(template_path)) {
        std::cout << "Template '" << t_name << "' does not exist.\n";
        return;
    }

    if (fs::exists(dest)) {
        std::cout << "Folder already exists with the name: " << dest << std::endl;
        return;
    }

    fs::path dest_path = fs::current_path() / dest; // Destination path

    // Use custom copy function to exclude .meta files
    copy_template(template_path, dest_path);

    std::cout << "Template created successfully!\n";
}

/**
 * @brief Lists all saved templates, optionally filtering by tags.
 *
 * @param filter_tags Optional vector of tags to filter templates.
 */
void list_templates(const std::vector<std::string>& filter_tags = {}) {
    if (!fs::exists(TEMPLATE_DIR) || !fs::is_directory(TEMPLATE_DIR) || fs::is_empty(TEMPLATE_DIR)) {
        std::cout << "No templates found in " << TEMPLATE_DIR << std::endl;
        return;
    }

    std::cout << "Available templates in \"" << TEMPLATE_DIR.string() << "\"\n";
    for (const auto& entry : fs::directory_iterator(TEMPLATE_DIR)) {
        if (entry.is_directory()) {
            std::string template_name = entry.path().filename().string();
            std::vector<std::string> tags = read_tags(entry.path());

            // If filter_tags is not empty, check if template has any of the tags
            bool show_template = true;
            if (!filter_tags.empty()) {
                // Check if any of the filter_tags are in tags
                show_template = false;
                for (const auto& tag : filter_tags) {
                    if (std::find(tags.begin(), tags.end(), tag) != tags.end()) {
                        show_template = true;
                        break;
                    }
                }
            }
            if (show_template) {
                std::cout << "- " << template_name;
                if (!tags.empty()) {
                    std::cout << " [Tags: ";
                    for (size_t i = 0; i < tags.size(); ++i) {
                        std::cout << tags[i];
                        if (i < tags.size() - 1)
                            std::cout << ", ";
                    }
                    std::cout << "]";
                }
                std::cout << std::endl;
            }
        }
    }
}

/**
 * @brief Deletes a specified template.
 *
 * @param template_n Name of the template to delete.
 */
void delete_template(const std::string& template_n) {
    if (!fs::exists(TEMPLATE_DIR / template_n) || !fs::is_directory(TEMPLATE_DIR / template_n)) {
        std::cout << "Template doesn't exist!\n";
        return;
    }
    fs::remove_all(TEMPLATE_DIR / template_n); // Remove directory and its contents
    std::cout << "Template deleted successfully!\n";
}

/**
 * @brief Adds tags to an existing template.
 *
 * @param t_name Name of the template.
 * @param tags Vector of tags to add.
 */
void add_tags_to_template(const std::string& t_name, const std::vector<std::string>& tags) {
    fs::path template_path = TEMPLATE_DIR / t_name;
    if (!fs::exists(template_path) || !fs::is_directory(template_path)) {
        std::cout << "Template does not exist.\n";
        return;
    }
    // Read existing tags
    std::vector<std::string> existing_tags = read_tags(template_path);
    // Add new tags if not already present
    for (const auto& tag : tags) {
        if (std::find(existing_tags.begin(), existing_tags.end(), tag) == existing_tags.end()) {
            existing_tags.push_back(tag);
        }
    }
    // Write back tags
    write_tags(template_path, existing_tags);
    std::cout << "Tags added successfully.\n";
}

/**
 * @brief Removes tags from an existing template.
 *
 * @param t_name Name of the template.
 * @param tags Vector of tags to remove.
 */
void remove_tags_from_template(const std::string& t_name, const std::vector<std::string>& tags) {
    fs::path template_path = TEMPLATE_DIR / t_name;
    if (!fs::exists(template_path) || !fs::is_directory(template_path)) {
        std::cout << "Template does not exist.\n";
        return;
    }
    // Read existing tags
    std::vector<std::string> existing_tags = read_tags(template_path);
    // Remove tags if present
    for (const auto& tag : tags) {
        existing_tags.erase(std::remove(existing_tags.begin(), existing_tags.end(), tag), existing_tags.end());
    }
    // Write back tags
    write_tags(template_path, existing_tags);
    std::cout << "Tags removed successfully.\n";
}

/**
 * @brief Prints the help menu for the program.
 */
void print_help() {
    printf("Usage:\n");
    printf("  save   \t\ttmpl save <template_name> <directory_to_save> [--tags tag1,tag2,...]\n");
    printf("  make   \t\ttmpl make <template_name> <new_directory_name>\n");
    printf("  list   \t\ttmpl list [--tags tag1,tag2,...]\n");
    printf("  delete \t\ttmpl delete <template_name>\n");
    printf("  tag    \t\ttmpl tag add|remove <template_name> <tag1,tag2,...>\n");
    printf("  help   \t\ttmpl help\n");
    printf("  version\t\ttmpl version\n");
}

/**
 * @brief Parses tags from a comma-separated string.
 *
 * @param tags_arg String containing comma-separated tags.
 * @return A vector of tags.
 */
std::vector<std::string> parse_tags(const std::string& tags_arg) {
    std::vector<std::string> tags;
    std::stringstream ss(tags_arg);
    std::string tag;
    while (std::getline(ss, tag, ',')) {
        tag.erase(std::remove_if(tag.begin(), tag.end(), ::isspace), tag.end()); // Trim whitespace
        if (!tag.empty()) {
            tags.push_back(tag);
        }
    }
    return tags;
}

/**
 * @brief Main entry point of the program.
 */
int main(int argc, char* argv[]) {
    if (argc <= 1) {
        printf("Invalid usage. For help, run:\ntmpl help\n");
        return -1;
    }

    if (std::strcmp(argv[1], "save") == 0) {
        if (argc >= 4) {
            std::string template_name = argv[2];
            std::string directory_to_save = argv[3];
            std::vector<std::string> tags;
            if (argc >= 6 && std::strcmp(argv[4], "--tags") == 0) {
                tags = parse_tags(argv[5]);
            }
            save_template(template_name, directory_to_save, tags);
        } else {
            printf("Invalid number of arguments for 'save'.\n");
            return -1;
        }

    } else if (std::strcmp(argv[1], "list") == 0) {
        std::vector<std::string> filter_tags;
        if (argc >= 4 && std::strcmp(argv[2], "--tags") == 0) {
            filter_tags = parse_tags(argv[3]);
        }
        list_templates(filter_tags);

    } else if (std::strcmp(argv[1], "help") == 0) {
        print_help();

    } else if (std::strcmp(argv[1], "version") == 0) {
        std::cout << "Version: " << VERSION << "\n";

    } else if (std::strcmp(argv[1], "make") == 0) {
        if (argc == 4) {
            make_project(argv[2], argv[3]);
        } else {
            std::cout << "Invalid number of arguments for 'make'.\n";
            return -1;
        }

    } else if (std::strcmp(argv[1], "delete") == 0) {
        if (argc == 3) {
            delete_template(argv[2]);
        } else {
            std::cout << "Invalid number of arguments for 'delete'.\n";
            return -1;
        }

    } else if (std::strcmp(argv[1], "tag") == 0) {
        if (argc == 5) {
            std::string action = argv[2];
            std::string template_name = argv[3];
            std::vector<std::string> tags = parse_tags(argv[4]);

            if (action == "add") {
                add_tags_to_template(template_name, tags);
            } else if (action == "remove") {
                remove_tags_from_template(template_name, tags);
            } else {
                std::cout << "Unknown action for 'tag' command. Use 'add' or 'remove'.\n";
                return -1;
            }
        } else {
            std::cout << "Invalid number of arguments for 'tag'.\n";
            return -1;
        }

    } else {
        printf("Unknown command. For help, run:\ntmpl help\n");
        return -1;
    }

    return 0;
}
