/****************************************
 ** \file       optparse.hh
 ** \brief      Program option parser.
 */

#ifndef OPTPARSE_HH
#define OPTPARSE_HH

#include <cstdlib>
#include <cstring>

#include <string>
#include <map>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iostream>

#ifdef DEBUG
#include <cassert>
#include "util/io.hh"
#define OPTP_DEBUG_(...) util::println(__VA_ARGS__)
#define OPTP_DEBUG_ASSERT_(expr, msg) assert(expr && msg)
#else
#define OPTP_DEBUG_(...) do {} while (0)
#define OPTP_DEBUG_ASSERT_(e, m) do {} while (0)
#endif

namespace optparse {

using std::string;
using ReturnArguments = std::vector<char*>;

struct Option
{
  string description;
  std::function<void()> handle;
  std::function<void(const char*)> handle_param;

  friend std::ostream& operator<<(std::ostream& out, const Option& o)
  { return out << "Option{" << o.description << "}"; }
};

struct Exit : public std::exception {};

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
  using Options = std::map<string, Option>;
  using AltNames = std::map<string, Option*>;
  using ShortNames = std::map<char, Option*>;

private:
  string _progname;
  Options _options;
  AltNames _alt_names;
  ShortNames _short_names;
  
public:
  OptionParser(string prog, bool create_help = true):
    _progname{prog}
  {
    if (create_help)
      add_option(
        "help|h", [this] {
          this->print_usage(std::cout);
          throw Exit{};
        }, "Show this help message."
      );
  }
  
private:
  // Parse a single-character argument
  // return the associated option, or null
  const Option* _parse_short(char ch) const
  {
    auto it = _short_names.find(ch);
    return it != end(_short_names) ? it->second : nullptr;
  }

  const Option* _parse_long(string const& arg) const
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
  template <class It>
  ReturnArguments parse_args(const It a, const It b) const
  {
    ReturnArguments unparsed;
    const Option* last_opt = {};
    
    for (auto it_arg = a; it_arg != b; ++it_arg) {
      auto const arg = *it_arg;
      auto const argend = arg + std::strlen(arg);
      OPTP_DEBUG_(
        "arg ", std::distance(a, it_arg), '/', std::distance(a, b), ": \"", arg, '"'
      );

      // if we are done, copy rest to unparsed
      if (arg == options_end) {
        copy(it_arg, b, back_inserter(unparsed));
        break;
      }
      
      const Option* cur_opt = {};

      // first, check if this is an option
      if (arg[0] == option_char && arg[1]) {
        
        if (last_opt)
          throw std::runtime_error{"expected option argument"};
        
        if (arg[1] == option_char && arg[2]) { // Long
          OPTP_DEBUG_("long: ", arg + 2);
          
          auto const valsep = std::find(arg + 2, argend, value_delimiter);
          string const str{arg + 2, valsep};
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
          // assume bundled
          // stop when param is expected, or at end of arg
          for (auto pos = arg + 1; *pos; ++pos) {
            OPTP_DEBUG_("short: ", pos[0]);
          
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
        // last option was saved, it wants this arg
        if (last_opt) {
          OPTP_DEBUG_ASSERT_(last_opt->handle_param, "");
          last_opt->handle_param(arg);
        } else {
          unparsed.push_back(arg);
        }
      }

      last_opt = cur_opt;
    }
        
    if (last_opt)
      throw std::runtime_error{"expected option argument"};
    
    return unparsed;
  }
  
  ReturnArguments parse_argv(int argc, char** argv) const
  {
    return parse_args(argv + 1, argv + argc);
  }
  
  void parse_argv_inplace(int& argc, char**& argv) const
  {
    auto unparsed = parse_argv(argc, argv);
    
    copy(begin(unparsed), end(unparsed), argv + 1);
    argc = static_cast<int>(unparsed.size() + 1);
  }

private:
  // invariant:
  //    can't have duplicate option names
  //    can't have empty name
  //    names must be validated - TODO
  void _add_option(string name, Option opt)
  {
    OPTP_DEBUG_("adding option: \"", name, '"');
    std::vector<string> names;
    do {
      auto pos = name.find(option_name_divider);
      auto n = name.substr(0, pos);
      OPTP_DEBUG_("partial option name: ", n);
      
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

public:
  OptionParser& add_option(string name, std::function<void()> fn, string desc = {})
  {
    _add_option(name, Option{desc, fn, {}});
    return *this;
  }

  OptionParser& add_option(string name, std::function<void(const char*)> fn, string desc = {})
  {
    _add_option(name, Option{desc, {}, fn});
    return *this;
  }

  template <class T>
  OptionParser& add_option(string name, T* store, string desc = {})
  {
    auto store_fn = [store](const char* p) {
      std::istringstream in{p};
      in >> *store;
      if (in.fail())
        throw std::runtime_error{"failed to parse parameter: " + in.str()};
    };
    _add_option(name, Option{desc, {}, store_fn});
    return *this;
  }
  
  void print_usage(std::ostream& out) const
  {
    uint const column_wrap = 80;
    uint const column_desc = 30;
    
    out << "Usage: " << _progname << " [options]\n";

    // for (auto&& gr: _option_groups) {
    //   out << ' ' << gr.description << ":\n";
    //   for (auto&& e: gr.options) {
    for (auto&& e: _options) {
      char short_name{};
      {
        auto it = find_if(
          begin(_short_names), end(_short_names),
          [&](const ShortNames::value_type& p) {return p.second == &e.second;}
        );
        if (it != end(_short_names))
          short_name = it->first;
      }
      
      std::ostringstream line;
      if (short_name)
        // indent 2, add short opt
        line << "  " << '-' << short_name << ',';
      else
        // indent 5
        line << "     ";
      line << " --" << e.first;
      if (line.str().size() > column_desc)
        line << '\n';

      // line up description text
      // for (auto desc_text = e.second.description;
      //      !desc_text.empty();
      //      line = std::ostringstream{}) {
        
      //   auto const str = line.str();
        
      //     out << line << '\n';
      //   line.();
      // }
      
    }
    // }
  }
};

}

#endif
