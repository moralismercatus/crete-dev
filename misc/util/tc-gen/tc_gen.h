/******************************************************************************
 * Author(s): Christopher J. Havlicek
 *
 * For contact information, see CONTACTS.TXT
 * For license information, see LICENSE.TXT
 ******************************************************************************/

#ifndef CRETE_UTIL_TC_GEN_H
#define CRETE_UTIL_TC_GEN_H

#include <istream>

#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>

#include <crete/test_case.h>

namespace crete
{
namespace util
{

auto generate_test_case(const boost::property_tree::ptree& complete_tree) -> TestCase;

class TestCaseUtility
{
public:
    TestCaseUtility(int argc, char* argv[]);

    auto parse_options(int argc, char* argv[]) -> void;
    auto make_options() -> boost::program_options::options_description;
    auto process_options() -> void;

private:
    boost::program_options::options_description ops_descr_;
    boost::program_options::variables_map var_map_;
};

} // namespace util
} // namespace crete

#endif // CRETE_UTIL_TC_GEN_H

