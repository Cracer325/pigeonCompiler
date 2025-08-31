#include <iostream>
#include <fstream>
#include <optional>
#include <sstream>
#include <vector>
#include "./arena.hpp"
#include "./tokenization.hpp"
#include "./parser.hpp"
#include "./generation.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Incorrect usage, correct usage:" << std::endl;
        std::cerr << "pig <input.pig>" << std::endl;
        return EXIT_FAILURE;
    }

    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1], std::ios::in);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }
    Tokenizer tokenizer(contents);
    {
        std::fstream file("out.asm", std::ios::out);
        std::vector<Token> tokens = tokenizer.tokenize();
        Parser parser(std::move(tokens));
        std::optional<NodeProg> root = parser.parse_prog();
        if (!root.has_value()) {
            std::cerr << "Invalid program" << std::endl;
            exit(EXIT_FAILURE);
        }
        Generator generator(root.value());
        file << generator.gen_prog();
    }

    system("nasm -felf64 out.asm");
    system("ld -o out out.o");
    return EXIT_SUCCESS;
}