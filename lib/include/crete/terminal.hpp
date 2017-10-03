/******************************************************************************
 * Author(s): Christopher J. Havlicek
 *
 * For contact information, see TERMTACTS.TXT
 * For license information, see LICENSE.TXT
 ******************************************************************************/

#include <tuple>
#include <string>

#include <sys/ioctl.h>
#include <stdio.h>

#include <crete/exception.h>

namespace crete
{
namespace term
{

auto const term_min_row = decltype( winsize::ws_row ){ 25 };
auto const term_min_col = decltype( winsize::ws_col ){ 80 };

namespace border
{
auto const top = std::pair< char, char >{ '\x6C', '\x6B'};
auto const left_top = std::pair< char, char >{ '\x6C', '\x71'};
auto const left_top_mid = std::pair< char, char >{ '\x6C', '\x77'};
auto const right_top = std::pair< char, char >{ '\x71', '\x6B'};
auto const right_top_cross = std::pair< char, char >{ '\x77', '\x6B'};
auto const bottom = std::pair< char, char >{ '\x6D', '\x6A'};
auto const left_bottom = std::pair< char, char >{ '\x6D', '\x71'};
auto const left_bottom_mid = std::pair< char, char >{ '\x6D', '\x76'};
auto const right_bottom = std::pair< char, char >{ '\x71', '\x6A'};
auto const right_bottom_cross = std::pair< char, char >{ '\x76', '\x6A'};
auto const mid = std::pair< char, char >{ '\x74', '\x75'};
auto const left_mid = std::pair< char, char >{ '\x74', '\x71'};
auto const left_mid_cross = std::pair< char, char >{ '\x74', '\x6E'};
auto const right_mid = std::pair< char, char >{ '\x71', '\x75'};
auto const right_mid_cross = std::pair< char, char >{ '\x6E', '\x75'};
auto const wall = std::pair< char, char >{ '\x78', '\x78'};
auto const left_wall = std::pair< char, char >{ '\x78', ' '};
auto const left_wall_mid = std::pair< char, char >{ '\x78', '\x6E'};
auto const right_wall = std::pair< char, char >{ ' ', '\x78'};
auto const right_wall_mid = std::pair< char, char >{ '\x78', '\x6E'};
auto const blank = std::pair< char, char >{ ' ', ' '};
}

namespace fill
{
auto const space = char{' '};
auto const line = char{'\x71'};
}

namespace color
{
auto const nrm = "\x1B[0m";
auto const red = "\x1B[31m";
auto const grn = "\x1B[32m";
auto const yel = "\x1b[33m";
auto const blu = "\x1b[34m";
auto const mag = "\x1b[35m";
auto const cyn = "\x1b[36m";
auto const wht = "\x1b[37m";
}

auto const reset_cursor = "\x1b[1;1h";
auto const reset_display =  "\x1b[2j";
auto const reset = "\x1b[1;1h" "\x1b[2j";

enum class Align
{
      center
    , left
    , right
};

class Line
{
public:
    Line( const unsigned int size
        , const std::string& data
        , const std::string& text_color
        , const char fill
        , const std::string& fill_color
        , const std::pair< char, char >& borders
        , const Align align
        , const bool line_break)
        : given_size_{ size }
        , text_color_{ text_color }
        , available_size( size - sizeof( borders_ ) )
        , leftmost_bound{ 1u }
        , rightmost_bound( available_size + sizeof( borders_.second ) )
        , fill_{ fill }
        , fill_color_ { fill_color }
        , line_break_{ line_break }
    {
        CRETE_EXCEPTION_ASSERT( ( data.size() + sizeof( borders ) ) <= size
                              , err::invalid{ "line data exceeds line size" } );

        data_.resize( size + 1 ); // +1 for null term.

        std::copy( data.begin(), data.end()
                 , data_.begin() );

        bound_.first = 0u;
        bound_.second = data.size();

        realign( bound_.first + 1
               , bound_.second + 1 );

        data_[size] = '\0';
        data_[0] = borders.first;
        data_[size - 1] = borders.second;

        fill_blanks();

        switch( align )
        {
        case Align::center:
            align_center();
            break;
        case Align::left:
            align_left();
            break;
        case Align::right:
            align_right();
            break;
        }
    }

    auto fill_blanks() -> void
    {
        for( auto i = leftmost_bound
           ; i < bound_.first
           ; ++i )
        {
            data_[i] = fill_;
        }
        for( auto i = bound_.second
           ; i < rightmost_bound
           ; ++i )
        {
            data_[i] = fill_;
        }
    }
    auto align_center() -> void
    {
        auto length = bound_.second - bound_.first;
        auto midpoint = available_size / 2;
        auto half_length = length / 2;

        auto b = leftmost_bound + midpoint - half_length;
        auto e = b + length;

        realign( b, e );
        fill_blanks();
    }
    auto align_right() -> void
    {
        auto length = bound_.second - bound_.first;

        auto b = rightmost_bound - length;
        auto e = rightmost_bound;

        realign( b, e );
        fill_blanks();
    }
    auto align_left() -> void
    {
        auto length = bound_.second - bound_.first;

        auto b = leftmost_bound;
        auto e = b + length;

        realign( b, e );
        fill_blanks();
    }
    auto data() const -> const char*
    {
        return data_.data();
    }
    auto export_ansi_box() const -> std::vector< char >
    {
        auto v = std::vector< char >{};

        std::copy( data_.begin(), data_.end()
                 , std::back_inserter( v ) );

        auto accum = 0u;
        v.insert( v.begin() + accum
                , fill_color_.begin(), fill_color_.end() );
        accum += fill_color_.size();
        v.insert( v.begin() + accum
                , '\x0E' );
        ++accum;
        v.insert( v.begin() + bound_.first + accum
                , '\x0F' );
        ++accum;
        v.insert( v.begin() + bound_.first + accum
                , text_color_.begin(), text_color_.end() );
        accum += text_color_.size();
        v.insert( v.begin() + bound_.second + accum
                , '\x0E' );
        ++accum;
        v.insert( v.begin() + bound_.second + accum
                , fill_color_.begin(), fill_color_.end() );
        accum += fill_color_.size();
        v.insert( v.begin() + given_size_ + accum
                , '\x0F' );
        ++accum;
        if( line_break_ )
        {
            v.insert( v.begin() + given_size_ + accum
                    , '\n' );
            ++accum;
        }


        return v;
    }

private:
    auto realign( unsigned begin
                , unsigned end ) -> void
    {
        auto length = bound_.second - bound_.first;

        assert( ( length ) == ( end - begin ) );
        assert( end <= rightmost_bound );

        decltype(data_) tmp = data_;

        for( auto i = 0u
           ; i < length
           ; ++i )
        {
            data_[begin + i] = tmp[bound_.first + i];
        }

        bound_.first = begin;
        bound_.second = end;
    }

private:
    unsigned given_size_;
    std::vector< char > data_;
    std::string text_color_;
    unsigned const available_size;
    unsigned const leftmost_bound; //  = 1u;
    unsigned const rightmost_bound; //  = available_size + sizeof( borders_.second );
    std::pair< unsigned, unsigned > bound_;
    char fill_;
    std::string fill_color_;
    std::pair< char, char > borders_;
    bool line_break_;
};

auto is_terminal_size_valid() -> std::pair< bool, std::string >
{
    struct winsize ws;

    if( ioctl( 1, TIOCGWINSZ, &ws ) )
        return { false, "failed to get window size" };

    if( ws.ws_row < term_min_row || ws.ws_col < term_min_col )
        return { false, "window not large enough to display status" };

    return { true, "" };
}

auto display_banner() -> void
{
    std::vector< Line > lines{
         { 80, "CRETE 1.0", color::grn, fill::space, color::nrm, border::blank, Align::center, true } // TODO: extract CRETE version from official source.
        // ---------------------------
        ,{ 40, "chronology", color::cyn, fill::line, color::nrm, border::left_top, Align::left, false }
        ,{ 40, "dispatch logistics", color::cyn, fill::line, color::nrm, border::right_top_cross, Align::left, true }
        ,{ 40, "  elapsed: 00d:00h:00m:00s", color::wht, fill::space, color::nrm, border::left_wall, Align::left, false }
        ,{ 40, "dispatch test cases: n/a", color::wht, fill::space, color::nrm, border::wall, Align::left, true }
        ,{ 40, " avg test: 00d:00h:00m:00s", color::wht, fill::space, color::nrm, border::left_wall, Align::left, false }
        ,{ 40, "dispatch traces: n/a", color::wht, fill::space, color::nrm, border::wall, Align::left, true }
        ,{ 40, "avg trace: 00d:00h:00m:00s", color::wht, fill::space, color::nrm, border::left_wall, Align::left, false }
        ,{ 40, "", color::nrm, fill::space, color::nrm, border::wall, Align::left, true }
        // ---------------------------
        ,{ 40, "overall logistics", color::cyn, fill::line, color::nrm, border::left_mid, Align::left, false }
        ,{ 40, "node logistics", color::cyn, fill::line, color::nrm, border::right_mid_cross, Align::left, true }
        ,{ 40, "total test cases: n/a", color::wht, fill::space, color::nrm, border::left_wall, Align::left, false }
        ,{ 40, "vm-nodes: n/a", color::wht, fill::space, color::nrm, border::wall, Align::left, true }
        ,{ 40, "total traces: n/a",color::wht, fill::space, color::nrm, border::left_wall, Align::left, false }
        ,{ 40, "svm-nodes: n/a", color::wht, fill::space, color::nrm, border::wall, Align::left, true }
        ,{ 40, "crashes: n/a", color::wht, fill::space, color::nrm, border::left_wall, Align::left, false }
        ,{ 40, "vm-node test cases: n/a", color::wht, fill::space, color::nrm, border::wall, Align::left, true }
        ,{ 40, "", color::wht, fill::space, color::nrm, border::left_wall, Align::left, false }
        ,{ 40, "svm-node traces: n/a",color::wht, fill::space, color::nrm, border::wall, Align::left, true }
        // ---------------------------
        ,{ 40, "throughput", color::cyn, fill::line, color::nrm, border::left_mid, Align::left, false }
        ,{ 40, "", color::cyn, fill::line, color::nrm, border::right_mid_cross, Align::left, true }
        ,{ 40, "test case: n/a", color::wht, fill::space, color::nrm, border::left_wall, Align::left, false }
        ,{ 40, "", color::wht, fill::space, color::nrm, border::wall, Align::left, true }
        ,{ 40, "trace: n/a", color::wht, fill::space, color::nrm, border::left_wall, Align::left, false }
        ,{ 40, "", color::wht, fill::space, color::nrm, border::wall, Align::left, true }
        // ---------------------------
        ,{ 40, "", color::nrm, fill::line, color::nrm, border::left_bottom, Align::left, false }
        ,{ 40, "", color::nrm, fill::line, color::nrm, border::right_bottom_cross, Align::left, true }
    };

    for( auto const& line : lines )
    {
        printf( "%s", line.export_ansi_box().data() );
    }
}

auto restore_terminal() -> void
{
    printf( reset_display );
    printf( reset );
}

} // namespace term
} // namespace crete
