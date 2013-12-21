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


#include "PlaneRender.hpp"
#include "AImageAligner.hpp"
#include "Plane.hpp"
#include "ImageEx.hpp"
#include "color.hpp"

#include "MultiPlaneIterator.hpp"

#include <QTime>
#include <QRect>
#include <vector>
using namespace std;


ImageEx* PlaneRender::render( const AImageAligner& aligner, unsigned max_count ) const{
	if( max_count > aligner.count() )
		max_count = aligner.count();
	
	//Abort if no images
	if( max_count == 0 ){
		qWarning( "No images to render!" );
		return nullptr;
	}
	qDebug( "render_image: image count: %d", (int)max_count );
	
	//Do iterator
	QRect full = aligner.size();
	ImageEx *img = new ImageEx( ImageEx::GRAY );
	if( !img )
		return NULL;
	img->create( 1, 1 ); //TODO: set as initialized
	
	
	//Create output plane
	Plane* out = new Plane( full.width(), full.height() );
	out->fill( color::BLACK );
	img->replace_plane( 0, out );
	
	//Initialize PlaneItInfos
	vector<PlaneItInfo> info;
	info.push_back( PlaneItInfo( out, full.x(),full.y() ) );
	
	for( unsigned i=0; i<max_count; i++ )
		info.push_back( PlaneItInfo(
				aligner.plane( i, 0 )
			,	round( aligner.pos(i).x() )
			,	round( aligner.pos(i).y() )
			) );
	
	//Execute
	MultiPlaneIterator it( info );
	it.data = data();
	it.iterate_all();
	it.for_all_pixels( pixel() );
	
	return img;
}


