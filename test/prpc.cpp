// Copyright (C) 2020 Stuart Duncan
//
// This file is part of PicoRPC.
//
// PicoRPC is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PicoRPC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PicoRPC.  If not, see <http://www.gnu.org/licenses/>.

#include "catch.hpp"
#include "prpc.hpp"

std::string tmp_response;
void inv_dummy_send(string msg){
  tmp_response=msg;
}

TEST_CASE("Construction doesn't throw exception", "[invoker]"){
  prpc::invoker* srv;
  REQUIRE_NOTHROW(srv = new prpc::invoker(inv_dummy_send));
}


TEST_CASE("Can fetch invoker version string", "[invoker]"){
  tmp_response = "";
  prpc::invoker srv(inv_dummy_send);
  srv.invoke("prpc-get-version");
  REQUIRE(tmp_response == string("PRPC_GOOD") + " \"" + PRPC_VERSION_STR + "\"");
}

TEST_CASE("Invoke non-existant function doesn't throw, response is error string", "[invoker]"){
  tmp_response = "";
  prpc::invoker srv(inv_dummy_send);
  REQUIRE_NOTHROW(srv.invoke("bogus-fun"));
  REQUIRE(tmp_response == "PRPC_INV_FUN_NOEXIST");
}

bool funcalled = false;
void testvoidvoid(){ funcalled = true; }
TEST_CASE("Add and invoke void(void) function", "[invoker]"){
  tmp_response = "";
  prpc::invoker srv(inv_dummy_send);
  funcalled = false;
  REQUIRE_NOTHROW(srv.add("test-voidvoid", testvoidvoid));
  
  SECTION("Re-adding existing function throws exception"){
    REQUIRE_THROWS(srv.add("test-voidvoid", testvoidvoid));
  }

  SECTION("Invoking void(void) function doesn't throw, function is called"){
    REQUIRE(!funcalled);
    REQUIRE_NOTHROW(srv.invoke("test-voidvoid"));
    REQUIRE(tmp_response == "PRPC_GOOD");
    REQUIRE(funcalled);
  }

  SECTION("Invoking void(void) with trailing space returns error string, function not called"){
    REQUIRE(!funcalled);
    REQUIRE_NOTHROW(srv.invoke("test-voidvoid "));
    REQUIRE(tmp_response == "PRPC_INV_ARG_EXTRACT_FAILED");
    REQUIRE(!funcalled);
  }

  SECTION("Invoking void(void) with a parameter doesn't throw, response is error string"){
    REQUIRE_NOTHROW(srv.invoke("test-voidvoid 99"));
    REQUIRE(tmp_response == "PRPC_INV_ARG_EXTRACT_FAILED");
    REQUIRE(!funcalled);
  }

  SECTION("Invoking non-existant function doesn't throw, response is error string"){
    REQUIRE_NOTHROW(srv.invoke("bogus-fun"));
    REQUIRE(tmp_response == "PRPC_INV_FUN_NOEXIST");
    REQUIRE(!funcalled);
  }

}

int voidintval = -1;
void testvoidint(int arg){ voidintval = arg;}
TEST_CASE("Add and invoke void(int) function", "[invoker]"){
  tmp_response="";
  voidintval = -1;
  prpc::invoker srv(inv_dummy_send);
  REQUIRE_NOTHROW(srv.add("test-voidint", testvoidint));

  SECTION("Invoking void(int) correctly doens't throw and the value is passed"){
    REQUIRE(voidintval == -1);
    REQUIRE_NOTHROW(srv.invoke("test-voidint 99"));
    REQUIRE(voidintval == 99);
    REQUIRE(tmp_response == "PRPC_GOOD");
  }
    
  SECTION("Invoking void(int) with bad arg type returns error string"){
    REQUIRE(voidintval == -1);
    REQUIRE_NOTHROW(srv.invoke("test-voidint ninetynine"));
    REQUIRE(tmp_response == "PRPC_INV_ARG_EXTRACT_FAILED");
    REQUIRE(voidintval == -1);

    REQUIRE_NOTHROW(srv.invoke("test-voidint 99 ninetynine"));
    REQUIRE(tmp_response == "PRPC_INV_ARG_EXTRACT_FAILED");
    REQUIRE(voidintval == -1);

    REQUIRE_NOTHROW(srv.invoke("test-voidint ninetynine 99"));
    REQUIRE(tmp_response == "PRPC_INV_ARG_EXTRACT_FAILED");
    REQUIRE(voidintval == -1);
  }
}

int testintint(){ return 42;}
TEST_CASE("Add and call int(void) function", "[invoker]"){
  tmp_response="";
  prpc::invoker srv(inv_dummy_send);
  REQUIRE_NOTHROW(srv.add("test-voidint", testintint));

  SECTION("Invoking int(void) correctly doesn't throw and the value is passed"){
    REQUIRE_NOTHROW(srv.invoke("test-voidint"));
    REQUIRE(tmp_response == "PRPC_GOOD 42" );
  }
}

string teststr;
void testvoidstr(string s){teststr = s;}
TEST_CASE("Add and call void(string-like) function", "[invoker]"){
  tmp_response="";
  teststr="";
  prpc::invoker srv(inv_dummy_send);

  SECTION("Invoking void(string) correctly doesn't throw and the value is passed"){
    REQUIRE_NOTHROW(srv.add("test-voidstr", testvoidstr));
    REQUIRE_NOTHROW(srv.invoke("test-voidstr test-string-with-no-spaces"));
    REQUIRE(tmp_response == "PRPC_GOOD" );
    REQUIRE(teststr == "test-string-with-no-spaces");

    REQUIRE_NOTHROW(srv.invoke("test-voidstr \"quote-delimited string with spaces & symbols. Will it work?\""));
    REQUIRE(tmp_response == "PRPC_GOOD" );
    REQUIRE(teststr == "quote-delimited string with spaces & symbols. Will it work?");

    REQUIRE_NOTHROW(srv.invoke("test-voidstr 192.168.0.2"));
    REQUIRE(tmp_response == "PRPC_GOOD");
    REQUIRE(teststr == "192.168.0.2");
  }

  SECTION("Adding/invoking const char* or char* or pointer-types is not allowed"){
  }
}

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

void simple(){}
int get_int(){return 42;}
int add_one(int n){ return n+1; }
string get_string(){ return string{"string with spaces in it"};}
string concat(string msg1, int msg2) { return msg1 + std::to_string(msg2); }

TEST_CASE("Test caller with dummy transport", "[caller-invoker]"){
  // server section
  invoke = new prpc::invoker(dummy_transport_invoke_send);
  invoke->add("simple", simple);
  invoke->add("get_int", get_int);
  invoke->add("add_one", add_one);
  invoke->add("get_string", get_string);
  invoke->add("concat", concat);
  
  // client section
  caller = new prpc::caller(dummy_transport_call_sendrec);
  SECTION("Calling simple function doesn't throw"){
    caller->call("simple");
  }

  SECTION("Passing and returning int works"){
    int return_getint = caller->call("get_int");
    REQUIRE(return_getint == 42);
    int return_addone = caller->call("add_one", 10);
    REQUIRE(return_addone == 11);
  }

  SECTION("Mixed arg types and string returns"){
    string gotstring = caller->call("get_string");
    REQUIRE(gotstring == "string with spaces in it");
    string retstr_1 = caller->call("concat", string{"spaces count in string: "}, 4);
    REQUIRE(retstr_1 == "spaces count in string: 4");
  }
  SECTION("Implicit parameter conversion to string is okay"){
    string restr_2 = caller->call("concat", 888, 4);
    string restr_3 = caller->call("concat", (double)888, 4);
    REQUIRE(restr_2 == "8884");
    REQUIRE(restr_3 == "8884");
  }

  SECTION("char* string literals are treated as strings"){
    string retstr_1 = caller->call("concat", "spaces count in string: ", 4);
    REQUIRE(retstr_1 == "spaces count in string: 4");
  }
  
  SECTION("Bad function calls throw"){
    REQUIRE_THROWS(caller->call("bogus-fn"));
    REQUIRE_THROWS(caller->call("add_one", (double)0.1));
    REQUIRE_THROWS(caller->call("add_one", 1, 99));
    REQUIRE_THROWS(caller->call("concat", "I am an string.", "I'm also a string, but I'm in the wrong place"));
  }
  
  SECTION("Returning to existing string variables is not okay, simple types are fine"){
    string return_string = caller->call("get_string");
    //string bad_return_string;
    //bad_return_string = caller->call("get_string");// -> compiler error
    
    int getint_return = caller->call("get_int");
    getint_return = caller->call("get_int");
  }

}