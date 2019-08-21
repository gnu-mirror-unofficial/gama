/*
  GNU Gama -- Approximate coordinates version 2
  Copyright (C) 2018  Petra Millarova <petramillarova@gmail.com>
		2019  Ales Cepek <cepek@gnu.org>

  This file is part of the GNU Gama C++ library.

  GNU Gama is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  GNU Gama is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with GNU Gama.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gnu_gama/local/acord/acord.h>
#include <gnu_gama/local/acord/acord2.h>
#include <gnu_gama/local/acord/acordalgorithm.h>
#include <gnu_gama/local/acord/acordazimuth.h>
#include <gnu_gama/local/acord/acordzderived.h>
#include <gnu_gama/local/acord/acordhdiffs.h>
#include <gnu_gama/local/acord/acordpolar.h>
#include <gnu_gama/local/acord/acordtraverse.h>
#include <gnu_gama/local/acord/acordvectors.h>
#include <gnu_gama/local/acord/acordweakchecks.h>
#include <gnu_gama/local/acord/acordstatistics.h>
#include <gnu_gama/local/bearing.h>
#include <gnu_gama/local/orientation.h>
#include <gnu_gama/local/median/g2d_helper.h>
#include <gnu_gama/local/gamadata.h>
#include <cmath>
#include <algorithm>
#include <unordered_set>

using namespace GNU_gama::local;

#ifdef DEBUG_ACORD2

#include <iostream>
#include <iomanip>

#define DBG(text) debug_info(text)

#else
#define DBG
#endif

Acord2::Acord2(PointData& pd, ObservationData& od)
  : PD_(pd), OD_(od)
{
  bool has_azimuths = false;
  slope_observations_ = false;

  for (auto i : OD_.clusters)
    {
      if (StandPoint* sp = dynamic_cast<StandPoint*>(i))
	{
	  SPClusters_.push_back(sp);

	  if (!has_azimuths)
	    {
	      for (const auto observation : sp->observation_list)
		{
		  if (dynamic_cast<const Azimuth*>(observation) != nullptr)
		    {
		      has_azimuths = true;
		      break;
		    }
		}
	    }

	  if (!slope_observations_)
	    {
	      for (const auto observation : sp->observation_list)
		{
		  if (dynamic_cast<const Z_Angle*>(observation) != nullptr)
		    {
		      slope_observations_ = true;
		      break;
		    }
		  /*if (dynamic_cast<const S_Distance*>(observation) != nullptr)
		    {
		      slope_observations_ = true;
		      break;
		    }*/
		}
	    }
	}
      else if (HeightDifferences* hcl = dynamic_cast<HeightDifferences*>(i))
	{
	  HDiffClusters_.push_back(hcl);
	}
      else if (Vectors* vcl = dynamic_cast<Vectors*>(i))
	{
	  VectorsClusters_.push_back(vcl);
	}
    }

  for (PointData::const_iterator i = PD_.begin(); i != PD_.end(); ++i)
    {
      const PointID& c = (*i).first;
      const LocalPoint& p = (*i).second;
      if (p.active_xy() && !p.test_xy())
	{ // this means the a network point xy were not set and are
	  // part of the network
	  // find and add all missing points into one set
	  missing_xy_.insert(c);
	}
      if (p.active_z() && !p.test_z())
	{
	  missing_z_.insert(c);
	}
    }

  for (auto sp : SPClusters_)
    {
      for (auto obs : sp->observation_list)
	{
	  PointID from = obs->from();
	  obs_from_.insert({obs->from(), obs});
	  if (Angle* angle = dynamic_cast<Angle*>(obs))
	    {
	      obs_to_.insert({angle->bs(), obs});
	      obs_to_.insert({angle->fs(), obs});
	    }
	  else
	    {
	      obs_to_.insert({obs->to(), obs});
	    }
	}
    }

  using std::make_shared;

  if (has_azimuths)
    {
      algorithms_.push_back( make_shared<AcordAzimuth> (this) );
    }
  if (!HDiffClusters_.empty())
    {
      algorithms_.push_back( make_shared<AcordHdiffs>  (this) );
    }
  if (slope_observations_)
    {
      algorithms_.push_back( make_shared<AcordZderived>(this) );
    }
  if (!VectorsClusters_.empty())
    {
      algorithms_.push_back( make_shared<AcordVectors>(this) );
    }
  if (!SPClusters_.empty())
    {
      algorithms_.push_back( make_shared<AcordPolar>   (this) );
      algorithms_.push_back( make_shared<AcordTraverse>(this) );
    }
  algorithms_.push_back( make_shared<AcordWeakChecks>(this) );
}


StandPoint* Acord2::find_standpoint(PointID pt)
{
  for (auto sp : SPClusters_)
    {
      if (sp == nullptr) continue;
      if (sp->station == pt) return sp;
    }
  return nullptr;
}


void Acord2::execute()
{
  //DBG_prnt("Acord2::execute xy known/same/missing")

  size_type after {},  before = missing_xy_.size() + missing_z_.size();
  if (before == 0) return;

  do  // while some points need/can be solved
    {
      before = missing_xy_.size() + missing_z_.size();

      DBG("\n---- start ----");
      for (auto a : algorithms_)
	{
	  a->execute();
	  DBG(a->className());
	}

      algorithms_.erase(
	  std::remove_if(
	     algorithms_.begin(),algorithms_.end(),
	     [](std::shared_ptr<AcordAlgorithm> a){ return a->completed ();}
	  ),
	  algorithms_.end()
	);
      DBG("----  end  ----");


      after = missing_xy_.size() + missing_z_.size();

      get_medians();
      candidate_xy_.clear();
      get_medians_z();
      candidate_z_.clear();
      traverses.clear();
    }
  while (after != 0 && after < before);

  //iterate once more over all SPClusters
  // and compute missing orientations
  for (auto cluster : OD_.clusters)
    {
      if (StandPoint* sp = dynamic_cast<StandPoint*>(cluster))
       {
          if (!sp->test_orientation())
            {
              Orientation ori(PD_, sp->observation_list);
              double orientation_shift;
              int n;
              ori.orientation(sp, orientation_shift, n);
              if (n > 0) sp->set_orientation(orientation_shift);
            }
        }
    }

}


// median: calculates median for a vector of doubles

double Acord2::median(std::vector<double> v)
{
  std::sort(v.begin(), v.end());
  double res = 0;
  if (v.size() % 2 == 0) //size is even
    {
      size_type i = v.size() / 2;
      size_type j = i - 1;
      res = (v[i] + v[j]) / 2;
    }
  else //size is odd
    {
      res = v[v.size()/2];
    }
  return res;
}

// dist_or_dir: function takes an observation and nullptr distance and
// direction and decides if observation is/can be converted to
// Distance or Direction type and places it in the appropriate
// variable

std::pair<double, bool> Acord2::get_dir(Observation* o)
{
  if (Azimuth* a = dynamic_cast<Azimuth*>(o))
    {
      return {a->value() + PD_.xNorthAngle(), true};
    }
  /*if (Angle* a = dynamic_cast<Angle*>(o))
    {
    auto d = angle_to_dir(a);
    if (d.second)
    {
    return {d.first, true};
    }
    }*/
  if (Direction* d = dynamic_cast<Direction*>(o))
    {
      return {d->value(), true};
    }
  return {0, false};
}


std::pair<double,bool> Acord2::get_dist(Observation* o)
{
  if (Distance* d = dynamic_cast<Distance*>(o))
    {
      return {d->value(),true};
    }
  if (slope_observations_)
    {
      if (S_Distance* sd = dynamic_cast<S_Distance*>(o))
        {
          //search for zenith angle so we can calculate horizontal distance
          auto i = obs_from_.lower_bound(o->from());
          auto ei = obs_from_.upper_bound(o->from());
          while (i != ei)
            {
              if (Z_Angle* za = dynamic_cast<Z_Angle*>(i->second))
                {
                  if (o->to() == za->to())
                    {
                      double distval = sd->value()*std::fabs(sin(za->value()));
                      return {distval,true};
                    }
                }
              ++i;
            }
        }
    }
  return {0,false};
}

// in_missingXY: returns true if point is in the missing_xy_ set

bool Acord2::in_missingXY(PointID pt)
{
  std::set<PointID>::iterator it_stpt = missing_xy_.find(pt);
  if (it_stpt == missing_xy_.end()) return false;
  else return true;
}

// get_medians: goes through same_points and if there are two or more
// entries for one PointID, decides if it can add the point to PD and
// calculates new coords

bool Acord2::get_medians()
{
  bool res = false;
  // get unique keys from same_points_
  std::set<PointID> keys;
  for (auto i : candidate_xy_) keys.insert(i.first);

  // for (std::vector<Acord2::Point>::iterator j, i =
  // same_points_.begin(), e = same_points_.end(); i != e; )
  for (auto pt : keys)
    {
      auto p = candidate_xy_.equal_range(pt);
      if (p.first == p.second) continue;    // should never happen

      auto i = p.first;
      auto j = p.second;

      // i and j now mark the beginning and end of an interval with
      // the "same" points
      std::vector<double> all_x;
      std::vector<double> all_y;

      double min_x = i->second.x();
      double max_x = i->second.x();
      double min_y = i->second.y();
      double max_y = i->second.y();

      while (i != j)
        {
          double x = i->second.x();
          double y = i->second.y();
          all_x.push_back(x);
          all_y.push_back(y);
          if (min_x > x) min_x = x;
          if (max_x < x) max_x = x;
          if (min_y > y) min_y = y;
          if (max_y < y) max_y = y;
          ++i;
        }

      if ((all_x.size()>1) && in_missingXY(pt))
        {
          //norm check
          double max_dx = std::fabs(max_x - min_x);
          double max_dy = std::fabs(max_y - min_y);
          double max_diff = std::max(max_dy, max_dx);

          double median_max_norm = median_max_norm_;  // coarse heuristic
          if (slope_observations_) median_max_norm *= 2;

          const auto n = all_x.size();
          switch (n)
            {
            case 3 : median_max_norm *= 1.5; break;
            case 4 : median_max_norm *= 2.0; break;
            default: break;
            }

          if (n > 4 || max_diff <= median_max_norm)
            {
              double med_x = median(all_x);
              double med_y = median(all_y);
              // only setting the coords, leaving other info as is
              PD_[pt].set_xy(med_x, med_y);
              missing_xy_.erase(pt);
              new_points_xy_++;
              res = true;
            }
        }
    }

  // same_points cleanup - we can delete already computed points
  for (auto p : keys)
    {
      if (!in_missingXY(p))
        {
          candidate_xy_.erase(p);
        }
    }
  return res;

};

// loops through candidate pairs <PointID, double>, calculate medians and store

void Acord2::get_medians_z()
{
  std::unordered_set<PointID> set;
  for (auto p : candidate_z_) set.insert(p.first);

  for (PointID id : set)
    {
      std::vector<double> values;
      auto range = candidate_z_.equal_range(id);
      for (auto i = range.first; i != range.second; ++i)
        {
          values.push_back(i->second);
        }
      std::sort(values.begin(), values.end());
      auto n = values.size();
      double median = (values[(n-1)/2] + values[n/2])/2;
      PD_[id].set_z(median);
      candidate_z_.erase(id);
    }
}

// takes a traverse and transforms its coordinates

bool Acord2::transform_traverse(Traverse& traverse)
{
  //now we must somehow transform the coords from local
  PointData local_traverse;
  PointIDList unknown_traverse_pts;
  for (auto p : traverse)
    {
      local_traverse[p.id] = p.coords;
      if (in_missingXY(p.id)) unknown_traverse_pts.push_back(p.id);
    }
  if (unknown_traverse_pts.size() == 0) return true;
  SimilarityTr2D transformation(PD_, local_traverse, unknown_traverse_pts);
  transformation.calculation();
  if (transformation.state() < unique_solution)
    {
      // Transformation failed: No unique solution.
    }
  PointData res = transformation.transf_points();
  if (!res.empty())
    {
      for (auto &pt : traverse)
        {
          if (res[pt.id].test_xy ())
            {
              pt.coords = res[pt.id];
            }
          else
            {
              pt.coords = PD_[pt.id];
            }

        }
      return true;
    }
  else
    {
      return false;
    }
}


#ifdef DEBUG_ACORD2
void Acord2::debug_info(const char* text) const
{
  using std::cerr;
  using std::endl;
  using std::setw;
  using Pair = std::pair<PointID, LocalPoint>;

  cerr
    << setw(15) << text << " : "
    << "alg " << setw(2)  << algorithms_.size()
    << "   known/same/missing   xy " << setw(2)
    << count_if(PD_.begin(),PD_.end(),[](Pair p) {
        return p.second.test_xy() && p.second.active_xy();})
    << " / "  << setw(2) << candidate_xy_.size() << " / " << setw(2)
    << count_if(PD_.begin(),PD_.end(),[](Pair p){
        return !p.second.test_xy() && p.second.active_xy();})
    << "    z " << setw(2)
    << count_if(PD_.begin(),PD_.end(),[](Pair p) {
        return p.second.test_z() && p.second.active_z();})
    << " / "  << setw(2) << candidate_z_.size() << " / " << setw(2)
    << count_if(PD_.begin(),PD_.end(),[](Pair p){
        return !p.second.test_z() && p.second.active_z();})
    << std::endl;
}
#endif
