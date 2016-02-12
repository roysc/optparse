#include <cmath>
#include <string>
#include <vector>

#include "optparse.hh"

#include "util/io.hh"
using namespace util::io;

int main(int argc, char* argv[])
{
  std::string f;
  int ru{}, rd{};
  double d{};
  auto roundup = [&](const char* a) {ru = std::ceil(std::stod(a));};
  auto rounddown = [&](const char* a) {rd = std::floor(std::stod(a));};
  
  auto const p = optparse::OptionParser(__FILE__)
    .add_option("dub", &d, "A double")
    .add_option("up", roundup, "Round up")
    .add_option("down|d", rounddown)
    .add_option("b|p|q", []{ println("!!"); }, "Super cool")
    .add_option("file|in|f", &f);

  optparse::ReturnArguments rest;
  try {
    // rest = p.parse_args(argv + 1, argv + argc);
    rest = p.parse_argv(argc, argv);
    // p.parse_argv_inplace(argc, argv);
  } catch (optparse::Exit&) {
    return 0;
  }
  
  println(std::vector<char*>{argv, argv + argc});
  println(rest);
  util::println("ru = ", ru, " ; rd = ", rd, " ; d = ", d);
  util::println("f = ", f);
}
