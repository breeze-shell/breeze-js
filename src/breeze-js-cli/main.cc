#include "script.h"

int main() {
    try {
        auto ctx = std::make_shared<breeze::script_context>();
        auto scripts_folder = std::filesystem::path("scripts");
        if (!std::filesystem::exists(scripts_folder)) {
            std::filesystem::create_directories(scripts_folder);
        }
        ctx->watch_folder(scripts_folder);
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}