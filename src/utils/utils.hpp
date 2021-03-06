/*
	This file is part of Overmix.

	Overmix is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Overmix is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Overmix.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <vector>

namespace util{

	template<typename T>
	bool removeItems( std::vector<T>& container, unsigned pos, unsigned amount ){
		if( container.size() < pos+amount )
			return false;
		
		auto start = container.begin() + pos;
		auto stop = start + amount;
		std::move( stop, container.end(), start );
		container.resize( container.size() - amount );
		
		return true;
	}
	
	template<typename T, typename Func>
	void removeItemsIf( std::vector<T>& container, Func f )
		{ container.erase( std::remove_if( container.begin(), container.end(), f ), container.end() ); }

}

#endif
