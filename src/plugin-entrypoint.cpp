#include <stdlib.h>
#include <iostream>
#include <ctype.h>
#include <chrono>
#include <sstream>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdlib.h>
#include <map>
#include <algorithm>
#include <future>
#include <netinet/sctp.h> 

#include "ceps_ast.hh"
#include "core/include/state_machine_simulation_core.hpp"

namespace utils{
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    node_struct_t rec(string name, node_struct_t e1);
    node_struct_t rec(string name, vector<node_t> nodes);
    node_struct_t rec(string name, int v);
    node_struct_t rec(string name, int8_t v);
    node_struct_t rec(string name, uint8_t v);
    node_struct_t rec(string name, int16_t v);
    node_struct_t rec(string name, uint16_t v);
    node_struct_t rec(string name, int64_t v);
    node_struct_t rec(string name, uint64_t v);
    node_struct_t rec(string name, string v);
    node_struct_t rec(string name);
    node_struct_t rec(string name, node_t n);

    template<typename T> node_t strct(T);



    template<typename T> node_struct_t rec(T v){
        return v;
    }

    template<typename T, typename... Ts> node_struct_t rec(string name, T a1, Ts... rest){
        auto r = rec(name,rest...);
        if (a1 && r) children(*r).insert(children(*r).begin(),a1);
        return r;
    }
}

namespace utils{

    node_struct_t rec(string name, int v){
        auto r = mk_struct(name); auto r2 = mk_int_node(v); children(*r).push_back(r2);
        return r;    
    }
    node_struct_t rec(string name, int8_t v)
    {
        return rec(name,(int)v);
    }
    node_struct_t rec(string name, uint8_t v)
    {
        return rec(name,(int)v);
    }
    node_struct_t rec(string name, int16_t v)
    {
        return rec(name,(int)v);
    }
    node_struct_t rec(string name, uint16_t v)
    {
        return rec(name,(int)v);
    }

    node_struct_t rec(string name, node_t n){
        auto r = mk_struct(name); if (n != nullptr) children(*r).push_back(n);
        return r;    

    }
    node_struct_t rec(string name, int64_t v){
        auto r = mk_struct(name); auto r2 = mk_int64_node(v); children(*r).push_back(r2);
        return r;    
    }
    node_struct_t rec(string name, uint64_t v){
        auto r = mk_struct(name); auto r2 = mk_int64_node(v); children(*r).push_back(r2);
        return r;    
    }

   node_struct_t rec(string name, string v){
        auto r = mk_struct(name); auto r2 = mk_string(v); children(*r).push_back(r2);
        return r;    
   }
   node_struct_t rec(string name, node_struct_t e1){
        auto r = mk_struct(name);
        if (e1) children(*r).push_back(e1);
        return r;
    }

    node_struct_t rec(string name, vector<node_t> nodes)
    {
        auto r = mk_struct(name);
        children(*r) = nodes;
        return r;
    }

   node_struct_t rec(string name){
        auto r = mk_struct(name);
        return r;
    }
   
   node_t id(string name){
        auto r = new Identifier(name);
        return r;
    }
    
}
 

namespace learn_structs{
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    class Spec{
            size_t cur_state{};
            optional<size_t> boundary_state{};
            bool is_boundary(size_t state){
                if (!boundary_state) return false;
                return *boundary_state == state;
            }
            void set_boundary(size_t state){
                boundary_state = state;
            }
            string name;
            map<string,int> name2occurence_count;
            string new_id(string);
        public:
            using state_it = size_t;
            struct state_t{
                size_t pos;
                string id;               
                string name;
                vector<size_t> neighbors;                
            };
            vector<state_t> states;
            Spec(){
                states.push_back({{0},{"Initial"}, {}, {1}}); // Initial
                states.push_back({{1},{"Final"}, {}, {} }); // Final
                boundary_state = 0;
            }
            void init_position(){
                cur_state = {};
            }
            void move_to(state_t);
            void move_to(state_it);
            optional<state_t> next(string);
            state_t insert(string);
            void connect(state_it from, state_it to);
            void print(ostream& os, size_t from);
            node_t as_state_machine();
            void set_name(string new_name) {name = new_name;}
            optional<size_t> find_by_name(string, size_t start_index = {});
            auto get_current_state_idx() const {return cur_state;}
            bool connected(state_it from, state_it to);
            state_it copy(state_it);
            vector<state_it> predecessors(state_it);
    };

    class Learner{
        public:
            using structs_t = vector<node_t>;
            Spec learn(structs_t structs );
    };

    static Ism4ceps_plugin_interface* plugin_master = nullptr;
    static const std::string version_info = "learn-structs v0.1";
    static constexpr bool print_debug_info{true};
    ceps::ast::node_t plugin_entrypoint(ceps::ast::node_callparameters_t params);
}

namespace learn_structs{
    
    Spec Learner::learn(structs_t structs ){
        Spec spec;
        for(auto e: structs){
            if (!is<Ast_node_kind::structdef>(e)) continue;
            auto& the_struct = *as_struct_ptr(e);
            spec.init_position();
            //cout << the_struct << " --\n";
            for(size_t i = 0; i != children(the_struct).size(); ++i){
                auto cur_elem_raw = children(the_struct)[i];
                if (!is<Ast_node_kind::structdef>(cur_elem_raw)) continue;
                auto& cur_input_elem = *as_struct_ptr(cur_elem_raw);
                auto name_of_current_input = name(cur_input_elem);
                //cout << cur_input_elem << " --\n";
                auto next_state = spec.next(name_of_current_input);
                if (!next_state){
                    //INVARIANT: spec.cur_state has no transition under cur_input_elem
                    
                    // Case a: there is no previously learned instance of cur_input_elem
                    // Case b: cur_input_elem is known but no transition from cur_state leads to an instance
                    // Case b-1: an instance lies 'ahead' of cur_state, i.e. index(instance) > cur_state
                    // Case b-2: an instance lies 'in front of' cur_state, i.e. index(instance) < cur_state
                    // Case b-3: cur_state is an instance, i.e. index(instance) = cur_state
                    
                    auto it = spec.find_by_name(name_of_current_input);
                    if (!it /*Case a*/)
                        spec.move_to(spec.insert(name_of_current_input));
                    else /*Case b*/ {
                        if (*it < spec.get_current_state_idx()){ /*check for cases b-1/b-3*/
                         auto tit = it;
                         for(; (tit = spec.find_by_name(name_of_current_input, *tit + 1)).has_value() ; it = *tit){
                            if (*it == spec.get_current_state_idx()) break;
                         }
                        }
                        //INVARIANT: Cases are clear ('it' points to the correct index in each possible b case)
                        if (*it == spec.get_current_state_idx()){ //b-3

                        } else { // b-1, b-2
                            if (spec.connected(*it,spec.get_current_state_idx()) ){
                                //cycle
                                //TODO: Refactor (too involved)
                                auto from = spec.get_current_state_idx();
                                auto to = *it;
                                auto new_from = spec.copy(from);
                               
                                auto new_from_pred = spec.predecessors(from);
                                
                                for(auto e : new_from_pred)
                                    if (e != *it) 
                                        for(auto& succ: spec.states[e].neighbors) 
                                            if (succ == from){ succ = new_from;}
                                auto new_to = spec.copy(*it);
                                size_t pos_from_in_to_neighbors = 0;
                                for(; spec.states[new_to].neighbors.size() > pos_from_in_to_neighbors; ++pos_from_in_to_neighbors)
                                    if (spec.states[new_to].neighbors[pos_from_in_to_neighbors] == from) break;
                                spec.states[new_to].neighbors.erase(spec.states[new_to].neighbors.begin() + pos_from_in_to_neighbors );
                                spec.connect(new_from, new_to);
                                spec.move_to(new_to);
                            } else {
                                spec.connect(spec.get_current_state_idx(), *it);
                                spec.move_to(*it);
                            }
                        }
                    }
                } else spec.move_to(*next_state);
            }
        }
        return spec;
    }

    vector<Spec::state_it> Spec::predecessors(state_it s){
        vector<Spec::state_it> r;
        for(auto e: states)
         for (auto ee: e.neighbors) if (ee == s) r.push_back(e.pos);
        return r;
    }

    string Spec::new_id(string name) {
        auto i = name2occurence_count[name]++;
        return name+"_"+to_string(i);
    }

    Spec::state_it Spec::copy(state_it it){
        states.push_back(states[it]);
        states[states.size() - 1].id = new_id(states[it].name);
        states[states.size() - 1].pos = states.size() - 1; 
        return states.size() - 1;
    }

    optional<size_t> Spec::find_by_name(string requested_name, size_t start_index){
        for(auto i = start_index; i < states.size(); ++i )
         if (states[i].name == requested_name) return i;
        return {};
    }

    optional<Spec::state_t> Spec::next(string requested_name){
        auto& e = states[cur_state];
        for (auto it: e.neighbors)
            if (states[it].name == requested_name) return states[it];
        return {};
    }

    void Spec::move_to(state_t s){
        cur_state = s.pos;        
    }

    void Spec::move_to(Spec::state_it sit){
        cur_state = sit;        
    }

    bool Spec::connected(state_it from, state_it to){
        for (auto e: states[from].neighbors) if (e == to) return true;
        return false;
    }

    void Spec::connect(state_it from, state_it to){
            states[from].neighbors.push_back(to);                
    }
    
    Spec::state_t Spec::insert(string requested_name){
        auto it = cur_state;
        for( ;it != states.size();++it){
            if (requested_name == states[it].name) 
             break;
        }
        bool not_found = it == states.size();
        if (not_found){
            state_t new_state{it,requested_name,requested_name,states[cur_state].neighbors};
            if (is_boundary(cur_state))
            {
                size_t i = 0;
                for(;i != states[cur_state].neighbors.size(); ++i)
                 if (states[cur_state].neighbors[i] == 1) break;
                if (i != states[cur_state].neighbors.size())
                    states[cur_state].neighbors.erase(states[cur_state].neighbors.begin() + i);
                set_boundary(new_state.pos);
            }
            states[cur_state].neighbors.push_back(it);
            states.push_back(new_state);
            return new_state;
        }
    }

    void Spec::print(ostream& os, size_t from){
        for (auto it : states[from].neighbors){
            os << states[from].id;
            os << " -> " << states[it].id << " ";
        }
        for (auto it : states[from].neighbors) print(os,it);
    }
    node_t Spec::as_state_machine(){
        using namespace utils;
        auto get_all_states = [&](){
            vector<node_t> r{/*mk_string("Initial"), mk_string("Final")*/};
            vector<string> v;
            for (auto e: states ){
                if (e.id.length() ) v.push_back(e.id);
                else if (e.name.length()) v.push_back(e.name);
            }            
            for (auto e: v)
                r.push_back(id(e));
            return r;
        };
        auto get_all_transitions = [&](){
            vector<node_t> t;
            for(auto s : states){
                if (!s.id.length() && !s.name.length()) continue;
                for (auto i : s.neighbors){
                    t.push_back(
                        rec("t", 
                            id(s.id), 
                            id(states[i].id), 
                            ceps::ast::mk_symbol(  i == 1 ? "eof" : states[i].name , string{"Event"}) ));
                }
            }            
            return t;
        };
        return rec("sm", 
                    id(name),
                    rec("states",get_all_states()), get_all_transitions() );
    }
}


ceps::ast::node_t learn_structs::plugin_entrypoint(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    auto result = mk_struct("result");
    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) return result;

    auto& ceps_struct = *as_struct_ptr(data);
    Nodeset ns{children(ceps_struct)};
    auto test_set = ns["test_set"];
    auto spec_name = ns["spec_name"].as_str();

    Learner learner;
    auto spec = learner.learn(test_set.nodes());
    //spec.print(cout,0);
    if (spec_name.length()) spec.set_name(spec_name) ;
    else if (children(ceps_struct).size() && is<Ast_node_kind::structdef>(children(ceps_struct)[0]) ) 
        spec.set_name("Spec_"+name(as_struct_ref(children(ceps_struct)[0])));
    
    auto sm = spec.as_state_machine();
    //cout << *sm << "\n";
    return utils::rec("result", utils::rec("state_machine", sm));
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  learn_structs::plugin_master = smc->get_plugin_interface();
  learn_structs::plugin_master->reg_ceps_phase0plugin("learn_struct", learn_structs::plugin_entrypoint);
}

