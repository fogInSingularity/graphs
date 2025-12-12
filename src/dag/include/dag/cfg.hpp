#ifndef CFG_HPP_
#define CFG_HPP_

#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <algorithm>
#include <deque>

#include <spdlog/spdlog.h>

#include "dag/dag.hpp"

namespace graph {

class Cfg : public Dag {
  private:
    using Dag::Id;
    Id start_id_;
    Id end_id_;

  public:
    // NOTE: maybe use some other way to construct graph, maybe static method
    Cfg() : start_id_{0}, end_id_{0} {
        Id max_id = 0;

        for (auto&& node: nodes_) {
            max_id = std::max(max_id, node.second.id_);
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

  private:
    enum class Color {
        kWhite,
        kGrey,
        kBlack,
    };

    template <typename Func>
    void DFSImpl(Func func, std::unordered_map<Id, Color>* colors, const Node& node) {
        static_assert(
            std::is_invocable_v<Func&, const Node&>, 
            "UnorderedTraverse expects function"
        );
        assert(colors != nullptr);

        if (!colors->contains(node.id_)) {
            colors->insert({node.id_, Color::kWhite});
        }

        auto color = colors->at(node.id_);
        if (color == Color::kBlack) { return ; }
        if (color == Color::kGrey) { 
            throw std::runtime_error{"Cycle detected"}; 
        }

        colors->at(node.id_) = Color::kGrey;

        auto&& adj_list = node.adj_list_;
        for (auto&& adj: adj_list) {
            auto&& next = nodes_[adj];
            DFSImpl(func, colors, next);
        }

        colors->at(node.id_) = Color::kBlack;

        func(node);
    }

  public:
    template <typename Func>
    void DFS(Func func) {
        static_assert(
            std::is_invocable_v<Func&, const Node&>, 
            "UnorderedTraverse expects function"
        );

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
        dom_set = dom_sets.at(default_id);

        for (auto&& cur_pred_id: pred) {
            if (cur_pred_id == default_id) { continue; }

            auto&& cur_pred = dom_sets.at(cur_pred_id);

            for (auto&& e: dom_set) {
                if (!cur_pred.contains(e)) {
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

  private:
    auto ComputeDomSet() {
        std::unordered_map<Id, DomSet> dom_sets;
        
        auto&& traverse_order = TopologicalSort();

        for (auto&& cur_id: traverse_order) {
            SPDLOG_TRACE("ComputeDomSet: cur_id {}", cur_id);
            auto&& pred = Pred(cur_id);
            auto&& dom_intr = DomIntersection(dom_sets, cur_id, pred);

            for (auto&& e: dom_intr) {
                SPDLOG_TRACE("ComputeDomSet: id {}, dom {}", cur_id, e);
            }

            dom_sets.insert({cur_id, dom_intr});
        }

        return dom_sets;
    }

    auto ComputeIDom() {
        auto&& dom_sets = ComputeDomSet();
        
        std::unordered_map<Id, Id> idoms;

        for (auto&& dag_node: nodes_) {
            Id cur_id = dag_node.second.id_;
            if (cur_id == start_id_) { continue; }

            auto&& cur_dom_set = dom_sets[cur_id];
            cur_dom_set.erase(cur_id); // idom(x) != x
            
            for (auto&& dom1: cur_dom_set) {
                bool is_idom = true;

                for (auto&& dom2: cur_dom_set) {
                    if (dom1 == dom2) { continue; }
                    
                    auto&& dom2_set = dom_sets[dom2];
                    if (dom2_set.contains(dom1)) { // (dom1 dom dom2)
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

        return idoms;
    }

    auto ReverseCfg() {
        Cfg new_cfg(*this);

        for (auto& node: new_cfg.nodes_) {
            node.second.adj_list_.clear();
        }

        for (auto&& node: nodes_) {
            auto&& cur_id = node.second.id_;
            auto&& adj_list = node.second.adj_list_;
            for (auto&& adj: adj_list) {
                new_cfg.nodes_[adj].adj_list_.push_back(cur_id);
            }
        }

        new_cfg.start_id_ = this->end_id_;
        new_cfg.end_id_ = this->start_id_;

        return new_cfg;
    }
  public:
    auto BuildIDomTree() {
        auto&& idoms = ComputeIDom();

        NodesContainer dom_tree_nodes;
        for (auto&& node: nodes_) {
            auto&& new_id = node.second.id_;
            Node new_node{new_id};

            dom_tree_nodes.insert({new_id, new_node});
        }

        for (auto&& idom: idoms) {
            auto&& node_it = dom_tree_nodes.find(idom.second);
            assert(node_it != dom_tree_nodes.end()); // we already inserted all nodes
            
            node_it->second.adj_list_.push_back(idom.first);
        }

        return Dag{dom_tree_nodes};
    }

    auto BuildPostDomTree() {
        auto&& rev_cfg = ReverseCfg();
        // LogNodes(rev_cfg.nodes_);
        return rev_cfg.BuildIDomTree();
    }
};

} // namespace graph

#endif // CFG_HPP_
