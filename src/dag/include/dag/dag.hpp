#ifndef DAG_HPP_
#define DAG_HPP_

// На вход подается текстовый файл с описанием графа в формате:
// [Номер узла][разделитель][номер узла преемника][разделитель][номер узла преемника] и т.д.

// Пример:
// 3 5 7 2
// 5 9
// 7 9

// Означает следующий граф:

//       Node3
//     /  |   \
// Node5 Node7 Node2
//  \    /
//   Node9

// Задание:

// 1. Построить граф на основе списков смежности
// 2. Достроить узлы Start и End (связать все подграфы в единый граф)
// 3. Убедиться, что данный граф ациклический
// 4. Провести топологическую сортировку данного графа.
// 5. Построить дерево доминаторов данного графа 
// 6. Построить дерево постдоминаторов данного графа

// Результатирующие графы должны выводиться в формате graphviz

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
#include <stdexcept>
#include <algorithm>
#include <deque>

#include <spdlog/spdlog.h>

namespace dag {
class Dag {
  private:
    using Id = int64_t;
    using AdjList = std::vector<Id>;
    struct Node {
        Id id_;
        AdjList adj_list_;

        explicit Node(Id id) : id_{id} {}
        Node() : id_{0} {
            SPDLOG_TRACE("Node(): default constructor called");
        }
    };

    Id start_id_;
    Id end_id_;
    using NodesContainer = std::unordered_map<Id, Node>;
    using NodesContainerIt = NodesContainer::iterator;
    NodesContainer nodes_;

  public:
    // NOTE: maybe use some other way to construct graph, maybe static method
    Dag() : start_id_{0}, end_id_{0} {
        Id max_id = 0;

        std::unordered_set<Id> leaf_nodes{};

        while (!std::cin.eof()) {
            Id new_id = 0;
            std::cin >> new_id;
            max_id = std::max(max_id, new_id);

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

            max_id = std::max(max_id, leaf);

            nodes_.emplace(leaf, leaf);
        }

        SPDLOG_TRACE("Dag() max_id {}", max_id);

        std::unordered_map<Id, bool> have_in_edge{};
        std::unordered_map<Id, bool> have_out_edge{};

        Node start_node{max_id + 1};
        Node end_node{max_id + 2};

        for (auto&& node_pair: nodes_) {
            auto&& cur_node_id = node_pair.second.id_;

            have_in_edge[cur_node_id] = false;
        }

        for (auto&& node_pair: nodes_) {
            auto&& cur_node_id = node_pair.second.id_;
            if (node_pair.second.adj_list_.empty()) {
                have_out_edge[cur_node_id] = false;
                SPDLOG_TRACE("Dag(): found node {} have no out edges", cur_node_id);
                continue;
            } else {
                have_out_edge[cur_node_id] = true;
            }

            auto&& adj_list = node_pair.second.adj_list_;
        
            for (auto&& adj: adj_list) {
                have_in_edge[adj] = true;
            }
        }

        for (auto&& edge_pair: have_in_edge) {
            if (!edge_pair.second) {
                SPDLOG_TRACE("Dag(): iter in edges {}", edge_pair.first);
                start_node.adj_list_.push_back(edge_pair.first);
            }
        }

        for (auto&& edge_pair: have_out_edge) {
            if (!edge_pair.second) {
                SPDLOG_TRACE("Dag(): iter out edges {}", edge_pair.first);
                auto&& it = nodes_.find(edge_pair.first);
                it->second.adj_list_.push_back(end_node.id_);
            }
        }

        nodes_.insert({start_node.id_, start_node});
        start_id_ = start_node.id_;
        
        nodes_.insert({end_node.id_, end_node});
        end_id_ = end_node.id_;
    }

    template <typename Func>
    void UnorderedTraverse(Func func) {
        // static_assert(std::is_invocable_v<Func>, "UnorderedTraverse expects function");

        for (auto&& v: nodes_) {
            func(v.second);
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

            if (node.id_ == start_id_) {
                dump_stream << "Start (Node" << node.id_ << ")";
            } else if(node.id_ == end_id_) {
                dump_stream << "End (Node" << node.id_ << ")";
            } else {
                dump_stream << "Node" << node.id_;
            }

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

        UnorderedTraverse(dump_node);

        // end
        dump_stream << "}\n";
    }

  private:
    enum class Color {
        kWhite,
        kGrey,
        kBlack,
    };

    template <typename Func>
    void DFSImpl(Func func, std::unordered_map<Id, Color>* colors, const Node& node) {
        // SPDLOG_TRACE("DFSImpl start node id {}", node.id_);

        if (!colors->contains(node.id_)) {
            colors->insert({node.id_, Color::kWhite});
        }

        auto color = colors->at(node.id_);
        if (color == Color::kBlack) { return ; }
        if (color == Color::kGrey) { throw std::runtime_error{"Cycle detected"}; }

        colors->at(node.id_) = Color::kGrey;

        auto&& adj_list = node.adj_list_;
        for (auto&& adj: adj_list) {
            auto&& next = nodes_[adj];
            DFSImpl(func, colors, next);
        }

        colors->at(node.id_) = Color::kBlack;

        func(node);

        // SPDLOG_TRACE("DFSImpl end node id {}", node.id_);
    }

  public:
    template <typename Func>
    void DFS(Func func) {
        std::unordered_map<Id, Color> colors{};

        DFSImpl(func, &colors, nodes_[start_id_]);
    }

    auto TopologicalSort() {
        std::deque<Id> sorted_ids;

        auto cb = [&sorted_ids](const Node& node) {
            SPDLOG_TRACE("Top sort: push front new id {}", node.id_);

            sorted_ids.push_front(node.id_);
        };

        DFS(cb);

        return sorted_ids;
    }

  private:
    using DomSet = std::unordered_set<Id>;
    using PredSet = std::unordered_set<Id>;
    PredSet Pred(Id id) {
        PredSet pred;

        for (auto&& node: nodes_) {
            auto&& cur_node = node.second;
            for (auto&& adj: cur_node.adj_list_) {
                if (adj == id) {
                    pred.insert(cur_node.id_);
                }
            }
        }

        return pred;
    }

    DomSet DomIntersection(const std::unordered_map<Id, DomSet>& dom_sets, Id id, const PredSet& pred) {
        DomSet dom_set;

        DomSet dom_to_erase;

        if (pred.empty()) { 
            SPDLOG_TRACE("DomIntersection: id {} empty pred", id);
            dom_set.insert(id);
            return dom_set; 
        }

        Id default_id = *pred.begin();
        // SPDLOG_TRACE("DomIntersection: default_id {}", default_id);
        dom_set = dom_sets.at(default_id);

        for (auto&& cur_pred_id: pred) {
            // SPDLOG_TRACE("DomIntersection: cur_pred_id {}", cur_pred_id);
            if (cur_pred_id == default_id) { continue; }

            auto&& cur_pred = dom_sets.at(cur_pred_id);

            for (auto&& e: dom_set) {
                // SPDLOG_TRACE("DomIntersection: dom set elem {}", e);
                if (!cur_pred.contains(e)) {
                    // dom_set.erase(e);
                    dom_to_erase.insert(e);
                }
            }
        }
        dom_set.insert(id);

        for (auto&& to_erase: dom_to_erase) {
            dom_set.erase(to_erase);
        }

        return dom_set;
    }

  public:
    auto ComputeDomSet() {
        std::unordered_map<Id, DomSet> dom_sets;
        
        auto&& traverse_order = TopologicalSort();

        for (auto&& cur_id: traverse_order) {
            // SPDLOG_TRACE("ComputeDomSet: cur_id {}", cur_id);
            auto&& pred = Pred(cur_id);
            auto&& dom_intr = DomIntersection(dom_sets, cur_id, pred);

            for (auto&& e: dom_intr) {
                SPDLOG_TRACE("ComputeDomSet: id {}, dom {}", cur_id, e);
            }

            dom_sets.insert({cur_id, dom_intr});
        }

        return dom_sets;
    }
  private:

  public:
    void BuildDomTree() {
        auto&& dom_sets = ComputeDomSet();
        
        std::unordered_map<Id, Id> idoms;

        for (auto&& dag_node: nodes_) {
            Id cur_id = dag_node.second.id_;
            // SPDLOG_TRACE("BuildDomTree: cur_id {}", cur_id);
            if (cur_id == start_id_) { continue; }

            auto&& cur_dom_set = dom_sets[cur_id];
            cur_dom_set.erase(cur_id); // idom(x) != x
            
            for (auto&& dom1: cur_dom_set) {
                // SPDLOG_TRACE("BuildDomTree: dom1 {}", dom1);
                bool is_idom = true;

                for (auto&& dom2: cur_dom_set) {
                    // SPDLOG_TRACE("BuildDomTree: dom2 {}", dom2);
                    if (dom1 == dom2) { continue; }
                    
                    auto&& dom2_set = dom_sets[dom2];
                    if (dom2_set.contains(dom1)) { // !(dom1 dom dom2)
                        is_idom = true;
                        break;
                    }
                }

                if (is_idom) {
                    SPDLOG_TRACE("BuildDomTree: cur_id {} idom id {}", cur_id, dom1);
                    idoms[cur_id] = dom1;
                    break;
                }
            }
        }
    }
};

} // namespace dag

#endif // DAG_HPP_
