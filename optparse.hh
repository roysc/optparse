/****************************************
 ** \file       util/optparse.hh
 ** \brief      Program option parser.
 */

#ifndef UTIL_OPTPARSE_HH
#define UTIL_OPTPARSE_HH

#include <cstring>
#include <cassert>

#include <stdexcept>
#include <map>
#include <iterator>
#include <algorithm>
#include <functional>
#include <sstream>

#ifdef DEBUG
#include "util/io.hh"
#define OPTP_DEBUG_(...) util::println(__VA_ARGS__)
#define OPTP_DEBUG_ASSERT_(expr, msg) assert(expr && msg)
#else
#define OPTP_DEBUG_(...) do {} while (0)
#define OPTP_DEBUG_ASSERT_(e, m) do {} while (0)
#endif

namespace optparse {

using std::string;
using std::map;
using std::function;

struct Option
{
  string description;
  function<void()> handle;
  function<void(const char*)> handle_param;
};

const char option_char = '-';
const string options_end = "--";
const char value_delimiter = '=';

const char option_name_divider = '|';

// Example:
//  OptionParser parser;
//  parser.add_option("long|l")
//  parser.add_option("s")
struct OptionParser
{
private:
  string _progname;
  map<string, Option> _options;
  map<string, Option*> _alt_names;
  map<char, Option*> _short_names;
  
public:
  OptionParser(string prog): _progname{prog} {}
  
private:
  // Parse a single-character argument
  // return the associated option, or null
  Option* _parse_short(char ch)
  {
    auto it = _short_names.find(ch);
    return it != end(_short_names) ? it->second : nullptr;
  }

  Option* _parse_long(const string& arg)
  {
    auto it = _options.find(arg);
    if (it != end(_options))
      return &it->second;
    else {
      auto it_alt = _alt_names.find(arg);
      return it_alt != end(_alt_names) ? it_alt->second : nullptr;
    }
  }

public:
  std::vector<char*> parse_args(int argc, char** argv)
  {
    std::vector<char*> unparsed;
    Option* last_opt = {};
    
    for (int idx_arg = 1; idx_arg < argc; ++idx_arg) {
      Option* cur_opt = {};
      
      auto const arg = argv[idx_arg];
      auto const argend = arg + std::strlen(arg);
      
      OPTP_DEBUG_("arg ", idx_arg, '/', argc, ": \"", arg, '"');

      // if we are done, copy rest to unparsed
      if (arg == options_end) {
        copy(argv + idx_arg, argv + argc, back_inserter(unparsed));
        break;
      }

      // 1: check if this is an option
      if (arg[0] == option_char && arg[1]) {
        
        if (last_opt)
          throw std::runtime_error{"expected option argument"};
        
        if (arg[1] == option_char && arg[2]) { // Long
          OPTP_DEBUG_("long: ", arg + 2);
          
          auto valsep = std::find(arg + 2, argend, value_delimiter);
          string str{arg + 2, valsep};
          auto opt = _parse_long(str);

          // TODO: change unknown-handling behavior
          if (!opt)
            throw std::runtime_error{"unknown argument: " + str};
          
          if (opt->handle_param) {
            // OPTP_DEBUG_("expecting param");
            // if we expect a param, check if it's adjacent
            if (valsep != argend)
              opt->handle_param(valsep + 1);
            // otherwise, param is next arg
            else {
              OPTP_DEBUG_("no '", value_delimiter, "'");
              cur_opt = opt;
            }
          } else {
            OPTP_DEBUG_ASSERT_(opt->handle, "");
            opt->handle();
          }
          
        } else {                // Short
          OPTP_DEBUG_("short: ", arg[1]);
          
          // assume bundled
          // stop when param is expected, or at end of arg
          for (auto pos = arg + 1; *pos; ++pos) {
            
            auto opt = _parse_short(*pos);
            if (!opt) throw std::runtime_error{string{"unknown argument: "} + *pos};
            
            if (opt->handle_param) {
              // if we expect a param, check if it's adjacent
              if (pos[1]) 
                opt->handle_param(pos + 1);
              // otherwise, param is next arg
              else
                cur_opt = opt;
              break;
            } else {
              OPTP_DEBUG_ASSERT_(opt->handle, "");
              opt->handle();
            }
          }
        }
        
      } else {
        // could just save handler?
        if (last_opt) {
          OPTP_DEBUG_ASSERT_(last_opt->handle_param, "");
          last_opt->handle_param(arg);
        } else {
          unparsed.push_back(arg);
        }
      }
      // OPTP_DEBUG_(last_opt);
      last_opt = cur_opt;
    }
        
    if (last_opt)
      throw std::runtime_error{"expected option argument"};
    
    return unparsed;
  }
  
  void parse_args_inplace(int& argc, char**& argv)
  {
    auto unparsed = parse_args(argc, argv);
    
    copy(begin(unparsed), end(unparsed), argv);
    argc = static_cast<int>(unparsed.size());
  }

  // invariant:
  //    can't have duplicate option names
  //    can't have empty name
  void _add_option(string name, Option opt)
  {
    std::vector<string> names;
    do {
      auto pos = name.find(option_name_divider);
      auto n = name.substr(0, pos);

      if (n.empty())
        throw std::invalid_argument{"option name cannot be empty"};
      if (count(begin(names), end(names), name))
        throw std::invalid_argument{"duplicate option name: " + n};
      
      names.push_back(n);
      if (pos == string::npos)
        name.clear();
      else
        name.erase(0, pos + 1);
    } while (!name.empty());
    
    auto it = _options.emplace(names[0], opt);
    // handle short-only option name by not erasing it
    if (names[0].size() != 1)
      names.erase(begin(names));
      
    for (auto&& n: names) {
      if (n.size() == 1) {
        auto it_alt = _short_names.emplace(n[0], &it.first->second);
        if (!it_alt.second)
          throw std::invalid_argument{"duplicate option name: " + n};
      } else {
        auto it_alt = _alt_names.emplace(n, &it.first->second);
        if (!it_alt.second)
          throw std::invalid_argument{"duplicate option name: " + n};
      }
    }
    
    OPTP_DEBUG_(_options);
  }
  
  void add_option(string name, function<void()> fn, string desc = {})
  {
    _add_option(name, Option{desc, fn, {}});
  }

  void add_option(string name, function<void(const char*)> fn, string desc = {})
  {
    _add_option(name, Option{desc, {}, fn});
  }

  template <class T>
  void add_option(string name, T* store, string desc = {})
  {
    auto store_fn = [store](const char* p) {
      std::istringstream in{p};
      in >> *store;
    };
    add_option(name, store_fn, desc);
  }
  
  // void print_usage(std::ostream& out)
  // {
  //   out << "Usage: " << _progname << "[options]\n";
  // }
};

}

#endif
