// Based on
// <https://github.com/The-Powder-Toy/The-Powder-Toy/blob/v97.0.352/resources/ToArray.cpp>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
  if (argc != 5) return 1;
  char* input_path = argv[1];
  char* output_source_path = argv[2];
  char* output_header_path = argv[3];
  char* variable_name = argv[4];

  std::ifstream input(input_path, std::ios::binary);
  if (!input) {
    std::cerr << "open(" << input_path << ") failed: " << strerror(errno) << std::endl;
    return 1;
  }
  std::ofstream out_source(output_source_path);
  if (!out_source) {
    std::cerr << "open(" << output_source_path << ") failed: " << strerror(errno) << std::endl;
    return 1;
  }
  std::ofstream out_header(output_header_path);
  if (!out_header) {
    std::cerr << "open(" << output_header_path << ") failed: " << strerror(errno) << std::endl;
    return 1;
  }

  out_header << "#pragma once" << std::endl;
  out_header << "#include <stddef.h>" << std::endl;
  out_header << "#ifdef __cplusplus" << std::endl;
  out_header << "extern \"C\" {" << std::endl;
  out_header << "#endif" << std::endl;
  out_header << "extern const unsigned char " << variable_name << "[];" << std::endl;
  out_header << "extern const size_t " << variable_name << "_size;" << std::endl;
  out_header << "#ifdef __cplusplus" << std::endl;
  out_header << "}" << std::endl;
  out_header << "#endif" << std::endl;

  out_source << "#include \"" << output_header_path << "\"" << std::endl;
  out_source << "const unsigned char " << variable_name << "[] = {";
  char c;
  unsigned long long length = 0;
  while (input.get(c)) {
    if (length > 0) {
      out_source << ",";
    }
    out_source << (unsigned int)(unsigned char)c;
    length += 1;
  }
  out_source << "};" << std::endl;
  out_source << "const size_t " << variable_name << "_size = " << length << ";" << std::endl;

  return 0;
}
