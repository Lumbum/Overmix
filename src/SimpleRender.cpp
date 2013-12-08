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


#include "SimpleRender.hpp"
#include "AImageAligner.hpp"
#include "Plane.hpp"
#include "ImageEx.hpp"

#include "MultiPlaneIterator.hpp"

#include <QTime>
#include <QRect>
#include <vector>
using namespace std;

static void render_average( MultiPlaneIterator &it, bool alpha_used ){
	unsigned start_plane = alpha_used ? 2 : 1;
	it.data = (void*)&start_plane;
	
	if( it.iterate_all() ){
	//Do average and store in [0]
	it.for_all_pixels( [](MultiPlaneLineIterator &it){
			unsigned start_plane = *(unsigned*)it.data;
			unsigned avg = 0;
			for( unsigned i=start_plane; i<it.size(); i++ )
				avg += it[i];
			
			if( it.size() > start_plane )
				it[0] = avg / (it.size() - start_plane); //NOTE: Will crash if image contains empty parts
			else
				it[0] = 0;
		} );
	}
	else{
	//Do average and store in [0]
	it.for_all_pixels( [](MultiPlaneLineIterator &it){
			unsigned start_plane = *(unsigned*)it.data;
			unsigned avg = 0, amount = 0;

			for( unsigned i=start_plane; i<it.size(); i++ ){
				if( it.valid( i ) ){
					avg += it[i];
					amount++;
				}
			}
			
			if( amount )
				it[0] = avg / amount;
			else if( start_plane == 2 )
				it[1] = 0;
		} );
	}
}

static void render_diff( MultiPlaneIterator &it, bool alpha_used ){
	unsigned start_plane = alpha_used ? 2 : 1;
	it.data = (void*)&start_plane;
	
	it.iterate_all(); //No need to optimize this filter
	
	//Do average and store in [0]
	it.for_all_pixels( [](MultiPlaneLineIterator &it){
			unsigned start_plane = *(unsigned*)it.data;
			//Calculate sum
			unsigned sum = 0, amount = 0;
			for( unsigned i=start_plane; i<it.size(); i++ ){
				if( it.valid( i ) ){
					sum += it[i];
					amount++;
				}
			}
			
			if( amount ){
				color_type avg = sum / amount;
				
				//Calculate sum of the difference from average
				unsigned diff_sum = 0;
				for( unsigned i=start_plane; i<it.size(); i++ ){
					if( it.valid( i ) ){
						unsigned d = abs( avg - it[i] );
						diff_sum += d;
					}
				}
				
				//Use an exaggerated gamma to make the difference stand out
				double diff = (double)diff_sum / amount / (255*256);
				it[0] = pow( diff, 0.3 ) * (255*256) + 0.5;
			}
			else if( start_plane == 2 )
				it[1] = 0;
		} );
}

static void render_dark_select( MultiPlaneIterator &it, bool alpha_used ){
	unsigned start_plane = alpha_used ? 2 : 1;
	it.data = (void*)&start_plane;
	
	it.iterate_all(); //No need to optimize this filter
	
	//Do average and store in [0]
	it.for_all_pixels( [](MultiPlaneLineIterator &it){
			unsigned start_plane = *(unsigned*)it.data;
			//Calculate sum
			color_type min = (256*256-1);
			for( unsigned i=start_plane; i<it.size(); i++ ){
				if( it.valid( i ) ){
					if( min > it[i] )
						min = it[i];
				}
			}
			
			it[0] = min;
		} );
}

ImageEx* SimpleRender::render( const AImageAligner& aligner, unsigned max_count ) const{
	QTime t;
	t.start();
	#undef DIFFERENCE
	if( max_count > aligner.count() )
		max_count = aligner.count();
	
	//Abort if no images
	if( max_count == 0 ){
		qWarning( "No images to render!" );
		return nullptr;
	}
	qDebug( "render_image: image count: %d", (int)max_count );
	
	//TODO: determine amount of planes!
	unsigned planes_amount = 3; //TODO: alpha?
	if( filter == FOR_MERGING && aligner.image(0)->get_system() == ImageEx::YUV )
		planes_amount = 1;
	if( filter == DIFFERENCE )
		planes_amount = 1; //TODO: take the best plane
	
	//Do iterator
	QRect full = aligner.size();
	ImageEx *img = new ImageEx( (planes_amount==1) ? ImageEx::GRAY : aligner.image(0)->get_system() );
	if( !img )
		return NULL;
	img->create( 1, 1 ); //TODO: set as initialized
	
	//Fill alpha
	Plane* alpha = new Plane( full.width(), full.height() );
	alpha->fill( 255*256 );
	img->replace_plane( 3, alpha );
	
	for( unsigned i=0; i<planes_amount; i++ ){
		//Determine local size
		double scale_x = (double)aligner.plane(0,i)->get_width() / aligner.plane(0,0)->get_width();
		double scale_y = (double)aligner.plane(0,i)->get_height() / aligner.plane(0,0)->get_height();
		
		//TODO: something is wrong with the rounding, chroma-channels are slightly off
		QRect local( 
				(int)round( full.x()*scale_x )
			,	(int)round( full.y()*scale_y )
			,	(int)round( full.width()*scale_x )
			,	(int)round( full.height()*scale_y )
			);
		QRect out_size( upscale_chroma ? full : local );
		
		//Create output plane
		Plane* out = new Plane( out_size.width(), out_size.height() );
		out->fill( 0 );
		img->replace_plane( i, out );
		
		vector<PlaneItInfo> info;
		info.push_back( PlaneItInfo( out, out_size.x(),out_size.y() ) );
		
		bool use_alpha = false;
		if( out_size == full ){
			info.push_back( PlaneItInfo( alpha, out_size.x(),out_size.y() ) );
			use_alpha = true;
			//TODO: we still have issues with the chroma planes as the
			//up-scaled layers doesn't always cover all pixels in the Y plane.
		}
		
		vector<Plane*> temp;
		
		if( out_size == local ){
			for( unsigned j=0; j<max_count; j++ )
				info.push_back( PlaneItInfo(
						aligner.plane( j, i )
					,	round( aligner.pos(j).x()*scale_x )
					,	round( aligner.pos(j).y()*scale_y )
					) );
		}
		else{
			temp.reserve( max_count );
			for( unsigned j=0; j<max_count; j++ ){
				Plane *p = aligner.plane( j, i )->scale_cubic( aligner.plane(j,0)->get_width(), aligner.plane(j,0)->get_height() );
				if( !p )
					qDebug( "No plane :\\" );
				temp.push_back( p );
				QPoint pos = aligner.pos(j).toPoint();
				info.push_back( PlaneItInfo( p, pos.x(),pos.y() ) );
			}
		}
		
		MultiPlaneIterator it( info );
		
		if( filter == DIFFERENCE )
			render_diff( it, use_alpha );
		else if( filter == DARK_SELECT && i == 0 )
			render_dark_select( it, use_alpha );
		else
			render_average( it, use_alpha );
		
		//Upscale plane if necessary
		if( full != out_size )
			img->replace_plane( i, out->scale_cubic( full.width(), full.height() ) );
		
		//Remove scaled planes
		for( unsigned j=0; j<temp.size(); j++ )
			delete temp[j];
	}
	
	/* 
	color (MultiImageIterator::*function)();
	function = &MultiImageIterator::average;
	*row = (it.*function)();
	*/
	qDebug( "render rest took: %d", t.elapsed() );
	
	return img;
}


