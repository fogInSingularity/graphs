#include <iostream>

#include <spdlog/spdlog.h>
#include <spdlog/common.h>
#include <spdlog/sinks/basic_file_sink.h>

#include "dag/cfg.hpp"
#include "dag/dag.hpp"

int main(const int argc, const char* argv[]) {
    auto logger = spdlog::basic_logger_mt("graph", "graph.log", true);
    spdlog::set_default_logger(logger);

    spdlog::set_pattern("[%l] %v"); // remove time and name(%n) from log

#if defined (NDEBUG)
    spdlog::set_level(spdlog::level::info);
#else // NDEBUG
    spdlog::flush_on(spdlog::level::trace);
    spdlog::set_level(spdlog::level::trace);
#endif // NDEBUG

    // log argv
    for (int i = 0; i < argc; i++) {
        spdlog::info("argv[{}]: {}", i, argv[i]);
    }

    try {
        graph::Cfg cfg{};
        cfg.DotDump("graph.dot");

        auto&& idom_tree = cfg.BuildIDomTree();
        idom_tree.DotDump("idom_tree.dot");

        auto&& postdom_tree = cfg.BuildPostDomTree();
        postdom_tree.DotDump("postdom_tree.dot");
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error" << std::endl;
    }
}

