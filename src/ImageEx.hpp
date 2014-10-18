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

#ifndef IMAGE_EX_HPP
#define IMAGE_EX_HPP

#include "planes/Plane.hpp"
#include <QString>
#include <QFile>
#include <QImage>
#include <algorithm>
#include <vector>

class ImageEx{
	public:
		const static unsigned MAX_PLANES = 4;
		enum system{
			GRAY,
			RGB,
			YUV
		};
		enum YuvSystem{
			SYSTEM_KEEP,
			SYSTEM_REC601,
			SYSTEM_REC709
		};
		enum Settings{
			SETTING_NONE = 0x0,
			SETTING_DITHER = 0x1,
			SETTING_GAMMA = 0x2
		};
	
	private:
		std::vector<Plane> planes;
		Plane alpha;
		system type;
		bool read_dump_plane( QIODevice &dev );
		bool from_dump( QString path );
		bool from_png( const char* path );
		bool from_qimage( QString path );
		
	public:
		ImageEx( system type = RGB ) : type( type ){ }
		ImageEx( Plane p ) : type( GRAY ){
			if( p.valid() )
				planes.push_back( p );
		}
		
		unsigned size() const{ return planes.size(); }
		void to_grayscale();
		
		bool create( unsigned width, unsigned height, bool alpha=false );
		
		template<typename... Args>
		void apply( Plane (Plane::*func)( Args... ) const, Args... args ){
			if( type == YUV )
				planes[0] = (planes[0].*func)( args... );
			else
				applyAll( false, func, args... );
		}
		template<typename... Args>
		void applyAll( bool do_alpha, Plane (Plane::*func)( Args... ) const, Args... args ){
			for( auto& plane : planes )
				plane = (plane.*func)( args... );
			if( do_alpha && alpha )
				alpha = (alpha.*func)( args... );
		}
		
		template<typename... Args>
		ImageEx copyApply( Args... args ) const{
			ImageEx temp( *this );
			temp.apply( args... );
			return temp;
		}
		
		template<typename... Args>
		ImageEx copyApplyAll( Args... args ) const{
			ImageEx temp( *this );
			temp.applyAll( args... );
			return temp;
		}
		
		bool is_valid() const{ return planes.size() > 0; }
		
		bool read_file( QString path );
		bool saveDump( QString path, unsigned depth=10 ) const;
		Plane& alpha_plane(){ return alpha; }
		const Plane& alpha_plane() const{ return alpha; }
		
		QImage to_qimage( YuvSystem system, unsigned setting=SETTING_NONE );
		
		unsigned get_width() const{
			if( planes.size() == 0 )
				return 0;
			return std::max_element( planes.begin(), planes.end()
				,	[]( const Plane& p1, const Plane& p2 ){ return p1.get_width() < p2.get_width(); } )->get_width();
		}
		unsigned get_height() const{
			if( planes.size() == 0 )
				return 0;
			return std::max_element( planes.begin(), planes.end()
				,	[]( const Plane& p1, const Plane& p2 ){ return p1.get_height() < p2.get_height(); } )->get_height();
		}
		
		system get_system() const{ return type; }
		
		
		double diff( const ImageEx& img, int x, int y ) const;
		bool is_interlaced() const;
		void replace_line( ImageEx& img, bool top );
		void combine_line( ImageEx& img, bool top );
		
		void scale( unsigned width, unsigned height ){
			for( auto& plane : planes )
				plane = plane.scale_cubic( width, height );
			if( alpha_plane() )
				alpha_plane() = alpha_plane().scale_cubic( width, height );
		}
		void scaleFactor( double factor_x, double factor_y )
			{ scale( get_width() * factor_x + 0.5, get_height() * factor_y + 0.5 ); }
		void crop( unsigned left, unsigned top, unsigned right, unsigned bottom );
		
		MergeResult best_vertical( const ImageEx& img, int level, double range, DiffCache *cache=nullptr ) const{
			return best_round( img, level, 0, range, cache );
		}
		MergeResult best_horizontal( const ImageEx& img, int level, double range, DiffCache *cache=nullptr ) const{
			return best_round( img, level, range, 0, cache );
		}
		MergeResult best_round( const ImageEx& img, int level, double range_x, double range_y, DiffCache *cache=nullptr ) const;
		
		Plane& operator[]( unsigned index ){ return planes[index]; }
		const Plane& operator[]( unsigned index ) const{ return planes[index]; }
};

#endif