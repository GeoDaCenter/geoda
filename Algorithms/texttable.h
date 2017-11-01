/**
 * GeoDa TM, Copyright (C) 2011-2015 by Luc Anselin - all rights reserved
 *
 * This file is part of GeoDa.
 *
 * GeoDa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GeoDa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * This texttable.h is modified from https://github.com/haarcuba/cpp-text-table
 *
 */

#ifndef __GEODA_CENTER_TEXTTABLE_H
#define __GEODA_CENTER_TEXTTABLE_H

#include <iostream>
#include <map>
#include <iomanip>
#include <vector>
#include <string>

class TextTable {
    
public:
    enum class Alignment { LEFT, RIGHT };
    typedef std::vector< std::string > Row;
    TextTable( char horizontal = '-', char vertical = '|', char corner = '+' ) :
    _horizontal( horizontal ),
    _vertical( vertical ),
    _corner( corner )
    {}
    
    void setAlignment( unsigned i, Alignment alignment )
    {
        _alignment[ i ] = alignment;
    }
    
    Alignment alignment( unsigned i ) const
    { return _alignment[ i ]; }
    
    char vertical() const
    { return _vertical; }
    
    char horizontal() const
    { return _horizontal; }
    
    void add( std::string const & content )
    {
        _current.push_back( content );
    }
    
    void endOfRow()
    {
        _rows.push_back( _current );
        _current.assign( 0, "" );
    }
    
    template <typename Iterator>
    void addRow( Iterator begin, Iterator end )
    {
        for( auto i = begin; i != end; ++i ) {
            add( * i );
        }
        endOfRow();
    }
    
    template <typename Container>
    void addRow( Container const & container )
    {
        addRow( container.begin(), container.end() );
    }
    
    std::vector< Row > const & rows() const
    {
        return _rows;
    }
    
    void setup() const
    {
        determineWidths();
        setupAlignment();
    }
    
    std::string ruler() const
    {
        std::string result;
        result += _corner;
        for( auto width = _width.begin(); width != _width.end(); ++ width ) {
            result += repeat( * width, _horizontal );
            result += _corner;
        }
        
        return result;
    }
    
    int width( unsigned i ) const
    { return _width[ i ]; }
    
private:
    char _horizontal;
    char _vertical;
    char _corner;
    Row _current;
    std::vector< Row > _rows;
    std::vector< unsigned > mutable _width;
    std::map< unsigned, Alignment > mutable _alignment;
    
    static std::string repeat( unsigned times, char c )
    {
        std::string result;
        for( ; times > 0; -- times )
            result += c;
        
        return result;
    }
    
    unsigned columns() const
    {
        return _rows[ 0 ].size();
    }
    
    void determineWidths() const
    {
        _width.assign( columns(), 0 );
        for ( auto rowIterator = _rows.begin(); rowIterator != _rows.end(); ++ rowIterator ) {
            Row const & row = * rowIterator;
            for ( unsigned i = 0; i < row.size(); ++i ) {
                _width[ i ] = _width[ i ] > row[ i ].size() ? _width[ i ] : row[ i ].size();
            }
        }
    }
    
    void setupAlignment() const
    {
        for ( unsigned i = 0; i < columns(); ++i ) {
            if ( _alignment.find( i ) == _alignment.end() ) {
                _alignment[ i ] = Alignment::LEFT;
            }
        }
    }
};

std::ostream & operator<<( std::ostream & stream, TextTable const & table )
{
    table.setup();
    stream << table.ruler() << "\n";
    for ( auto rowIterator = table.rows().begin(); rowIterator != table.rows().end(); ++ rowIterator ) {
        TextTable::Row const & row = * rowIterator;
        stream << table.vertical();
        for ( unsigned i = 0; i < row.size(); ++i ) {
            auto alignment = table.alignment( i ) == TextTable::Alignment::LEFT ? std::left : std::right;
            stream << std::setw( table.width( i ) ) << alignment << row[ i ];
            stream << table.vertical();
        }
        stream << "\n";
        stream << table.ruler() << "\n";
    }
    
    return stream;
}

#endif
