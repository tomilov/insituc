#include <insituc/ast/tokens.hpp>
#include <insituc/ast/io.hpp>
#include <insituc/ast/compare.hpp>
#include <insituc/parser/parser.hpp>
#include <insituc/meta/io.hpp>
#include <insituc/meta/compiler.hpp>

#include <string>
#include <iostream>
#include <iterator>
#include <stdexcept>

#include <cstdlib>

int
main(int argc, char * argv[])
{
    std::istream & in_ = std::cin;
    std::ostream & out_ = std::cout;
    std::ostream & err_ = std::cerr;
    std::string source_ = ((1 < argc) ? argv[1] : "");
    if (source_.empty()) {
        std::string line_;
        in_.unsetf(std::ios_base::skipws);
        while (std::getline(in_, line_) && !line_.empty()) {
            source_.append(line_);
            source_ += '\n';
        }
    }
    using namespace insituc;
    auto const parse_result_ = parser::parse(std::cbegin(source_), std::cend(source_));
    if (!!parse_result_.error_) {
        auto const & error_description_ = *parse_result_.error_;
        err_ << "Error: \"" << error_description_.which_
             << "\" at input position #" << std::distance(error_description_.first_, error_description_.where_) << ", context:\n";
        std::copy(error_description_.where_, error_description_.last_, std::ostreambuf_iterator< char_type >(err_));
        err_ << std::endl;
        return EXIT_FAILURE;
    }
    out_ << "AST:" << std::endl
         << parse_result_.ast_ << std::endl;
    meta::assembler assembler_;
    meta::compiler const compiler_(assembler_);
    if (!compiler_(parse_result_.ast_)) {
        err_ << "Failure! Translation error." << std::endl;
        return EXIT_FAILURE;
    }
    out_ << "Assembly listing:" << std::endl
         << assembler_ << std::endl;
    out_ << "Success!" << std::endl;
    return EXIT_SUCCESS;
}


