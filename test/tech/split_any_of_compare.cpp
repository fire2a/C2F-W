#include <algorithm>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <boost/algorithm/string.hpp>

static std::vector<std::string> split_any_of(std::string_view s,
                                             std::string_view delims,
                                             bool compress)
{
    std::vector<std::string> out;
    const size_t n = s.size();
    size_t i = 0;
    while (i <= n) {
        size_t j = s.find_first_of(delims, i);
        if (j == std::string_view::npos) j = n;
        out.emplace_back(s.substr(i, j - i));
        if (j == n) break;
        if (compress) {
            i = j + 1;
            while (i < n && delims.find(s[i]) != std::string_view::npos) ++i;
        } else {
            i = j + 1;
        }
    }
    return out;
}

static bool equal_vectors(const std::vector<std::string>& a,
                          const std::vector<std::string>& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

int main()
{
    struct Case { std::string line; std::string delims; bool compress; };
    std::vector<Case> cases = {
        {"a,b,c", ",", false},
        {"a,,b,,,c", ",", false},
        {"a,,b,,,c", ",", true},
        {"a;b,c|d", ",;|", false},
        {"a;;b,,|c", ",;|", true},
        {"", ",;|", false},
        {",|;", ",;|", false},
        {",|;", ",;|", true},
        {"  a  b   c  ", " ", false},
        {"  a  b   c  ", " ", true},
    };

    int failures = 0;

    for (const auto& cs : cases) {
        // Boost split
        std::vector<std::string> boost_out;
        if (cs.compress) {
            boost::algorithm::split(boost_out, cs.line,
                                    boost::is_any_of(cs.delims),
                                    boost::token_compress_on);
        } else {
            boost::algorithm::split(boost_out, cs.line,
                                    boost::is_any_of(cs.delims),
                                    boost::token_compress_off);
        }

        // std replacement
        auto std_out = split_any_of(cs.line, cs.delims, cs.compress);

        const bool ok = equal_vectors(boost_out, std_out);
        if (!ok) {
            ++failures;
            std::cout << "Mismatch for line='" << cs.line << "' delims='" << cs.delims
                      << "' compress=" << (cs.compress ? "on" : "off") << "\n";
            std::cout << "  boost: [";
            for (size_t i = 0; i < boost_out.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << '"' << boost_out[i] << '"';
            }
            std::cout << "]\n  std  : [";
            for (size_t i = 0; i < std_out.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << '"' << std_out[i] << '"';
            }
            std::cout << "]\n";
        }
    }

    if (failures == 0) {
        std::cout << "All cases matched." << std::endl;
        return 0;
    } else {
        std::cout << failures << " mismatches." << std::endl;
        return 1;
    }
}
