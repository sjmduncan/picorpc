# PicoRPC

Really small header only RPC in C++17; No dependencies, BYO transport layer.

Modeled on [nanorpc](https://github.com/tdv/nanorpc), but the implementation there didn't quite scratch the itch so here we are. Also includes a copy of [Catch2](https://github.com/catchorg/Catch2) to run tests.

Discovery of all possible invokable functions (and their return/arg types) is also supported at runtime by repeatedly calling `prpc-get-next-function` which returns minimal information about the function: `<function-id> <return-type> <arg1-type> <arg2-type> ...`. When all functions are discovered the return value is `"PRPC_FUNLIST_END"`, after which you're done (or you can start iterating from the beginning again).

## Usage

The following example should build and run.

You can also invoke lambdas, but you currently can't pass/return STL containers or user defined types, and of course you can't pass/return pointers with the exception of string literals as function arguments.

```CPP
#include "prpc.hpp"
#include <iostream>
#include <cassert>
#include <exception>

using std::clog;
using std::endl;
using std::string;

prpc::invoker* invoker;
prpc::caller* caller;

string dummy_transport_buffer;
void dummy_transport_invoker_send(string message){
  dummy_transport_buffer=message;
}
string dummy_transport_call_sendrec(string msg){
  invoker->invoker(msg);
  return dummy_transport_buffer;
}

// Some example functions which will be invoked by the 'invoke' call in dummy_transport_call_sendrc
// and return values via the dummy_transport_buffer string.
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
  invoker = new prpc::invoker(dummy_transport_invoker_send);
  invoker->add("get_int", get_int);
  invoker->add("add_one", add_one);
  invoker->add("get_string", get_string);
  invoker->add("concat", concat);
  
  // client section ----------------------------------------------------------
  caller = new prpc::caller(dummy_transport_call_sendrec);

  clog << "prpc version=" << caller.call("prpc-get-version") << endl;

  // Print all function names + args added on the server
  while(true){
    try{
    string fun_descriptor = caller.call("prpc-get-next-function");
    if(fun_descriptor == "PRPC_FUNLIST_END") break;
    else clog << fun_descriptor << endl;
    }catch(std::exception e){
      clog <<"Error in prpc-get-next-function. Ending loop" << endl;
    }
  }

  try{
    caller.call("bogus-function-id");
  }catch(std::exception e){
    clog <<"exception trying to call 'bogus-function-id'. Maybe it doesn't exist?" << endl;
  }

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
