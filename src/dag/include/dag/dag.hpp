#ifndef DAG_HPP_
#define DAG_HPP_

#include <fstream>
#include <vector>
#include <cstdint>
#include <iostream>
#include <string>
#include <istream>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <source_location>
#include <stdexcept>
#include <functional>

#include <spdlog/spdlog.h>

namespace graph {

class Dag {
  protected:
    using Id = int64_t;
    using AdjList = std::vector<Id>;
    struct Node {
        Id id_;
        AdjList adj_list_;

        explicit Node(Id id) : id_{id} {}
        Node() : id_{0} {
            SPDLOG_WARN("Node(): default constructor called");
        }
    };

    using NodesContainer = std::unordered_map<Id, Node>;
    using NodesContainerIt = NodesContainer::iterator;
    NodesContainer nodes_;

    static void LogNodes(
        NodesContainer nodes, 
        std::source_location loc = std::source_location::current()
    ) {
        for (auto&& node: nodes) {
            SPDLOG_TRACE("node id {} at {}", node.second.id_, loc.function_name());
            for (auto&& adj: node.second.adj_list_) {
                SPDLOG_TRACE("\tnode adj {}", adj);
            }
        }
    }

    template <typename Container, typename T>
    static auto& SafeMapAt(
        Container& umap, 
        T val, 
        std::source_location loc = std::source_location::current()
    ) try {
        return umap.at(val);
    } catch (const std::exception& e) {
        std::cerr 
            << "at exception catch value: "  << val 
            << " at " << loc.function_name() << ":" << loc.line() << " "
            << e.what() << std::endl;
        throw e;
    } catch (...) {
        std::cerr 
            << "at exception catch value: "  << val 
            << " at " << loc.function_name() << ":" << loc.line() 
            << std::endl;
        throw ;
    }

  public:
    // NOTE: maybe use some other way to construct graph, maybe static method
    Dag() {
        std::unordered_set<Id> leaf_nodes{};

        while (!std::cin.eof()) {
            Id new_id = 0;
            std::cin >> new_id;

            leaf_nodes.erase(new_id);

            if (std::cin.eof()) { break; }

            SPDLOG_TRACE("Dag(): new node wth id {}", new_id);

            Node new_node{new_id};

            std::string adj_list_str;
            std::getline(std::cin, adj_list_str);

            std::istringstream parse_stream{adj_list_str};
            Id id = 0;
            while (parse_stream >> id) {
                SPDLOG_TRACE("Dag(): node id {} has adj id {}", new_id, id);
                if (!nodes_.contains(id)) {
                    leaf_nodes.insert(id);
                }
                new_node.adj_list_.push_back(id);
            }

            nodes_.insert({new_node.id_, new_node});
        }

        for (auto&& leaf: leaf_nodes) {
            SPDLOG_TRACE("Dag(): leaf nodes: {}", leaf);

            nodes_.emplace(leaf, leaf);
        }
    }
    explicit Dag(const NodesContainer& nodes) : nodes_{nodes} {}

    template <typename Func>
    void UnorderedTraverse(Func func) {
        static_assert(
            std::is_invocable_v<Func&, const Node&>, 
            "UnorderedTraverse expects function"
        );

        for (auto&& v: nodes_) {
            std::invoke(func, v.second);
        }
    }

    void DotDump(const std::filesystem::path& dump_path) {
        std::ofstream dump_stream{dump_path};
    
        // begin
        dump_stream << "digraph {\n";

        auto dump_node = [&dump_stream, this](const Node& node){
            dump_stream 
                << "\t"
                << "Node" 
                << node.id_ 
                << " [shape = box, style = filled, fillcolor = \"#08d9d6\", label = \"";

            dump_stream << "Node" << node.id_;

            dump_stream << "\"]\n";

            for (auto&& adj_id: node.adj_list_) {
                dump_stream
                    << "\t"
                    << "Node" 
                    << node.id_ 
                    << "->" 
                    << "Node" 
                    << adj_id 
                    << "\n";
            };
        };

        // UnorderedTraverse(dump_path);
        UnorderedTraverse(dump_node);

        // end
        dump_stream << "}\n";
    }


};

} // namespace graph

#endif // DAG_HPP_
