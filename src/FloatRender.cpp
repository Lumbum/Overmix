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


#include "FloatRender.hpp"
#include "AImageAligner.hpp"
#include "Plane.hpp"
#include "ImageEx.hpp"
#include "color.hpp"

#include <QTime>
#include <float.h>
#include <algorithm>
#include <vector>
using namespace std;

static double cubic( double b, double c, double x ){
	x = abs( x );
	
	if( x < 1 )
		return
				(12 - 9*b - 6*c)/6 * x*x*x
			+	(-18 + 12*b + 6*c)/6 * x*x
			+	(6 - 2*b)/6
			;
	else if( x < 2 )
		return
				(-b - 6*c)/6 * x*x*x
			+	(6*b + 30*c)/6 * x*x
			+	(-12*b - 48*c)/6 * x
			+	(8*b + 24*c)/6
			;
	else
		return 0;
}
static double spline( double x ){ return cubic( 1.0, 0.0, x ); }
//TODO: reuse implementation in Plane

class PointRenderBase{
	public:
		struct Point{
			double distance;
			color_type value;
			bool operator<(const Point& other) const{
				return distance < other.distance;
			}
		};
		
	private:
		QPoint pos;

		QPointF toAbsolute( QPointF img_pos, QPointF offset, double scale ) const{
			return img_pos * scale + offset;
		}
		QPointF toRelative( QPointF pos, QPointF offset, double scale ) const{
			return (pos - offset) / scale;
		}
		
	public:
		PointRenderBase( int x, int y ) : pos( x, y ) { }
		
		void add_points( const Plane* img, QPointF offset, double scale ){
			QRectF relative( offset, QSizeF( img->get_width()*scale, img->get_height()*scale ) );
			QRectF window( pos - QPointF( 2,2 )*scale, QSizeF( 4,4 )*scale );
			QRectF usable = window.intersected(relative);
			
			QPointF fstart = toRelative( usable.topLeft(), offset, scale );
			QPointF fend = toRelative( usable.bottomRight(), offset, scale );
			for( int iy=ceil(fstart.y()); iy<floor(fend.y()); ++iy )
				for( int ix=ceil(fstart.x()); ix<floor(fend.x()); ++ix ){
					QPointF distance = toAbsolute( QPointF( ix, iy ), offset, scale ) - pos;
					distance /= scale;
					Point p{ sqrt(distance.x()*distance.x() + distance.y()*distance.y()), img->pixel(ix,iy) };
					add_point( p );
				}
		}
		
		virtual void add_point( Point p ) = 0;
};

class PointRender : public PointRenderBase{
	protected:
		vector<Point>& points;
		
	public:
		PointRender( int x, int y, vector<Point>& points ) : PointRenderBase( x, y ), points(points) {
			points.clear();
		}
		
		color_type value(){
			sort( points.begin(), points.end() );
			double sum = 0.0;
			double weight = 0.0;
			for( unsigned i=0; i<min(points.size(),(vector<Point>::size_type)16); ++i ){
				double w = spline( points[i].distance );
				sum += points[i].value * w;
				weight += w;
			}
			return (weight!=0.0) ? round(sum / weight) : 0;
		}
		
		virtual void add_point( Point p ) override{
			points.push_back( p );
		}
};

class PointRender2 : public PointRenderBase{
	protected:
		double sum = 0.0;
		double weight = 0.0;
		
	public:
		PointRender2( int x, int y ) : PointRenderBase( x, y ), sum(0), weight(0) { }
		
		color_type value(){
			return (weight!=0.0) ? color::truncate(sum / weight) : color::BLACK;
		}
		
		virtual void add_point( Point p ) override{
			double w = spline( p.distance );
			sum += p.value * w;
			weight += w;
		}
};

class PointRender3 : public PointRenderBase{
	protected:
		Point p{ 99999, 0 };
		
	public:
		PointRender3( int x, int y ) : PointRenderBase( x, y ) { }
		
		color_type value(){
			return p.value;
		}
		
		virtual void add_point( Point p ) override{
			this->p = min( this->p, p );
		}
		
};


ImageEx* FloatRender::render( const AImageAligner& aligner, unsigned max_count ) const{
	QTime t;
	t.start();
	
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
	
	//Do iterator
	QRect full = aligner.size();
	double scale = 2.0;
	ImageEx *img = new ImageEx( (planes_amount==1) ? ImageEx::GRAY : aligner.image(0)->get_system() );
	if( !img )
		return NULL;
	img->create( full.width()*scale, full.height()*scale );
	
	//Fill alpha
	Plane* alpha = new Plane( full.width()*scale, full.height()*scale );
	alpha->fill( color::WHITE );
	img->replace_plane( 3, alpha );
	
	vector<PointRenderBase::Point> points;
	for( unsigned i=0; i<planes_amount; i++ ){
		Plane* out = (*img)[i];
		
		for( unsigned iy=0; iy<out->get_height(); ++iy ){
			color_type* row = out->scan_line( iy );
			for( unsigned ix=0; ix<out->get_width(); ++ix ){
				PointRender p( ix + full.x()*scale, iy + full.y()*scale, points );
				
				for( unsigned j=0; j<max_count; ++j ){
					double scale2 = (double)aligner.plane( j, 0 )->get_width() / aligner.plane( j, i )->get_width() * scale;
					p.add_points( aligner.plane( j, i ), aligner.pos( j )*scale, scale2 );
				}
				
				row[ix] = p.value();
			}
		}
	}
	
	qDebug( "float render rest took: %d", t.elapsed() );
	
	return img;
}



