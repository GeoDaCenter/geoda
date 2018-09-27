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
    enum Alignment { LEFT, RIGHT };
    enum MODE { ASCII, MD, LATEX };
    
    typedef std::vector< std::string > Row;
    TextTable( char horizontal = '-', char vertical = '|', char corner = '+' ) :
    _horizontal( horizontal ),
    _vertical( vertical ),
    _corner( corner ),
    mode( TextTable::MD )
    {}
    
    TextTable( MODE m ) : mode( m )
    {
        if ( m == TextTable::MD ) {
            _horizontal = '-';
            _vertical = '|';
            _corner = '|';
        }
    }
    
    void setAlignment( unsigned i, Alignment alignment )
    {
        _alignment[ i ] = alignment;
    }
    
    Alignment alignment( unsigned i ) const
    { return _alignment[ i ]; }
    
    char vertical() const
    { return _vertical; }
   
    void setVertical( char c)
    { _vertical = c;}
    
    char horizontal() const
    { return _horizontal; }
    void setHorizontal( char c)
    { _horizontal = c; }
    
    void setCorner( char c)
    { _corner = c; }
    
    void add( std::string const & content )
    {
        _current.push_back( content );
    }
    
    void endOfRow()
    {
        _rows.push_back( _current );
        _current.assign( 0, "" );
    }
   
/* 
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
 */   
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
        std::vector<unsigned>::iterator width;
        for( width = _width.begin(); width != _width.end(); ++ width ) {
            result += repeat( * width, _horizontal );
            result += _corner;
        }
        
        return result;
    }
    
    int width( unsigned i ) const
    { return _width[ i ]; }
    
    MODE mode;
    
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
        for ( int r=0; r < _rows.size(); r++) {
            Row const & row = _rows[r];
            for ( unsigned i = 0; i < row.size(); ++i ) {
                _width[ i ] = _width[ i ] > row[ i ].size() ? _width[ i ] : row[ i ].size();
            }
        }
    }
    
    void setupAlignment() const
    {
        for ( unsigned i = 0; i < columns(); ++i ) {
            if ( _alignment.find( i ) == _alignment.end() ) {
                _alignment[ i ] = TextTable::LEFT;
            }
        }
    }
};

std::ostream & operator<<( std::ostream & stream, TextTable const & table )
{
    table.setup();
    if (table.mode == TextTable::ASCII) {
        stream << table.ruler() << "\n";
        std::vector< TextTable::Row > const & rows = table.rows();
        for (int r=0; r<rows.size(); r++) {
            TextTable::Row const & row = rows[r];
            stream << table.vertical();
            for ( unsigned i = 0; i < row.size(); ++i ) {
                if( table.alignment( i ) == TextTable::LEFT) 
                stream << std::setw( table.width( i ) ) << std::left << row[ i ];
                else
                stream << std::setw( table.width( i ) ) << std::right << row[ i ];
                stream << table.vertical();
            }
            stream << "\n";
            stream << table.ruler() << "\n";
        }
        
    } else if (table.mode == TextTable::MD ) {
        int idx = 0;
        std::vector< TextTable::Row > const & rows = table.rows();
        for (int r=0; r<rows.size(); r++) {
            TextTable::Row const & row = rows[r];
            stream << table.vertical();
            for ( unsigned i = 0; i < row.size(); ++i ) {
                if( table.alignment( i ) == TextTable::LEFT) 
                stream << std::setw( table.width( i ) ) << std::left << row[ i ];
                else
                stream << std::setw( table.width( i ) ) << std::right << row[ i ];
                stream << table.vertical();
            }
            stream << "\n";
            if (idx == 0) stream << table.ruler() << "\n";
            idx ++;
        }

    }
    
    return stream;
}

#endif
