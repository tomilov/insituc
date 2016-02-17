#include <insituc/ast/io.hpp>
#include <insituc/parser/parser.hpp>

#include <string>
#include <iostream>
#include <iterator>

#ifdef __linux__
#define GREEN(str) __extension__ "\e[1;32m" str "\e[0m"
#else
#define GREEN(str) str
#endif

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
                  << "\" at input position: ";
        std::copy(error_description_.where_, error_description_.last_, std::ostreambuf_iterator< char_type >(err_));
        err_ << std::endl;
        return EXIT_FAILURE;
    }
    out_ << parse_result_.ast_ << std::flush;
    auto const & ranges_ = parse_result_.ranges_;
    std::size_t const ranges_size_ = ranges_.size();
    out_ << "ranges size: " << ranges_size_ << "\nranges: " << std::endl;
    for (auto const & range_ : ranges_) {
        out_ << std::setw(3) << std::distance(range_.first, range_.second) << " " << GREEN(">");
        std::copy(range_.first, range_.second, std::ostreambuf_iterator< char_type >(out_));
        out_ << GREEN("<") << "\n";
        for (std::size_t j = 0; j < 79; ++j) {
            out_ << '-';
        }
        out_ << std::endl;
    }
    return EXIT_SUCCESS;
}
