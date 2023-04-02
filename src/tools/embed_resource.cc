// Based on
// <https://github.com/The-Powder-Toy/The-Powder-Toy/blob/v97.0.352/resources/ToArray.cpp>

#include <cerrno>
#include <cstring>
#include <fstream> // IWYU pragma: keep
#include <iostream>

int main(int argc, char* argv[]) {
  if (argc < 2) return 1;
  char* mode = argv[1];

  if (strcmp(mode, "header") == 0) {

    if (argc != 4) return 1;
    char* output_header_path = argv[2];
    char* variable_name = argv[3];

    std::ofstream output(output_header_path);
    if (!output) {
      std::cerr << "open(" << output_header_path << ") failed: " << strerror(errno) << std::endl;
      return 1;
    }

    output << "#pragma once" << std::endl;
    output << "#include <stddef.h>" << std::endl;
    output << "#ifdef __cplusplus" << std::endl;
    output << "extern \"C\" {" << std::endl;
    output << "#endif" << std::endl;
    output << "extern const unsigned char " << variable_name << "[];" << std::endl;
    output << "extern const size_t " << variable_name << "_size;" << std::endl;
    output << "#ifdef __cplusplus" << std::endl;
    output << "}" << std::endl;
    output << "#endif" << std::endl;

  } else if (strcmp(mode, "source") == 0) {

    if (argc != 6) return 1;
    char* input_path = argv[2];
    char* output_source_path = argv[3];
    char* output_header_path = argv[4];
    char* variable_name = argv[5];

    std::ifstream input(input_path, std::ios::binary);
    if (!input) {
      std::cerr << "open(" << input_path << ") failed: " << strerror(errno) << std::endl;
      return 1;
    }
    std::ofstream output(output_source_path);
    if (!output) {
      std::cerr << "open(" << output_source_path << ") failed: " << strerror(errno) << std::endl;
      return 1;
    }

    output << "#include \"" << output_header_path << "\"" << std::endl;
    output << "const unsigned char " << variable_name << "[] = {";
    char c;
    unsigned long long length = 0;
    while (input.get(c)) {
      if (length > 0) {
        output << ",";
      }
      output << (unsigned int)(unsigned char)c;
      length += 1;
    }
    output << "};" << std::endl;
    output << "const size_t " << variable_name << "_size = " << length << ";" << std::endl;

  } else {
    return 1;
  }

  return 0;
}
