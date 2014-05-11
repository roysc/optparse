#include "optparse.hh"

#include <string>
#include <iostream>
#include <cmath>
#include <vector>

#include "util/io.hh"
using namespace util::io;

int main(int argc, char* argv[])
{
  std::string f;
  int ru{}, rd{};
  double d{};
  optparse::OptionParser p{__FILE__};
  
  auto roundup = [&](const char* a) {ru = std::ceil(std::stod(a));};
  auto rounddown = [&](const char* a) {rd = std::floor(std::stod(a));};
  
  // p.add_option("f", &f);
  p.add_option("dub", &d);
  p.add_option("up", roundup);
  p.add_option("down|d", rounddown);
  p.add_option("b|p|q", []{ println("!!"); });
  p.add_option("file|in|f", &f);

  p.parse_args_inplace(argc, argv);

  println(std::vector<char*>{argv, argv + argc});
  util::println("ru = ", ru, " ; rd = ", rd, " ; d = ", d);
  util::println("f = ", f);
}
