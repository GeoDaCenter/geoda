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
 */

#include "AbstractShape.h"
#include "../GenUtils.h"

void AbstractShape::Assign(char* nme)
{
	while (*nme == ' ') ++nme;
	strncpy(Id, nme, GdaConst::ShpObjIdLen-1);
}

void AbstractShape::Identify(const long d) {
	GenUtils::longToString(d, Id, 10);
}

void AbstractShape::ReadID(std::istream& s)
{
	long int value;
	char ch;
	while (s >> ch)
		if ((ch >= '0' && ch <= '9') || ch  == '-' || ch == '+' || ch ==  '"')
			break;
	if (s.fail()) {
		Identify(0);
		return;
	}
	if (ch == '"') {
		s.get(Id, GdaConst::ShpObjIdLen - 1, '"');
		if (ch == '"') s >> ch;
	} else {
		s.putback(ch);
		s >> value;
		Identify(value);
	}
}

void AbstractShape::WriteID(std::ostream& s, const long pts) const
{
	s << "  ";
	if ((Id[0] >= '0' && Id[0] <= '9') || Id[0] == '-')
		s << Id;
	else
		s << '"' << Id << '"';
	s << "  " << pts << std::endl;
}

