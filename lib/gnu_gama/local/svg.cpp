/* GNU Gama -- adjustment of geodetic networks
   Copyright (C) 2012  Ales Cepek <cepek@gnu.org>

   This file is part of the GNU Gama C++ library.

   This library is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */


#include <gnu_gama/local/svg.h>
#include <gnu_gama/radian.h>
#include <algorithm>
#include <sstream>
#include <utility>
#include <set>


namespace
{
/**/  class Point {
/**/  public:
/**/    double x, y;
/**/
/**/    Point() {}
/**/    Point(double a, double b) : x(a), y(b) {}
/**/
/**/  };
/**/
/**/  std::ostream& operator<< (std::ostream& ostr, const Point& p)
/**/  {
/**/    return ostr << p.x << "," << p.y << " ";
/**/  }
/**/
/**/  Point operator+(const Point& a, const Point& b)
/**/  {
/**/    return Point(a.x+b.x, a.y+b.y);
/**/  }
/**/
/**/  Point operator-(const Point& a, const Point& b)
/**/  {
/**/    return Point(a.x-b.x, a.y-b.y);
/**/  }
/**/
/**/  Point operator*(const Point& a, double q)
/**/  {
/**/    return Point(a.x*q, a.y*q);
/**/  }
/**/
/**/  Point TX, TY;
/**/  void sett(double tr11, double tr12, double tr21, double tr22)
/**/  {
/**/    TX = Point(tr11, tr12);
/**/    TY = Point(tr21, tr22);
/**/  }
}


using namespace GNU_gama::local;

GamaLocalSVG::GamaLocalSVG(LocalNetwork* is)
  : IS(*is), PD(is->PD), OD(is->OD),
    ysign(GaMaConsistent(PD) ? +1 : -1),
    tst_draw_axes(true), tst_draw_point_symbols(true),
    tst_draw_point_ids(true), tst_draw_ellipses(true),
    tst_draw_observations(true)
{
  svg_init();

  fontsize = fontsize_;
}

void GamaLocalSVG::svg_init() const
{
  /* Original coordinate unit vectors x and y (EN, NW, ...) expressed
     in SVG coordinate system (x right, y down).

     For example in NE coordinate system axis vector x (1, 0) {points
     north} is expressed in SVG as (0, -1) {x points up}; similarly
     canonical coordinate vector y (0, 1) {pointing east} in SVG
     corresponds to (1, 0) {points right}; and parameters of are
     sett(0,-1,1,0) */
  switch (PD.local_coordinate_system)
    {
      // right handed
    case LocalCoordinateSystem::EN: sett( 1, 0, 0,-1); break;
    case LocalCoordinateSystem::NW: sett( 0,-1,-1, 0); break;
    case LocalCoordinateSystem::SE: sett( 0, 1, 1, 0); break;
    case LocalCoordinateSystem::WS: sett(-1, 0, 0, 1); break;
      // left handed
    case LocalCoordinateSystem::NE: sett( 0,-1, 1, 0); break;
    case LocalCoordinateSystem::SW: sett( 0, 1,-1, 0); break;
    case LocalCoordinateSystem::ES: sett( 1, 0, 0, 1); break;
    case LocalCoordinateSystem::WN: sett(-1, 0, 0,-1); break;
    default:
      sett(0,0,0,0);
    }


  // clear global transformation parameters
  minx = maxx = miny = maxy = offset = 0;

  bool first_point = true;
  double x, y, tminx, tmaxx, tminy, tmaxy;
  std::vector<double> abmed;

  for (PointData::const_iterator i=PD.begin(), e=PD.end(); i!=e; ++i)
    {
      PointID    pid   = i->first;
      LocalPoint point = i->second;

      // skip points that are not part of the adjustment or do not have xy
      if (!point.active_xy() || !point.test_xy()) continue;

      svg_xy(point, x, y);

      if (first_point)
        {
          tminx = tmaxx = x;
          tminy = tmaxy = y;
          first_point = false;
        }
      else
        {
          if (x < tminx) tminx = x;
          if (x > tmaxx) tmaxx = x;
          if (y < tminy) tminy = y;
          if (y > tmaxy) tmaxy = y;
        }

      if (tst_draw_ellipses && IS.is_adjusted() && !point.fixed_xy())
        {
          double a, b, alpha;
          IS.std_error_ellipse(pid, a, b, alpha);
          abmed.push_back(a);
          abmed.push_back(b);
        }
    }

  ab_median = 0;
  if (unsigned n = abmed.size())
    {
      std::sort(abmed.begin(), abmed.end());
      if (n % 2) ab_median =  abmed[n/2];
      else       ab_median = (abmed[n/2] + abmed[n/2-1])/2;
    }

  minx = tminx;
  miny = tminy;
  maxx = std::abs(tmaxx-tminx);
  maxy = std::abs(tmaxy-tminy);
  offset = (maxx + maxy)/2*0.05;
  fontsize_ = offset*0.4;
}

void GamaLocalSVG::svg_xy(const LocalPoint& point, double& x, double& y) const
{
  // transformation matrix is inv([TX; TY]) = [TX; TY]'
  x = TX.x*point.x() + TY.x*point.y()*ysign - minx + 2*offset;
  y = TX.y*point.x() + TY.y*point.y()*ysign - miny + 2*offset;
}

std::string GamaLocalSVG::string() const
{
  std::ostringstream stream;
  draw(stream);
  return stream.str();
}

void GamaLocalSVG::draw(std::ostream& output_stream) const
{
  svg = &output_stream;

  svg_init();

  const double wmaxx = 1.0*(maxx + 4*offset);
  const double wmaxy = 1.0*(maxy + 4*offset);

  *svg << "<?xml version='1.0' encoding='utf-8' standalone='no'?>\n";
  *svg <<
    "<!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 1.1//EN'\n"
    "  'http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd'>\n"
    "<svg version='1.1' "
    " width='"  << wmaxx << "'"
    " height='" << wmaxy << "'"
    " xmlns='http://www.w3.org/2000/svg'"
    " xmlns:xlink='http://www.w3.org/1999/xlink' >\n";

#if 0
  *svg << "<rect x='0' y='0' "
       << "width ='" << wmaxx << "' "
       << "height='" << wmaxy << "' "
       << "style='fill:none;stroke:blue;stroke-width:5' />\n";

  *svg << "<rect x='" << 2*offset << "' y='" << 2*offset << "' "
       << "width ='" << maxx << "' " << "height='" << maxy << "' "
       << "style='fill:grey;stroke:black;stroke-width:5;opacity:0.1' />\n";
#endif

  svg_symbols();
  svg_axes_xy();
  svg_observations();
  svg_points();

  *svg << "</svg>\n";
}

void GamaLocalSVG::svg_symbols() const
{
  *svg << "<defs>\n";

  // shape: 0 - triangle, 1 - circle
  svg_point_shape("Fixed",       0, "blue");
  svg_point_shape("Constrained", 1, "green");
  svg_point_shape("Free",        1, "yellow");

  *svg << "</defs>\n";
}

void GamaLocalSVG::svg_point_shape (std::string type, // Fixed Constrained Free
                                    int shape,        // 0-triangle, 1-circle
                                    std::string fillColor) const
{
  *svg << "<symbol id='symbol" << type << "Point' overflow='visible'>\n";
  if (shape == 0)
    {
      *svg << "<polyline points='"
           << Point(-0.5, 0.28868) * (1.2*fontsize)
           << Point( 0.5, 0.28868) * (1.2*fontsize)
           << Point( 0.0,-0.57735) * (1.2*fontsize)
           << Point(-0.5, 0.28868) * (1.2*fontsize) << "'";
    }
  else
    {
      *svg <<  "<circle cx='" << 0 << "' cy='" << 0 << "' "
           << "r='" << 0.5*fontsize << "' ";
    }
  *svg << " style='" << "stroke:black;fill:" << fillColor << "'/>\n"
       << "</symbol>\n";
}

void GamaLocalSVG::svg_axes_xy() const
{
  if (!tst_draw_axes) return;

  Point  P, X, Y, CX, X1, X2, CY, Y1, Y2;
  std::string alignx, aligny;
  const double arrowlength = 4*offset;
  const double caption = 0.4*offset;
  const double arrowheadlong  = 1.5*0.3*offset;
  const double arrowheadshort = 0.3*0.3*offset;

  const double left   = offset;
  const double right  = maxx + 3*offset;
  const double top    = offset;
  const double bottom = maxy + 3*offset;

  switch (PD.local_coordinate_system)
    {
      // right handed
    case LocalCoordinateSystem::EN: P = Point(left,  bottom);  break;
    case LocalCoordinateSystem::NW: P = Point(right, bottom);  break;
    case LocalCoordinateSystem::SE: P = Point(left,  top);     break;
    case LocalCoordinateSystem::WS: P = Point(right, top);     break;

      // left handed
    case LocalCoordinateSystem::NE: P = Point(left,  bottom);  break;
    case LocalCoordinateSystem::SW: P = Point(right, top);     break;
    case LocalCoordinateSystem::ES: P = Point(left,  top);     break;
    case LocalCoordinateSystem::WN: P = Point(right, bottom);  break;

    default:
      return;
    }

  X  = P + TX*arrowlength;
  Y  = P + TY*arrowlength;
  CX = X + TX*caption;
  CY = Y + TY*caption;
  X1 = X - TX*arrowheadlong + TY*arrowheadshort;
  X2 = X - TX*arrowheadlong - TY*arrowheadshort;
  Y1 = Y - TY*arrowheadlong + TX*arrowheadshort;
  Y2 = Y - TY*arrowheadlong - TX*arrowheadshort;
  if (TX.x)
    {
      alignx = "dominant-baseline: central;";
      aligny = "text-anchor: middle;";
    }
  else
    {
      aligny = "dominant-baseline: central;";
      alignx = "text-anchor: middle;";
    }

  *svg << "<g style='stroke:black;stroke-width:1;'>\n";

  // axes
  *svg << "<polyline points='"
       << X << P << Y
       << "' style='fill:none;' />\n";

  // arrows
  *svg << "<polyline points='"
       << X1 << X << X2
       << "' style='fill:none;' />\n";
  *svg << "<polyline points='"
       << Y1 << Y << Y2
       << "' style='fill:none;' />\n";

  *svg << "</g>\n";

  // captions
  *svg << "<text font-family='sans-serif' "
       << "font-size='" << fontsize <<  "' "
       << "x='" << CX.x << "' y='" << CX.y << "' "
       << "style='" << alignx << "'>X</text>\n";
  *svg << "<text font-family='sans-serif' "
       << "x='" << CY.x << "' y='" << CY.y << "' "
       << "font-size='" << fontsize <<  "' "
       << "style='" << aligny << "'> Y</text>\n";
}

void GamaLocalSVG::svg_points() const
{
  for (PointData::const_iterator i=PD.begin(), e=PD.end(); i!=e; ++i)
    {
      PointID    pid   = i->first;
      LocalPoint point = i->second;

      // skip points that are not part of the adjustment or do not have xy
      if (!point.active_xy() || !point.test_xy()) continue;

      svg_draw_point(pid, point);
   }
}

void GamaLocalSVG::svg_observations() const
{
  if (!tst_draw_observations) return;

  typedef std::pair<PointID, PointID> PairID;
  typedef std::set<std::pair<PointID, PointID> > Pairs;

  Pairs pairs;

  typedef GNU_gama::List<GNU_gama::Cluster<Observation>*> ClusterList;
  const ClusterList& clusters = OD.clusters;
  for (ClusterList::const_iterator
         c=clusters.begin(), ce=clusters.end(); c!=ce; ++c)
    {
      for (ObservationList::const_iterator
             i=(*c)->observation_list.begin(),
             e=(*c)->observation_list.end(); i!=e; ++i)
        {
          if (const Observation* b = dynamic_cast<const Observation*>(*i))
            {
              if (b->active())
                {
                  PointID from = b->from();
                  PointID to   = b->to();
                  if (from < to) pairs.insert(PairID(from, to));
                  else           pairs.insert(PairID(to, from));
                }

              if (const Angle* b = dynamic_cast<const Angle*>(*i))
                {
                  PointID from = b->from();
                  PointID to   = b->fs();
                  if (from < to) pairs.insert(PairID(from, to));
                  else           pairs.insert(PairID(to, from));
                }
            }
        }
    }

  for (Pairs::const_iterator i=pairs.begin(), e=pairs.end(); i!=e; ++i)
    {
      // operator[] cannot be used for a const map
      PointData::const_iterator a = PD.find(i->first);
      PointData::const_iterator b = PD.find(i->second);
      if (a == PD.end()) continue;
      if (b == PD.end()) continue;

      const LocalPoint& A = a->second;
      const LocalPoint& B = b->second;
      if (!A.test_xy() || !B.test_xy()) continue;

      double x1, y1, x2, y2;
      svg_xy(A, x1, y1);
      svg_xy(B, x2, y2);

      *svg << "<line x1='"<<x1<<"' y1='"<<y1<<"' x2='"<<x2<<"' y2='"<<y2<<"' "
           << "style='stroke:black' />\n";
    }
}

void GamaLocalSVG::svg_draw_point(const PointID& pid,
                                  const LocalPoint& point) const
{
      const double psize = 0.4*fontsize;

      double x, y;
      svg_xy(point, x, y);

      if (tst_draw_point_symbols)
        {
          *svg << "<use id='" << pid << "' x='" << x << "' y='" << y << "'";
          if (point.fixed_xy())
            *svg << " xlink:href='#symbolFixedPoint' />";
          else if (point.constrained_xy())
            *svg << " xlink:href='#symbolConstrainedPoint' />";
          else
            *svg << " xlink:href='#symbolFreePoint' />";
        }

      if (tst_draw_point_ids)
        {
          *svg << "<text font-family='sans-serif' "
               << "transform='translate(" << 2*psize << " " << 2*psize <<")' "
               << "font-size='" << fontsize <<  "' "
               << "x='" << x << "' y='" << y << "' "
               << ">" << pid.str() << "</text>\n";
        }

      if (tst_draw_ellipses && IS.is_adjusted() && !point.fixed_xy())
        {
          double a, b, alpha;
          IS.std_error_ellipse(pid, a, b, alpha);

          if (PD.right_handed_angles()) alpha = -alpha;

          double q = ab_median;
          if (ab_median <= 0) q = 1;   // handle unrealistic data

          a *= offset/q;
          b *= offset/q;

          switch (PD.local_coordinate_system)
            {
              // right handed
            case LocalCoordinateSystem::EN: ; break;
            case LocalCoordinateSystem::NW: alpha += M_PI/2; break;
            case LocalCoordinateSystem::SE: alpha += M_PI/2; break;
            case LocalCoordinateSystem::WS: ; break;
              // left handed
            case LocalCoordinateSystem::NE: alpha += M_PI/2; break;
            case LocalCoordinateSystem::SW: alpha += M_PI/2; break;
            case LocalCoordinateSystem::ES: break;
            case LocalCoordinateSystem::WN: break;
            default:
              ;
            }

           while (alpha < 0)      alpha += 2*M_PI;
           while (alpha > 2*M_PI) alpha -= 2*M_PI;

          alpha *= RAD_TO_DEG;   // see gnu_gama/radian.h

          *svg << "<ellipse  " //cx='" << x << "' cy='" << y << "' "
               << "rx='" << a << "' ry='" << b << "' "
               << "transform='translate(" << x << " " << y << ") "
               << "rotate("<< alpha << ")' "
               << "style='stroke:grey;fill:none;' />";
        }
}