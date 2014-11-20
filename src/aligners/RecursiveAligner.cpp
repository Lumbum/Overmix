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


#include "RecursiveAligner.hpp"
#include "../containers/ImageContainer.hpp"
#include "../renders/AverageRender.hpp"

#include <limits>
using namespace std;

static void copyLine( Plane& plane_out, const Plane& plane_in, unsigned y_out, unsigned y_in ){
	auto out = plane_out.scan_line( y_out );
	auto in = plane_in.const_scan_line( y_in );
	for( unsigned ix=0; ix<plane_out.get_width(); ++ix )
		out[ix] = in[ix];
}

static void mergeLine( Plane& plane_out, const Plane& plane_in1, const Plane& plane_in2, unsigned y_out, unsigned y_in1, unsigned y_in2 ){
	auto out = plane_out.scan_line( y_out );
	auto in1 = plane_in1.const_scan_line( y_in1 );
	auto in2 = plane_in2.const_scan_line( y_in2 );
	for( unsigned ix=0; ix<plane_out.get_width(); ++ix )
		out[ix] = (in1[ix] + in2[ix]) / 2; //TODO: use precisison_color_type
}

static Plane mergeVertical( const Plane& p1, const Plane& p2, int offset ){
	auto top    = offset < 0 ? p2 : p1;
	auto bottom = offset < 0 ? p1 : p2;
	offset = abs(offset);
	unsigned height = max( top.get_height(), bottom.get_height() + offset );
	
	Plane out( p1.get_width(), height );
	
	//Copy top part
	for( int iy=0; iy < abs(offset); ++iy )
		copyLine( out, top, iy, iy );
	
	//Merge the shared middle part
	auto shared_height = min( top.get_height()-offset, bottom.get_height() );
	for( unsigned iy=0; iy<shared_height; ++iy )
		mergeLine( out, top, bottom, iy+offset, iy+offset, iy );
	
	//Copy bottom part, might be in the top plane, if it is larger than the bottom plane
	if( top.get_height() > bottom.get_height() + offset )
		for( unsigned iy=offset+shared_height; iy<height; ++iy )
			copyLine( out, top, iy, iy );
	else
		for( unsigned iy=shared_height; iy<bottom.get_height(); ++iy )
			copyLine( out, bottom, iy+offset, iy );
	
	return out;
}

pair<ImageEx,Point<double>> RecursiveAligner::combine( const ImageEx& first, const ImageEx& second ) const{
	auto offset = findOffset( first, second ).distance;
	
	if( offset.x == 0 && !first.alpha_plane() && !second.alpha_plane() && first[0].get_width() == second[0].get_width() )
		return { mergeVertical( first[0], second[0], offset.y ), offset };
	else{
		//Wrap planes in ImageContainer
		//TODO: Optimize this
		ImageContainer container;
		container.addImage( ImageEx( first ) ); //We are having copies here!!
		container.addImage( ImageEx( second ) );
		container.setPos( 1, offset );
		
		//Render it
		return { AverageRender( false, true ).render( container ), offset };
	}
}

static void addToWatcher( AProcessWatcher* watcher, int add ){
	if( watcher )
		watcher->setCurrent( watcher->getCurrent() + add );
}

ImageEx RecursiveAligner::align( AProcessWatcher* watcher, unsigned begin, unsigned end ){
	auto amount = end - begin;
	switch( amount ){
		case 0: qFatal( "No images to align!" );
		case 1: addToWatcher( watcher, 1 );
				return { image( begin )[0], alpha( begin) }; //Just return this one
		case 2: { //Optimization for two images
				ImageEx first ( image( begin   )[0], alpha( begin   ) );
				ImageEx second( image( begin+1 )[0], alpha( begin+1 ) );
				auto offset = combine( first, second );
				setPos( begin+1, pos(begin) + offset.second );
				addToWatcher( watcher, 2 );
				return offset.first;
			}
		default: { //More than two images
				//Solve sub-areas recursively
				unsigned middle = amount / 2 + begin;
				auto offset = combine( align( watcher, begin, middle ), align( watcher, middle, end ) );
				
				//Find top-left corner of first
				auto corner1 = Point<double>( numeric_limits<double>::max(), numeric_limits<double>::max() );
				auto corner2 = corner1;
				for( unsigned i=begin; i<middle; i++ )
					corner1 = corner1.min( pos(i) );
				//Find top-left corner of second
				for( unsigned i=middle; i<end; i++ )
					corner2 = corner2.min( pos(i) );
				
				//move all in "middle to end" using the offset
				for( unsigned i=middle; i<end; i++ )
					setPos( i, pos( i ) + corner1 + offset.second - corner2 );
				
				return offset.first; //Return the combined image
			}
	}
}

void RecursiveAligner::align( AProcessWatcher* watcher ){
	if( count() == 0 )
		return;
	
	ProgressWrapper( watcher ).setTotal( count() );
	
	raw = true;
	align( watcher, 0, count() );
	raw = false;
}

