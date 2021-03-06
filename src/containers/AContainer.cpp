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


#include "AContainer.hpp"

#include "../planes/ImageEx.hpp"
#include "../comparators/AComparator.hpp"

#include <stdexcept>

using namespace Overmix;


const Plane& AContainer::alpha( unsigned index ) const{
	return image( index ).alpha_plane();
}

const Plane& AContainer::mask( unsigned index ) const
	{ throw std::logic_error( "AContainer::mask not defined for this container type!" ); }

const Plane& AContainer::plane( unsigned index ) const{
	auto& img = image( index );
	if( img.size() < 1 )
		throw std::out_of_range( "No planes in ImageEx!" );
	//TODO: with RGB we should use the Green channel instead of Red
	return img[0];
}

void AContainer::cropImage( unsigned index, unsigned left, unsigned top, unsigned right, unsigned bottom ){
	auto& img = imageRef( index );
	auto offset = img.crop( left, top, right, bottom );
	setPos( index, pos( index ) + offset );
}

void AContainer::cropImage( unsigned index, Rectangle<double> area ){
	auto& img = imageRef( index );
	if( area.intersects( { pos(index), img.getSize() }) ){
		auto offset = area.pos - pos(index);
		auto real = offset.max( {0,0} ).to<unsigned>();
		auto size = (area.size - (real - offset)).min( img.getSize() );
		img.crop( offset, size );
		setPos( index, pos(index) + real );
		
		if( size != area.size.to<unsigned>() )
			img = ImageEx();
	}
	else
		img = ImageEx();
	
}

void AContainer::scaleImage( unsigned index, Point<double> scale, ScalingFunction scaling ){
	setPos( index, pos( index ) * scale );
	
	auto& img = imageRef( index );
	if( img.getSize() == (img.getSize() * scale).round().to<unsigned>() )
		return; //Don't attempt to scale, if it will not change the size
	img.scaleFactor( scale, scaling );
}

/** @return The smallest rectangle which can contain all the images */
Rectangle<double> AContainer::size() const{
	auto min = minPoint();
	auto max = min;
	for( unsigned i=0; i<count(); ++i )
		max = max.max( image(i).getSize().to<double>() + pos(i) );
	return { min, max-min };
}

/** @return The lowest x and y cordinates of all items */
Point<double> AContainer::minPoint() const{
	if( count() == 0 )
		return { 0, 0 };
	
	Point<double> min = pos( 0 );
	for( unsigned i=0; i<count(); i++ )
		min = min.min( pos(i) );
	
	return min;
}

/** @return The highest x and y cordinates of all items */
Point<double> AContainer::maxPoint() const{
	if( count() == 0 )
		return { 0, 0 };
	
	Point<double> max = pos( 0 );
	for( unsigned i=0; i<count(); i++ )
		max = max.max( pos(i) );
	
	return max;
}

std::pair<bool,bool> AContainer::hasMovement() const{
	std::pair<bool,bool> movement( false, false );
	if( count() == 0 )
		return movement;
	
	auto fixed = pos( 0 );
	for( unsigned i=1; i<count() && (!movement.first || !movement.second); ++i ){
		auto current = pos( i );
		movement.first  = movement.first  || current.x != fixed.x;
		movement.second = movement.second || current.y != fixed.y;
	}
	
	return movement;
}

static void addToFrames( std::vector<int>& frames, int frame ){
	for( auto& item : frames )
		if( item == frame )
			return;
	frames.push_back( frame );
}

std::vector<int> AContainer::getFrames() const{
	std::vector<int> frames;
	for( unsigned i=0; i<count(); ++i )
			if( frame( i ) >= 0 )
				addToFrames( frames, frame( i ) );
	
	//Make sure to include at least include
	if( frames.size() == 0 )
		frames.push_back( -1 );
	
	return frames;
}

void AContainer::setCachedOffset( unsigned, unsigned, ImageOffset ){
	qDebug( "No caching of alignment offsets implemented in this container" );
}

ImageOffset AContainer::getCachedOffset( unsigned index1, unsigned index2 ) const{
	if( hasCachedOffset( index1, index2 ) )
		throw std::logic_error( "AContainer::getCachedOffset not implemented!" );
	else
		throw std::logic_error( "AContainer::getCachedOffset called when hasCachedOffset returned false" );
}

ImageOffset AContainer::findOffset( unsigned index1, unsigned index2 ){
	if( hasCachedOffset( index1, index2 ) )
		return getCachedOffset( index1, index2 );
	
	auto offset = getComparator()->findOffset( plane(index1), plane(index2), alpha(index1), alpha(index2) );
	setCachedOffset( index1, index2, offset );
	return offset;
}
