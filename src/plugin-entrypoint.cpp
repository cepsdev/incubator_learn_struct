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
        public:
            struct state_t{

            };
            void init_position();
            void consume(string);
            optional<state_t> next(string);
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

                } else spec.consume(name(cur_input_elem));
            }
        }
        return spec;
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
    learner.learn(children(ceps_struct));

    return result;
}

extern "C" void init_plugin(IUserdefined_function_registry* smc)
{
  learn_structs::plugin_master = smc->get_plugin_interface();
  learn_structs::plugin_master->reg_ceps_phase0plugin("learn_struct", learn_structs::plugin_entrypoint);
}

