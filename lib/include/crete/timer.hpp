/******************************************************************************
 * Author(s): Christopher J. Havlicek
 *
 * For contact information, see CONTACTS.TXT
 * For license information, see LICENSE.TXT
 ******************************************************************************/
#ifndef CRETE_TIMER_HPP
#define CRETE_TIMER_HPP

#include <boost/timer/timer.hpp>
#include <fmt/format.h>

#define CRETE_TIME_SCOPE( msg ) \
    fmt::print( "{}...\n", msg ); \
    boost::timer::auto_cpu_timer \
        crete_scope_timer( fmt::format( "{} done: %ws\n", msg ) ); 

#endif // CRETE_TIMER_HPP
