/******************************************************************************
 * Author(s): Christopher J. Havlicek
 *
 * For contact information, see CONTACTS.TXT
 * For license information, see LICENSE.TXT
 ******************************************************************************/
#ifndef CRETE_ASSERT_HPP
#define CRETE_ASSERT_HPP

#define BOOST_ENABLE_ASSERT_HANDLER 1

#include <boost/assert.hpp>
#include <boost/stacktrace.hpp>
#include <fmt/format.h>
#include <sstream>
#include <cstdlib>

#define CRETE_ASSERT( expr ) BOOST_ASSERT( ( expr ) )
#define CRETE_ASSERT_MSG( expr, msg ) BOOST_ASSERT_MSG( ( expr ), ( msg ) )

#define CRETE_ABORT_MSG( msg ) BOOST_ASSERT_MSG( ( false ), ( msg ) )

#define CRETE_WARNING_ASSERT(pred, msg) if(!(pred)) { crete::print_assertion_failed_msg( #pred, fmt::format( "[Warning Only] {}",  msg ), __FUNCTION__, __FILE__, __LINE__ ); } 

namespace crete {

inline
void print_assertion_failed_msg( std::string const expr
                               , std::string const msg
                               , std::string const function
                               , std::string const file
                               , long line )
{
    using boost::stacktrace::stacktrace;
    using fmt::print;
    using std::exit;
    using std::stringstream;

#ifndef BOOST_STACKTRACE_USE_NOOP
    stringstream ss;

    ss << stacktrace() << '\n';

    print( stderr
         , "Assertion failed:\n Expression: {}\n Message: {}\n Function: {}\n File: {}\n Line: {}\n{}\n"
         , expr
         , msg
         , function
         , file
         , line
         , ss.str() );
#else
    print( stderr
         , "Assertion failed:\n Expression: {}\n Message: {}\n Function: {}\n File: {}\n Line: {}\n"
         , expr
         , msg
         , function
         , file
         , line );
#endif // BOOST_STACKTRACE_USE_NOOP
}

} // namespace crete

namespace boost {

inline
void assertion_failed( char const* expr
                     , char const* function
                     , char const* file
                     , long line)
{
    using crete::print_assertion_failed_msg;

    print_assertion_failed_msg( expr
                              , "N/A"
                              , function
                              , file
                              , line );

    exit( EXIT_FAILURE );
}

inline
void assertion_failed_msg( char const* expr
                         , char const* msg
                         , char const* function
                         , char const* file
                         , long line )
{
    using crete::print_assertion_failed_msg;

    print_assertion_failed_msg( expr
                              , msg
                              , function
                              , file
                              , line );

    exit( EXIT_FAILURE );
}

} // namespace boost

#endif // CRETE_ASSERT_HPP
