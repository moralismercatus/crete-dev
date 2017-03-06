#include "tc_gen.h"
#include <crete/exception.h>

#include <boost/property_tree/xml_parser.hpp>

#include <sstream>
#include <fstream>
#include <regex>

namespace pt = boost::property_tree;

namespace crete
{
namespace util
{

auto generate_test_case(const pt::ptree& complete_tree) -> TestCase
{
    auto rtc = TestCase{};
    auto& crete_node = complete_tree.get_child("crete");
    auto tc_node = crete_node.get_child("test-case");

    auto elem_range = tc_node.equal_range("element");

    for(auto elem_it = elem_range.first;
        elem_it != elem_range.second;
        ++elem_it)
    {
        auto tce = TestCaseElement{};

        auto const& elem = elem_it->second;
        auto& name_node = elem.get_child("name");
        auto& data_node = elem.get_child("data");
        auto name = name_node.get_value<std::string>();

        tce.name = decltype(tce.name){name.begin(), name.end()};
        tce.name_size = tce.name.size();

        if(data_node.size() == 0)
        {
            auto sdata = data_node.get_value<std::string>();

            CRETE_EXCEPTION_ASSERT(sdata.size() % 2 == 0,
                                   err::parse{"element data must contain even number of hex digits"});

            auto match = std::smatch{};
            while(std::regex_search(sdata,
                                    match,
                                    std::regex{"([0-9a-fA-F][0-9a-fA-F])"}))
            {
                auto sbyte = match[1];
                auto ibyte = 0u;
                std::stringstream ss{sbyte};

                ss >> std::hex >> ibyte;

                tce.data.emplace_back(static_cast<uint8_t>(ibyte));

                sdata = match.suffix().str();
            }
        }
        else
        {
            CRETE_EXCEPTION_ASSERT(data_node.size() == 1,
                                   err::parse{"'data' node expects a single child"});

            auto fill_opt = data_node.get_child_optional("fill");
            auto file_opt = data_node.get_child_optional("file");

            if(fill_opt)
            {
                auto const& fill = *fill_opt;
                auto sbyte = fill.get<std::string>("<xmlattr>.value");
                auto isize = std::stoi(fill.get<std::string>("<xmlattr>.size"), 0, 16);
                auto ibyte = 0u;
                auto match = std::smatch{};

                if(std::regex_search(sbyte,
                                     match,
                                     std::regex{"([0-9a-fA-F][0-9a-fA-F])"}))
                {
                    auto sbyte = match[1];
                    auto ibyte = 0u;
                    std::stringstream ss{sbyte};

                    ss >> std::hex >> ibyte;

                    for(auto i = 0; i < isize; ++i)
                    {
                        tce.data.emplace_back(ibyte);
                    }
                }
                else
                {
                    CRETE_EXCEPTION_THROW(err::parse{"element.fill.value expects hex byte"});
                }
            }
            // TODO: handle <file> case.
        }

        tce.data_size = tce.data.size();
        rtc.add_element(tce);
    }

    return rtc;
}

} // namespace util
} // namespace crete

int main(int argc, char* argv[])
{
    try
    {
    pt::ptree config;
    {
        std::ifstream ifs("tc.xml");

        pt::read_xml(ifs,
                     config);
    }

    auto tc = crete::util::generate_test_case(config);

    std::cerr << tc << std::endl;
    }
    catch(std::exception& e)
    {
        std::cerr << boost::diagnostic_information(e) << std::endl;
    }

    return 0;
}
