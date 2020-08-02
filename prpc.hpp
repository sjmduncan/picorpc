// Copyright (C) 2020 Stuart Duncan
//
// This file is part of picoRPC.
//
// picoRPC is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// picoRPC is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with picoRPC.  If not, see <http://www.gnu.org/licenses/>.

#pragma once
#include <any>
#include <map>
#include <tuple>
#include <string>
#include <iomanip>
#include <sstream>
#include <exception>
#include <functional>

using std::map;
using std::string;
using std::function;

namespace prpc{
  template <typename T> constexpr bool _is_tuple = false;
  template <typename ... T> constexpr bool _is_tuple<std::tuple<T...>>   = true;
  
  typedef std::function<void(string)> transport_send_f;
  typedef std::function<string(string)> transport_sendrec_f;

  // Default space-delimited strings
  class serial_message{
    protected:
      std::stringstream serial_stream;
    public:
      string fun_id;
      template <typename ... T, std::size_t ... I>
      void extract_args_tuple( std::tuple<T ... > &tuple, std::index_sequence<I ... >){
        (extract_arg_value(std::get<I>(tuple)) , ... );
      }
      template <typename T>
      std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::string>, void>
      insert_value(T const &value){
        serial_stream << ' ' << value;
      }
      template <typename ... T, std::size_t ... I>
      void insert_tuple(std::tuple<T ... > const &tuple, std::index_sequence<I ... >){
        (insert_value(std::get<I>(tuple)) , ... );
      }
      void insert_value(char const *value){insert_value(std::string{value});}
      template <typename T>
      std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string>, void>
      insert_value(T const &value){
        serial_stream << ' ' << std::quoted(value);
      }
      template <typename T> std::enable_if_t<std::is_same_v<T, std::string>, void>
      extract_arg_value(T &value){
        serial_stream >> std::quoted(value);
      }
      template <typename T> std::enable_if_t<!std::is_same_v<T, std::string>, void>
      extract_arg_value(T &value){
        serial_stream >> value;
      }
      template <typename T> std::enable_if_t<_is_tuple<T>, void>
      extract(T &value){
        extract_args_tuple(value, std::make_index_sequence<std::tuple_size_v<T>>{});
      }
      template <typename T>
      std::enable_if_t<_is_tuple<T>, void>
      insert(T const &value){
        serial_stream << fun_id;
        insert_tuple(value, std::make_index_sequence<std::tuple_size_v<T>>{});
      }
      template <typename T>
      std::enable_if_t<std::is_same_v<std::decay_t<T>, std::string>, void>
      append(T const &value){
        serial_stream << ' ' << std::quoted(value);
      }
      template <typename T>
      std::enable_if_t<!std::is_same_v<std::decay_t<T>, std::string>, void>
      append(T const &value){
        serial_stream << ' ' << value;
      }
      string serial(){return serial_stream.str(); }
      bool has_failed(){return serial_stream.fail() || (serial_stream.rdbuf()->in_avail() != 0); }
  };

  class from_serial : public serial_message{

    public:
     from_serial(string msg_str){
       serial_stream = std::stringstream(msg_str);
       serial_stream >> fun_id;
     }

  };
  class to_serial : public serial_message{
    public:
      to_serial(string fun_id){
        serial_stream = std::stringstream();
        serial_stream << fun_id;
      }
  };
  class invoker{
    
    template <typename> struct function_signature;
    template <typename R, typename ... T> struct function_signature<std::function<R(T ... )>>{
      using ret_t = std::decay_t<R>;
      using args_tupl_t = std::tuple<std::decay_t<T> ... >;
    };
    template <typename FUN_T, typename ARGS_T>
    static
    std::enable_if_t<!std::is_same_v<std::decay_t<decltype(std::apply(std::declval<FUN_T>(), std::declval<ARGS_T>()))>, void>, void>
    apply_optional_return(FUN_T func, ARGS_T args, to_serial &resp)
    {
      auto data = std::apply(std::move(func), std::move(args));
      resp.append(data);
    }

    template <typename FUN_T, typename ARGS_T>
    static
    std::enable_if_t<std::is_same_v<std::decay_t<decltype(std::apply(std::declval<FUN_T>(), std::declval<ARGS_T>()))>, void>, void>
    apply_optional_return(FUN_T func, ARGS_T args, to_serial &resp)
    {
      std::apply(std::move(func), std::move(args));
    }
    map<string, function<void(from_serial&,to_serial&)>> wrapped_functions;
    transport_sendrec_f rec_fun;
    transport_send_f send_fun;
    map<string, string> func_argstr;

    map<string, string>::iterator funiter=func_argstr.begin();
    string next_func(){
      if(funiter == func_argstr.end()){
        funiter=func_argstr.begin();
        funiter++;
        return string{"PRPC_FUNLIST_END"};
      }
      string rv=  "\"" + funiter->first + " " + funiter->second + "\"";
      funiter++;
      return rv;
    }  
    public:
      invoker(transport_send_f _send_fun){
        send_fun = std::move(_send_fun);
        std::function<string(void)> fp =std::bind(&invoker::next_func,this);
        add("prpc-get-next-function", fp);
      }
      template<typename FUN_T>
      void add(string fun_id, FUN_T function){
        if(wrapped_functions.count(fun_id) != 0) throw std::exception();

        auto fun_wrap = [_function = std::move(function)] (from_serial &inv_params, to_serial &resp){
          std::function func{std::move(_function)};

          using function_signature = function_signature<decltype(func)>;
          using args_tupl_t = typename function_signature::args_tupl_t;

          args_tupl_t data;
          inv_params.extract(data);
          if(inv_params.has_failed()) resp.append("PRPC_INV_ARG_EXTRACT_FAILED");
          else{
            apply_optional_return(std::move(func), std::move(data), resp);
          }
        };

        wrapped_functions[fun_id] = std::move(fun_wrap);
        func_argstr[fun_id] = fun_id;
        funiter=func_argstr.begin();
        funiter++;
      }

      void invoke(string inv_param_str){
        from_serial inv_params(inv_param_str);
        to_serial ret_param(inv_params.fun_id);

        if(wrapped_functions.count(inv_params.fun_id) == 0){
          ret_param.append("PRPC_INV_FUN_NOEXIST");
        }else{
          try{
            wrapped_functions[inv_params.fun_id](inv_params, ret_param);
          }catch(std::exception e){
            ret_param.append("PRPC_INV_EXCEPT");
            ret_param.append(e.what());
          }
        }
        send_fun(ret_param.serial());
      }
  };
  class caller{
    transport_sendrec_f sendrec_fun;
    class call_return final{
      mutable std::optional<std::any> value;
      from_serial *rp;
      public:
        call_return(from_serial *serial_param){
          rp = serial_param;
        }
        template <typename T>
        T as() const {
          using Type = std::decay_t<T>;

          if (!value){
            Type data{};
            rp->extract_arg_value(data);

            value = std::move(data);
          }
          return std::any_cast<Type>(*value);
        }

        template <typename T>
        operator T () const
        {
            return as<T>();
        }
    };
    from_serial *rp = nullptr;
    public:
    caller(transport_sendrec_f _rec_fun){
      sendrec_fun = std::move(_rec_fun);
    }
    template <typename ... TArgs>
    call_return call(string fun_id, TArgs && ... args)
    {
      auto data = std::make_tuple(std::forward<TArgs>(args) ... );

      to_serial params(fun_id);
      params.insert(data);

      string response = sendrec_fun(params.serial());
      if(rp) delete rp;
      rp = new from_serial(response);
      return call_return(rp);
    }
  };
}