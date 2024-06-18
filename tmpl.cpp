#include <iostream>
#include <filesystem>
#include <string>
#include <cstring>

/*
tmpl save <template_name>
tmpl make <template_name> <destination>
tmpl list
tmpl delete <template_name>
*/

namespace fs = std::filesystem;
const fs::path TEMPLATE_DIR = fs::path(getenv("HOME")) / ".templates";
void save_template(const std::string& t_name, const std::string& src_dir)
{
    fs::path template_path = TEMPLATE_DIR / t_name;
    if (fs::exists(template_path)) {
        std::cerr << "Template with that name already exists!\n";
        return;
    }

    fs::create_directories(template_path);
    fs::copy(src_dir, template_path, fs::copy_options::recursive);
    std::cout << "Template saved successfully!\n";
}

void make_project(const std::string& t_name, const std::string& dest)
{
    if (!fs::exists(TEMPLATE_DIR)) {
        std::cout << "No templates found in: " << TEMPLATE_DIR << std::endl;
        return;
    }

    if (fs::exists(dest)) {
        std::cout << "Folder already exists with the name: " << dest << std::endl;
        return;
    }

    fs::path location = fs::current_path() / dest;
    fs::copy(TEMPLATE_DIR/t_name, location, fs::copy_options::recursive);
    std::cout << "Template created successfully!\n";

}

void list_templates()
{
    if (!fs::exists(TEMPLATE_DIR) || !fs::is_directory(TEMPLATE_DIR) || fs::is_empty(TEMPLATE_DIR)) {
        std::cout << "No templates found in " << TEMPLATE_DIR << std::endl;
        return;
    }

    std::cout << "Available templates in " << TEMPLATE_DIR << std::endl;
    for (const auto& entry : fs::directory_iterator(TEMPLATE_DIR)) {
        if (entry.is_directory()) {
            std::cout << "- " << entry.path().filename().string() << std::endl;
        }
    }
}

void delete_template(const std::string& template_n)
{
    if (!fs::exists(TEMPLATE_DIR/template_n) || !fs::is_directory(TEMPLATE_DIR/template_n)) {
        std::cout << "Tempalte doesn't exist!\n";
        return;
    }
    fs::remove_all(TEMPLATE_DIR/template_n);
    std::cout << "File deleted successfully!\n";
}

void print_help()
{
    printf("save\t\t\ttmpl save <template_name>\n");
    printf("make\t\t\ttmpl make <template_name> <new_directory_name>\n");
    printf("list\t\t\ttmpl list\n");
    printf("delete\t\t\ttmpl delete <template_name>\n");
}

int main(int argc, char* argv[])
{
    if (argc <= 1) {
        printf("Invalid usage call:\ntmpl help\n");
        return -1;
    }

    if (std::strcmp(argv[1], "save") == 0) {
        if (argc < 4) {
            printf("Invalid number of arguments\n");
            return -1;
        }
        save_template(argv[2], argv[3]);
    } else if (std::strcmp(argv[1], "list") == 0) {
        list_templates();
    } else if (std::strcmp(argv[1], "help") == 0) {
        print_help();
    } else if (std::strcmp(argv[1], "make") == 0) {
        make_project(argv[2], argv[3]);
    } else if (std::strcmp(argv[1], "delete") == 0) {
        delete_template(argv[2]);
    } else {
        printf("Unknown command, use:\ntmpl help\n");
        return -1;
    }

    return 0;
}