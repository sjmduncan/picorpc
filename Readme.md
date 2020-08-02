# Pico RPC

Header only RPC. Pure C++17 (with a runtime-binding C# client). No dependencies. BYO transport. Modeled on [nanorpc](https://github.com/tdv/nanorpc), but the implementation there didn't quite scratch the itch so here we are.

Discovery of all possible invokable functions (and their return/arg types) is also supported at runtime by repeatedly calling `prpc-get-next-function` until it returns `"PRPC_FUNLIST_END"`. Every string returned before that is of the format `<function-id> <return-type> <arg1-type> <arg2-type> ...`. This is used to implement the included C# client.

## Usage

```CPP
#include "prpc.hpp"
#include <iostream>
#include <cassert>

using std::clog;
using std::endl;
using std::string;

prpc::invoker* invoke;
prpc::caller* caller;

string dummy_transport_buffer;
void dummy_transport_invoke_send(string message){
  dummy_transport_buffer=message;
}
string dummy_transport_call_sendrec(string msg){
  invoke->invoke(msg);
  return dummy_transport_buffer;
}

// Some example functions which will be invoked via the dummy transport buffer
int get_int(){
  clog <<"server: get_int invoked"<<endl;
  return 42;
}
int add_one(int n){
  clog << "server: add_one invoked (n=" << n << ")" << endl;
  return n+1;
}
string get_string(){ 
  clog << "server: get_string invoked" << endl;
  return string{"string with spaces in it"};
}
string concat(string arg_str, int arg_int) {
  clog << "server: concat invoked (arg_str=\"" << arg_str <<"\", arg_int=" << arg_int << ")" << endl;
  return arg_str + std::to_string(arg_int);
}

int main(){
  // Server section ----------------------------------------------------------
  invoke = new prpc::invoker(dummy_transport_invoke_send);
  invoke->add("get_int", get_int);
  invoke->add("add_one", add_one);
  invoke->add("get_string", get_string);
  invoke->add("concat", concat);
  
  // client section ----------------------------------------------------------
  caller = new prpc::caller(dummy_transport_call_sendrec);

  int got_int = caller->call("get_int");
  assert(got_int == 42);
  clog << "client: get_int returned: " << got_int << endl;
  
  int added_int = caller->call("add_one", 10);
  assert(added_int == 11);
  clog << "client: add_one returned: " << added_int << endl;
  
  string got_str = caller->call("get_string");
  assert((got_str == "string with spaces in it"));
  clog << "client: get_string returned: \"" << got_str << '\"' << endl;
  
  string cat_str = caller->call("concat", string{"spaces count in string: "}, 4);
  assert((cat_str == "spaces count in string: 4"));
  clog << "client: concat returned: \"" << cat_str << std::endl;

  string retstr_1 = caller->call("concat", "this string has this many spaces: ", 6);
  assert((retstr_1 == "this string has this many spaces: 6"));
  clog << "client: concat returned: \"" << cat_str << "\" (without string{} around literal)" << std::endl;
}
```