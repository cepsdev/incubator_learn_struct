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

    template<typename T> node_t strct(T);



    template<typename T> node_struct_t rec(string name, T v){
        return rec(name,children(as_struct_ref(strct<T>(v))));
    }

    template<typename T, typename... Ts> node_struct_t rec(string name, T a1, Ts... rest){
        auto r = rec(name,rest...);
        if (a1) children(*r).insert(children(*r).begin(),a1);
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
}
 

namespace learn_structs{
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    class Spec{
            size_t cur_state{};
        public:
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
            }
            void init_position(){
                cur_state = {};
            }
            void move_to(state_t);
            optional<state_t> next(string);
            state_t insert(string);
            void print(ostream& os, size_t from);
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
                //cout << cur_input_elem << " --\n";
                auto cur_state = spec.next(name(cur_input_elem));
                if (!cur_state){
                    spec.move_to(spec.insert(name(cur_input_elem)));
                } else spec.move_to(*cur_state);
            }
        }
        return spec;
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
    
    Spec::state_t Spec::insert(string requested_name){
        auto it = cur_state;
        for( ;it != states.size();++it){
            if (requested_name == states[it].name) 
             break;
        }
        bool not_found = it == states.size();
        if (not_found){
            state_t new_state{it,requested_name,requested_name,states[cur_state].neighbors};
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
}


ceps::ast::node_t learn_structs::plugin_entrypoint(ceps::ast::node_callparameters_t params){
    using namespace std;
    using namespace ceps::ast;
    using namespace ceps::interpreter;

    auto result = mk_struct("result");
    auto data = get_first_child(params);    
    if (!is<Ast_node_kind::structdef>(data)) return result;

    auto& ceps_struct = *as_struct_ptr(data);

    Learner learner;
    auto spec = learner.learn(children(ceps_struct));
    spec.print(cout,0);

    return result;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  learn_structs::plugin_master = smc->get_plugin_interface();
  learn_structs::plugin_master->reg_ceps_phase0plugin("learn_struct", learn_structs::plugin_entrypoint);
}

