#include <iostream>
#include <filesystem>
#include <string>
#include <cstring>

/*
Template Manager (tmpl):
A command-line tool for saving, creating, listing, and deleting file system templates.

Usage:
  tmpl save <template_name> <directory_to_save>
      - Saves the contents of the specified directory as a template.

  tmpl make <template_name> <destination>
      - Creates a new project from the specified template in the given destination directory.

  tmpl list
      - Lists all available templates.

  tmpl delete <template_name>
      - Deletes the specified template.

  tmpl help
      - Displays help instructions.
*/

namespace fs = std::filesystem;

// Directory where templates are stored
const fs::path TEMPLATE_DIR = fs::path(getenv("HOME")) / ".templates";

/**
 * @brief Saves the contents of a directory as a new template.
 *
 * @param t_name Name of the template to save.
 * @param src_dir Path to the directory to be saved as a template.
 *
 * @details
 * - Checks if a template with the given name already exists. If so, prints an error message.
 * - Creates a new directory for the template inside TEMPLATE_DIR and copies the source directory into it.
 */
void save_template(const std::string& t_name, const std::string& src_dir)
{
    fs::path template_path = TEMPLATE_DIR / t_name;
    if (fs::exists(template_path)) {
        std::cerr << "Template with that name already exists!\n";
        return;
    }

    fs::create_directories(template_path); // Create the template directory if it doesn't exist
    fs::copy(src_dir, template_path, fs::copy_options::recursive); // Copy all files and subdirectories
    std::cout << "Template saved successfully!\n";
}

/**
 * @brief Creates a new project from a saved template.
 *
 * @param t_name Name of the template to use.
 * @param dest Destination directory where the new project will be created.
 *
 * @details
 * - Ensures TEMPLATE_DIR exists and contains the specified template.
 * - Ensures the destination directory does not already exist.
 * - Copies the template files to the destination directory.
 */
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

    fs::path location = fs::current_path() / dest; // Destination path
    fs::copy(TEMPLATE_DIR / t_name, location, fs::copy_options::recursive); // Copy template files
    std::cout << "Template created successfully!\n";
}

/**
 * @brief Lists all saved templates.
 *
 * @details
 * - Checks if TEMPLATE_DIR exists and is not empty.
 * - Iterates through the directory to list all subdirectories (templates).
 */
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

/**
 * @brief Deletes a specified template.
 *
 * @param template_n Name of the template to delete.
 *
 * @details
 * - Checks if the specified template exists and is a directory.
 * - Deletes the template directory and all its contents.
 */
void delete_template(const std::string& template_n)
{
    if (!fs::exists(TEMPLATE_DIR / template_n) || !fs::is_directory(TEMPLATE_DIR / template_n)) {
        std::cout << "Template doesn't exist!\n";
        return;
    }
    fs::remove_all(TEMPLATE_DIR / template_n); // Remove directory and its contents
    std::cout << "Template deleted successfully!\n";
}

/**
 * @brief Prints the help menu for the program.
 *
 * @details
 * - Displays usage instructions for all available commands.
 */
void print_help()
{
    printf("Usage:\n");
    printf("  save   \t\ttmpl save <template_name> <directory_to_save>\n");
    printf("  make   \t\ttmpl make <template_name> <new_directory_name>\n");
    printf("  list   \t\ttmpl list\n");
    printf("  delete \t\ttmpl delete <template_name>\n");
    printf("  help   \t\ttmpl help\n");
}

/**
 * @brief Main entry point of the program.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 *
 * @details
 * - Parses command-line arguments and invokes the appropriate function.
 * - Handles invalid or unknown commands with error messages and usage hints.
 */
int main(int argc, char* argv[])
{
    if (argc <= 1) {
        printf("Invalid usage. For help, run:\ntmpl help\n");
        return -1;
    }

    if (std::strcmp(argv[1], "save") == 0) {
        if (argc == 4) {
            save_template(argv[2], argv[3]);
        } else {
            printf("Invalid number of arguments for 'save'.\n");
            return -1;
        }

    } else if (std::strcmp(argv[1], "list") == 0) {
        list_templates();

    } else if (std::strcmp(argv[1], "help") == 0) {
        print_help();

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

    } else {
        printf("Unknown command. For help, run:\ntmpl help\n");
        return -1;
    }

    return 0;
}
