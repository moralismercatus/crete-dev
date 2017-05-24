/******************************************************************************
 * Author(s): Christopher J. Havlicek
 *
 * For contact information, see CONTACTS.TXT
 * For license information, see LICENSE.TXT
 ******************************************************************************/

#ifndef CRETE_UTIL_OVMF_COV_H
#define CRETE_UTIL_OVMF_COV_H

#include <istream>
#include <unordered_map>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/filesystem/path.hpp>

#include <crete/test_case.h>

namespace crete
{
namespace util
{
namespace ovmf
{

class CoverageUtility
{
public:
    using Address = std::uint64_t;
    using Offset = std::uint64_t;

public:
    CoverageUtility(int argc, char* argv[]);

    auto parse_options(int argc, char* argv[]) -> void;
    auto make_options() -> boost::program_options::options_description;
    auto process_options() -> void;

    auto process() -> void;

private:
    boost::program_options::options_description ops_descr_;
    boost::program_options::variables_map var_map_;
    boost::filesystem::path build_dir_;
    boost::filesystem::path serial_log_;
    boost::filesystem::path trace_seq_;
};

auto collate_serial_log_data( const boost::filesystem::path log )
    -> std::unordered_map< std::string, CoverageUtility::Address >;
auto find_debug_files(const boost::filesystem::path build_dir, const vector<string>& file_names )
    -> std::unordered_map< std::string, boost::filesystem::path >;
auto compute_code_offsets( const boost::filesystem::path& trace_seq
                         , const std::unordered_map< std::string
                                                   , CoverageUtility::Address > address_map )
    -> std::unordered_map< CoverageUtility::Address
                         , CoverageUtility::Offset >;

} // namespace ovmf
} // namespace util
} // namespace crete

#endif // CRETE_UTIL_TC_GEN_H

