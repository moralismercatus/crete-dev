/******************************************************************************
 * Author(s): Christopher J. Havlicek
 *
 * For contact information, see CONTACTS.TXT
 * For license information, see LICENSE.TXT
 ******************************************************************************/

#include "ovmf_cov.hpp"
#include <crete/exception.h>

#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>

#include <sstream>
#include <fstream>
#include <regex>
#include <unordered_map>
#include <unordered_set>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
namespace pt = boost::property_tree;

namespace crete
{
namespace util
{
namespace ovmf
{

CoverageUtility::CoverageUtility(int argc, char *argv[])
    : ops_descr_(make_options())
{
    parse_options(argc, argv);
    process_options();
}

auto CoverageUtility::parse_options(int argc, char* argv[]) -> void
{
    po::store(po::parse_command_line(argc, argv, ops_descr_), var_map_);
    po::notify(var_map_);
}

auto CoverageUtility::make_options() -> boost::program_options::options_description
{
    po::options_description desc{ "Options" };

    desc.add_options()
        ( "help,h"                                          ,"displays help message" )
        ( "build,b"      ,po::value<fs::path>()->required() ,"path to EDK2 build directory" )
        ( "trace-seq,t"  ,po::value<fs::path>()             ,"path to trace sequence file" )
        ( "serial-log,s" ,po::value<fs::path>()->required() ,"path to QEMU serial output file" )
        ;

    return desc;
}

auto CoverageUtility::process_options() -> void
{
    if( var_map_.size() == 0 )
    {
        cout << "Missing arguments" << endl;
        cout << "Use '--help' for more details" << endl;
        exit(0); // TODO: tenuous. No stack unwinding. Use special exception.
    }
    if( var_map_.count( "help" ) )
    {
        std::cout << ops_descr_
                  << std::endl;
        exit(0); // TODO: tenuous. No stack unwinding. Use special exception.
    }
    if( var_map_.count( "build" ) )
    {
        auto build_dir = var_map_[ "build" ].as< fs::path >();

        CRETE_EXCEPTION_ASSERT( fs::exists( build_dir )
                              , err::file_missing{ build_dir.string() } );

        build_dir_ = build_dir;
    }
    if( var_map_.count( "trace-seq" ) )
    {
        auto trace_seq = var_map_[ "trace-seq" ].as< fs::path >();

        CRETE_EXCEPTION_ASSERT( fs::exists( trace_seq )
                              , err::file_missing{ trace_seq.string() } );

        trace_seq_ = trace_seq;
    }
    if( var_map_.count( "serial-log" ) )
    {
        auto serial_log = var_map_[ "serial-log" ].as< fs::path >();

        CRETE_EXCEPTION_ASSERT( fs::exists( serial_log )
                              , err::file_missing{ serial_log.string() } );

        serial_log_ = serial_log;
    }
}

auto CoverageUtility::process() -> void
{
    auto address_map = collate_serial_log_data( serial_log_ );
    auto efi_files = std::vector< std::string >{};

    for( const auto& e : address_map )
    {
        efi_files.emplace_back(e.first);
    }

    auto debug_files = find_debug_files( build_dir_, efi_files );
    auto code_locs = compute_code_locations( trace_seq_, address_map );

}

auto collate_serial_log_data( const fs::path log )
    -> std::unordered_map< std::string, std::uint64_t >
{
    using boost::algorithm::ends_with;
    using boost::algorithm::erase_all;

    fs::ifstream ifs{ log };

    CRETE_EXCEPTION_ASSERT( ifs.good()
                          , err::file_open_failed{ log.string() } );

    auto address_map = std::unordered_map< std::string, std::uint64_t >{};
    auto dxe_entered = false;

    for( auto line = string{}
       ; std::getline( ifs, line )
       ; )
    {
        erase_all(line, "\n");
        erase_all(line, "\r");

        if( dxe_entered )
        {
            if( ends_with( line, ".efi" ) )
            {
                auto address = std::string{};

                        std::cout << line << std::endl;
                {
                    const auto rgx = std::regex{ "0x[0-9A-F]+" };
                    auto match = std::smatch{};

                    if( std::regex_search( line, match, rgx ) )
                    {
                        address = match[0];
                    }
                    else
                    {
                        CRETE_EXCEPTION_THROW( err::parse{ "failed to find address in serial log" } );
                    }
                }
                {
                    const auto rgx = std::regex{ "[0-9a-zA-Z]+\\.efi" };
                    auto match = std::smatch{};

                    if( std::regex_search( line, match, rgx ) )
                    {
                        address_map.emplace( match[0], stoull( address , 0, 16) );
                    }
                    else
                    {
                        CRETE_EXCEPTION_THROW( err::parse{ "failed to find .efi file in serial log" } );
                    }
                }
            }
        }
        else if( line == "DXE IPL Entry" )
        {
            dxe_entered = true;
        }
    }

    for( const auto p : address_map )
    {
        std::cout << std::hex;
        std::cout << p.first << ":0x" << p.second << std::endl;
        std::cout << std::dec;
    }

    return address_map;
}

auto find_debug_files( const fs::path build_dir
                     , const vector< std::string >& file_names )
    -> std::unordered_map< std::string, fs::path >
{
    auto debug_files = std::unordered_map< std::string, fs::path >{};
    auto file_name_set = std::unordered_set< std::string >{ file_names.begin(), file_names.end() };

    for( const auto& de : fs::recursive_directory_iterator{ build_dir } )
    {
        auto p = de.path();
        if( p.extension() == ".debug" )
        {
            auto fn = fs::path{p}.replace_extension( "efi" ).filename().string();

            if( file_name_set.count( fn ) > 0 )
            {
                debug_files.emplace( fn, p );

                std::cout << fn << ":" << p.string() << std::endl;
            }
        }
    }

    return debug_files;
}

auto compute_code_offsets( const fs::path& trace_seq
                         , const std::unordered_map< std::string
                                                   , CoverageUtility::Address > address_map )
    -> std::unordered_map< CoverageUtility::Address
                         , CoverageUtility::Offset >
{
    auto offset_map = std::unordered_map< CoverageUtility::Address
                                        , CoverageUtility::Offset >{};
    auto seq = std::vector< CoverageUtility::Offset >{};

    {
        fs::ifstream ifs{ trace_seq };

        CRETE_EXCEPTION_ASSERT( ifs.good()
                              , err::file_open_failed{ trace_seq.string() } );

        auto addr = CoverageUtility::Address{};

        while( ifs >> addr )
        {
            // 1. find which range addr belongs in
            // 2. compute the offset = addr - base
            // 3. emplace { addr, offset }
            // Note: this may better return vector<pair<debug_file, offset>>,
            // that describes the trace seq in terms of the debug file and the offsets
            // covered within it. For starters, it'd be sufficient to simply print this
            // info. In the future, more convenient applications can be expounded upon.
        }
    }
}

} // namespace ovmf
} // namespace util
} // namespace crete

int main(int argc, char* argv[])
{
    try
    {
        auto util = crete::util::ovmf::CoverageUtility{argc, argv};

        util.process();
    }
    catch(std::exception& e)
    {
        std::cerr << boost::diagnostic_information(e) << std::endl;
    }

    return 0;
}
