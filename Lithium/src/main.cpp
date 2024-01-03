#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>
#include <cstring>
#include "stdio.h"

#include "tokenization.hpp"
#include "parser.hpp"
#include "generation.hpp"
//#include "generationWin.hpp"
//#include "generationLith.hpp"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage:" << std::endl;
        fprintf(stderr, "  %s <file.l> <compilation args>\n", argv[0]);
        return EXIT_FAILURE;
    }

    bool verbose = false;
    bool debug = false;
    std::string outputFile = "out";
    std::string platform = "linux";
    std::string inputFile = "";
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "-output") == 0 || std::strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                outputFile = argv[i + 1];
                i++;
            }
            else {
                std::cerr << "Error: -o option requires an argument.\n";
                return 1;
            }            
        }
        else if (std::strcmp(argv[i], "-verbose") == 0 || std::strcmp(argv[i], "-v") == 0) {
            verbose = true;
        }
        else if (std::strcmp(argv[i], "-debug") == 0 || std::strcmp(argv[i], "-d") == 0) {
            debug = true;
        }
        else if (std::strcmp(argv[i], "-platform") == 0 || std::strcmp(argv[i], "-p") == 0) {
            if (i + 1 < argc) {
                platform = argv[i + 1];
                i++;
            }
            else {
                std::cerr << "Error: -p option requires an argument.\n";
                return 1;
            }    
        }
        else {
            inputFile = argv[i];
        }
    }

    if (inputFile == "") {
        std::cout << "No input file" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::stringstream contents_stream;
    std::fstream input(inputFile, std::ios::in);
    contents_stream << input.rdbuf();
    input.close();
    std::string contents = contents_stream.str();

    std::string fileName = inputFile.substr(inputFile.find_last_of("/\\") + 1);

    Tokenizer tokenizer(std::move(contents), fileName);
    std::vector<Token> tokens = tokenizer.tokenize();

    Parser parser(std::move(tokens), fileName);
    std::optional<NodeProg> prog = parser.parse_prog();

    if (!prog.has_value()) {
        std::cerr << "Invalid program" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (platform == "win") {
        std::cout << "Broken by updates and currently no longer supported." << std::endl;
        // GeneratorWin generator(prog.value());
        // std::fstream file("out.asm", std::ios::out);
        // file << generator.gen_prog();
        // file.close();
        // system("nasm -fwin64 out.asm");
        // system("gl.exe /console /entry:_start out.obj kernel32.dll");
    }
    else if (platform == "linux") {
        Generator generator(prog.value(), verbose, fileName);
        std::fstream file("out.asm", std::ios::out);
        file << generator.gen_prog();
        file.close();
        system("nasm -felf64 out.asm");
        std::string ldCmd = "ld -o " + outputFile + " out.o";
        system(ldCmd.c_str());
    }
    else if (platform == "lith") {
        std::cout << "Not yet supported." << std::endl; 
        // GeneratorLith generator(prog.value());
        // std::fstream file("out.asm", std::ios::out);
        // file << generator.gen_prog();
        // file.close();
    }

    if (!debug) {
        system("rm out.asm");
        system("rm out.o");
    }

    return EXIT_SUCCESS;
}