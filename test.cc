#include "optparse.hh"

#include <string>
#include <iostream>
#include <cmath>
#include <vector>

#include "util/io.hh"
using namespace util::io;

int main(int argc, char* argv[])
{
  optparse::OptionParser p{__FILE__};

  std::string f;

  int ru{}, rd{};
  
  auto roundup = [&](const char* a) {
    println("roundup: '", a, "'");
    auto d = std::stod(a);
    ru = std::ceil(d);
  };

  auto rounddown = [&](const char* a) {
    println("rounddown: '", a, "'");
    auto d = std::stod(a);
    rd = std::floor(d);
  };

  double d{};
  
  // p.add_option("f", &f);
  p.add_option("up", roundup);
  p.add_option("down|d", rounddown);
  p.add_option("dub", &d);
  p.add_option("b|p|q", []{ println("!!"); });
  p.add_option("file|in|f", &f);

  p.parse_args_inplace(argc, argv);

  println(std::vector<char*>{argv, argv + argc});
  util::println("ru = ", ru, " ; rd = ", rd, " ; d = ", d);
  util::println("f = ", f);
}
