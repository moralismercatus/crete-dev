#ifndef CRETE_TC_GEN_H
#define CRETE_TC_GEN_H

#include <istream>

#include <boost/property_tree/ptree.hpp>

#include <crete/test_case.h>

namespace crete
{
namespace util
{

auto generate_test_case(const boost::property_tree::ptree& complete_tree) -> TestCase;

}
}

#endif // CRETE_TC_GEN_H

