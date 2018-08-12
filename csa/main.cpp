#include <iostream>

#include "clara.hpp"
#include "data_structure.hpp"
#include "experiments.hpp"


int main(int argc, char* argv[]) {
    std::string name;
    bool use_hl = false;
    bool show_help = false;
    auto cli_parser = clara::Arg(name, "name")("The name of the dataset to be used in the algorithm") |
                      clara::Opt(use_hl)["--hl"]("Unrestricted walking with hub labelling") |
                      clara::Help(show_help);

    auto result = cli_parser.parse(clara::Args(argc, argv));
    if (!result) {
        std::cerr << "Error in command line: " << result.errorMessage() << std::endl;
        cli_parser.writeToStream(std::cout);
        exit(1);
    }
    if (show_help) {
        cli_parser.writeToStream(std::cout);
        return 0;
    }

    Experiment exp {name, use_hl};
    exp.run();

    return 0;
}
