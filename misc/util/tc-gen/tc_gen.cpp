/******************************************************************************
 * Author(s): Christopher J. Havlicek
 *
 * For contact information, see CONTACTS.TXT
 * For license information, see LICENSE.TXT
 ******************************************************************************/

#include "tc_gen.h"
#include <crete/exception.h>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>

#include <sstream>
#include <fstream>
#include <regex>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
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

TestCaseUtility::TestCaseUtility(int argc, char *argv[])
    : ops_descr_(make_options())
{
    parse_options(argc, argv);
    process_options();
}

auto TestCaseUtility::parse_options(int argc, char* argv[]) -> void
{
    po::store(po::parse_command_line(argc, argv, ops_descr_), var_map_);
    po::notify(var_map_);
}

auto TestCaseUtility::make_options() -> boost::program_options::options_description
{
    po::options_description desc("Options");

    desc.add_options()
        ("help,h", "displays help message")
        ("config,c", po::value<fs::path>(), "XML describing test-case to generate")
        ("out-file,o", po::value<fs::path>(), "output file (default: ./input_arguments.bin)")
        ("test-case,t", po::value<fs::path>(), "test-case or test-case directory")
            ;

    return desc;
}

auto TestCaseUtility::process_options() -> void
{
    auto out_file = fs::path{"input_arguments.bin"};

    if(var_map_.size() == 0)
    {
        cout << "Missing arguments" << endl;
        cout << "Use '--help' for more details" << endl;
        exit(0);
    }
    if(var_map_.count("help"))
    {
        std::cout << ops_descr_ << "\n";
        std::cout << "Example --config file:\n"
                  << "<crete>\n"
                  << "\t<element>\n"
                  << "\t\t<name>name1</name>\n"
                  << "\t\t<data>deadbeef0123</data\n"
                  << "\t<\element>\n"
                  << "\t<element>\n"
                  << "\t\t<name>name2</name>\n"
                  << "\t\t<data>\n"
                  << "\t\t\t<fill value=\"ff\" size=\"8\"/>\n"
                  << "\t\t</data>\n"
                  << "\t</element>\n"
                  << "\t<element>\n"
                  << "\t\t<name>name3</name>\n"
                  << "\t\t<data>\n"
                  << "\t\t\t<file>my_data_file.bin</file>\n"
                  << "\t\t</data>\n"
                  << "\t</element>\n"
                  << "</crete>\n"
                  << std::endl;
        exit(0);
    }
    if(var_map_.count("out-file"))
    {
        out_file = var_map_["out-file"].as<fs::path>();

        CRETE_EXCEPTION_ASSERT(fs::exists(out_file), err::file_missing{out_file.string()});
    }
    if(var_map_.count("config"))
    {
        auto config = var_map_["config"].as<fs::path>();

        CRETE_EXCEPTION_ASSERT(fs::exists(config), err::file_missing{config.string()});

        pt::ptree tree;
        {
            fs::ifstream ifs(config,
                             std::ios::in | std::ios::binary);

            pt::read_xml(ifs,
                         tree);
        }

        auto tc = crete::util::generate_test_case(tree);

        {
            fs::ofstream ofs(out_file,
                             std::ios::out | std::ios::binary);

            crete::write_serialized(ofs, tc);
        }

        std::cout << tc << std::endl;
    }
    if(var_map_.count("test-case"))
    {
        auto tcp = var_map_["test-case"].as<fs::path>();

        CRETE_EXCEPTION_ASSERT(fs::exists(tcp), err::file_missing{tcp.string()});

        auto printer = [](const fs::path& tc)
        {
            std::cout << tc << ":\n";

            fs::ifstream ifs(tc,
                             std::ios::in | std::ios::binary);

            std::cout << read_serialized(ifs) << "\n";
        };

        if(fs::is_directory(tcp))
        {
            for(const auto p : fs::directory_iterator(tcp))
            {
                printer(p);
            }
        }
        else
        {
            printer(tcp);
        }
    }
}

} // namespace util
} // namespace crete

int main(int argc, char* argv[])
{
    try
    {
         auto tc_util = crete::util::TestCaseUtility{argc, argv};

//        pt::ptree config;
//        {
//            std::ifstream ifs(argv[1]);

//            pt::read_xml(ifs,
//                         config);
//        }

//        auto tc = crete::util::generate_test_case(config);

//        {
//            std::ofstream ofs("input_arguments.bin");

//            crete::write_serialized(ofs, tc);
//        }

//        std::cerr << tc << std::endl;
    }
    catch(std::exception& e)
    {
        std::cerr << boost::diagnostic_information(e) << std::endl;
    }

    return 0;
}
