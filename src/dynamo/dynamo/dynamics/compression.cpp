/*  dynamo:- Event driven molecular dynamics simulator 
    http://www.dynamomd.org
    Copyright (C) 2011  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

    This program is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    version 3 as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <dynamo/dynamics/compression.hpp>
#include <dynamo/interactions/intEvent.hpp>
#include <dynamo/2particleEventData.hpp>
#include <dynamo/BC/BC.hpp>
#include <dynamo/simulation.hpp>
#include <dynamo/species/species.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <magnet/intersection/ray_sphere.hpp>
#include <magnet/intersection/ray_plane.hpp>
#include <magnet/intersection/offcentre_spheres.hpp>
#include <magnet/xmlwriter.hpp>

namespace dynamo {
  DynCompression::DynCompression(dynamo::Simulation* tmp, double GR):
    DynNewtonian(tmp), growthRate(GR) {}

  double 
  DynCompression::SphereSphereInRoot(const Particle& p1, const Particle& p2, double d) const
  {
    Vector r12 = p1.getPosition() - p2.getPosition();
    Vector v12 = p1.getVelocity() - p2.getVelocity();
    Sim->BCs->applyBC(r12, v12);
    return magnet::intersection::ray_growing_sphere<>(r12, v12, d, growthRate, Sim->systemTime);
  }

  double
  DynCompression::SphereSphereOutRoot(const Particle& p1, const Particle& p2, double d) const
  {
    Vector r12 = p1.getPosition() - p2.getPosition();
    Vector v12 = p1.getVelocity() - p2.getVelocity();
    Sim->BCs->applyBC(r12, v12);
    return magnet::intersection::ray_growing_sphere<true>(r12, v12, d, growthRate, Sim->systemTime);
  }

  std::pair<bool, double> 
  DynCompression::getOffcentreSpheresCollision(const double offset1, const double diameter1, const double offset2, const double diameter2, const Particle& p1, const Particle& p2, double t_max, double maxdist) const
  {  
#ifdef DYNAMO_DEBUG
    if (!hasOrientationData())
      M_throw() << "Cannot use this function without orientational data";

    if (!isUpToDate(p1))
      M_throw() << "Particle1 " << p1.getID() << " is not up to date";

    if (!isUpToDate(p2))
      M_throw() << "Particle2 " << p2.getID() << " is not up to date";
#endif

    Vector r12 = p1.getPosition() - p2.getPosition();
    Vector v12 = p1.getVelocity() - p2.getVelocity();
    Sim->BCs->applyBC(r12, v12);
    
    const double limit_time_window = 1 / growthRate;

    std::pair<bool, double> retval = magnet::intersection::offcentre_growing_spheres(r12, v12, orientationData[p1.getID()].angularVelocity, orientationData[p2.getID()].angularVelocity, orientationData[p1.getID()].orientation * Quaternion::initialDirector() * offset1, orientationData[p2.getID()].orientation * Quaternion::initialDirector() * offset2, diameter1, diameter2, maxdist, std::min(limit_time_window, t_max), Sim->systemTime, growthRate);
    
    //Check if there's no collision reported but we've limited the interval
    if ((retval.second == HUGE_VAL) && (t_max > limit_time_window))
      return std::pair<bool, double>(false, limit_time_window);
    
    //Otherwise return what was calculated
    return retval;
  }
  
  double 
  DynCompression::getPlaneEvent(const Particle& part, const Vector & origin, const Vector & norm, double diameter) const
  {
    Vector rij = part.getPosition() - origin, vij = part.getVelocity() - norm * diameter * growthRate;
    Sim->BCs->applyBC(rij, vij);
    return magnet::intersection::ray_plane(rij, vij, norm, diameter * (1.0 + growthRate * Sim->systemTime));
  }

  ParticleEventData 
  DynCompression::runPlaneEvent(Particle& part, const Vector& vNorm, const double e, const double diameter) const
  {
    updateParticle(part);
    ParticleEventData retVal(part, *Sim->species[part], WALL);
    Vector vij = part.getVelocity() - vNorm * diameter * growthRate;
    part.getVelocity() -= (1+e) * (vNorm | vij) * vNorm;
    return retVal; 
  }

  double
  DynCompression::sphereOverlap(const Particle& p1, const Particle& p2,
			      const double& d) const
  {
    Vector r12 = p1.getPosition() - p2.getPosition();
    Sim->BCs->applyBC(r12);
    double currd2 = std::pow(d * (1 + Sim->systemTime * growthRate), 2);
    return std::sqrt(std::max(currd2 - (r12 | r12), 0.0));
  }


  PairEventData 
  DynCompression::SmoothSpheresColl(const IntEvent& event, const double& e, const double& d2, const EEventType& eType) const
  {
    Particle& particle1 = Sim->particles[event.getParticle1ID()];
    Particle& particle2 = Sim->particles[event.getParticle2ID()];
    updateParticlePair(particle1, particle2);  
    PairEventData retVal(particle1, particle2, *Sim->species[particle1], *Sim->species[particle2], eType);
    Sim->BCs->applyBC(retVal.rij, retVal.vijold);
    double p1Mass = Sim->species[retVal.particle1_.getSpeciesID()]->getMass(particle1.getID()); 
    double p2Mass = Sim->species[retVal.particle2_.getSpeciesID()]->getMass(particle2.getID()); 
    double r2 = retVal.rij.nrm2();
    retVal.rvdot = (retVal.rij | retVal.vijold);
    double mu = 1.0 / ((1.0 / p1Mass) + (1.0 / p2Mass));
    bool infinite_masses = (p1Mass == HUGE_VAL) && (p2Mass == HUGE_VAL);
    if (infinite_masses)
      {
	p1Mass = p2Mass = 1;
	mu = 0.5;
      }

    retVal.impulse = retVal.rij * ((1.0 + e) * mu * (retVal.rvdot - growthRate * sqrt(d2 * r2)) / retVal.rij.nrm2());  
    particle1.getVelocity() -= retVal.impulse / p1Mass;
    particle2.getVelocity() += retVal.impulse / p2Mass;
    //If both particles have infinite mass we pretend no momentum was transferred
    retVal.impulse *= !infinite_masses;

    return retVal;
  }

  PairEventData 
  DynCompression::SphereWellEvent(const IntEvent& event, const double& deltaKE, const double& d2, size_t) const
  {
    Particle& particle1 = Sim->particles[event.getParticle1ID()];
    Particle& particle2 = Sim->particles[event.getParticle2ID()];
    updateParticlePair(particle1, particle2);  
    PairEventData retVal(particle1, particle2, *Sim->species[particle1], *Sim->species[particle2], event.getType());
    Sim->BCs->applyBC(retVal.rij, retVal.vijold);
    double p1Mass = Sim->species[retVal.particle1_.getSpeciesID()]->getMass(particle1.getID());
    double p2Mass = Sim->species[retVal.particle2_.getSpeciesID()]->getMass(particle2.getID());
    double mu = 1.0 / ((1.0 / p1Mass) + (1.0 / p2Mass));
    bool infinite_masses = (p1Mass == HUGE_VAL) && (p2Mass == HUGE_VAL);
    if (infinite_masses)
      {
	p1Mass = p2Mass = 1;
	mu = 0.5;
      }

    Vector  urij = retVal.rij / retVal.rij.nrm();
    retVal.rvdot = (urij | retVal.vijold);
    double sqrtArg = std::pow(retVal.rvdot - growthRate * sqrt(d2), 2)  + (2.0 * deltaKE / mu);
    if ((deltaKE < 0) && (sqrtArg < 0))
      {
	event.setType(BOUNCE);
	retVal.setType(BOUNCE);

	retVal.impulse = urij * (2.0 * mu * (retVal.rvdot - growthRate * sqrt(d2)));
      }
    else if (deltaKE==0)
      retVal.impulse = Vector(0,0,0);
    else
      {	  
	retVal.particle1_.setDeltaU(-0.5 * deltaKE);
	retVal.particle2_.setDeltaU(-0.5 * deltaKE);	  
      
	if (retVal.rvdot < 0)
	  retVal.impulse = urij 
	    * (2.0 * deltaKE / (growthRate * sqrt(d2) + std::sqrt(sqrtArg) - retVal.rvdot ));
	else
	  retVal.impulse = urij 
	    * (2.0 * deltaKE / (growthRate * sqrt(d2) - std::sqrt(sqrtArg) - retVal.rvdot ));
	;
      }

    retVal.rvdot *= retVal.rij.nrm();
  
#ifdef DYNAMO_DEBUG
    if (boost::math::isnan(retVal.impulse[0]))
      M_throw() << "A nan dp has ocurred"
		<< "\ndeltaKE = " << deltaKE
		<< "\ngrowthRate = " << growthRate
		<< "\nd2 = " << d2
		<< "\nsqrtArg = " << sqrtArg
		<< "\nrvdot = " << retVal.rvdot
		<< "\nArg " << (growthRate * sqrt(d2) - std::sqrt(sqrtArg) - retVal.rvdot)
	;
#endif
  
    particle1.getVelocity() -= retVal.impulse / p1Mass;
    particle2.getVelocity() += retVal.impulse / p2Mass;
    retVal.impulse *= !infinite_masses;
  
    return retVal;
  }

  void 
  DynCompression::outputXML(magnet::xml::XmlStream& XML) const
  {
    XML << magnet::xml::attr("Type") 
	<< "Compression";
  }

  double 
  DynCompression::getPBCSentinelTime(const Particle& part,
				   const double& lMax) const
  {
#ifdef DYNAMO_DEBUG
    if (!isUpToDate(part))
      M_throw() << "Particle is not up to date";
#endif

    Vector  pos(part.getPosition()), vel(part.getVelocity());

    Sim->BCs->applyBC(pos, vel);

    double retval = (0.5 * Sim->primaryCellSize[0] - lMax) / (fabs(vel[0]) + lMax * growthRate);

    for (size_t i(1); i < NDIM; ++i)
      {
	double tmp = (0.5 * Sim->primaryCellSize[i] - lMax) / (fabs(vel[0]) + lMax * growthRate);
      
	if (tmp < retval)
	  retval = tmp;
      }

    return retval;
  }

  PairEventData 
  DynCompression::parallelCubeColl(const IntEvent&,
				 const double&, const double&,
				 const EEventType&) const
  { M_throw() << "Not Implemented"; }


  ParticleEventData 
  DynCompression::runAndersenWallCollision(Particle& part, const Vector & vNorm, const double& sqrtT, const double d) const
  {  
    updateParticle(part);

    if (hasOrientationData())
      M_throw() << "Need to implement thermostating of the rotational degrees"
	" of freedom";

    //This gives a completely new random unit vector with a properly
    //distributed Normal component. See Granular Simulation Book
    ParticleEventData tmpDat(part, *Sim->species[part], WALL);
 
    double mass = Sim->species[tmpDat.getSpeciesID()]->getMass(part.getID());

    std::normal_distribution<> normal_dist;
    std::uniform_real_distribution<> uniform_dist;

    for (size_t iDim = 0; iDim < NDIM; iDim++)
      part.getVelocity()[iDim] = normal_dist(Sim->ranGenerator) * sqrtT / std::sqrt(mass);
  
    part.getVelocity()
      //This first line adds a component in the direction of the normal
      += vNorm * (sqrtT * sqrt(-2.0*log(1.0 - uniform_dist(Sim->ranGenerator)) / mass)
		  //This removes the original normal component
		  -(part.getVelocity() | vNorm)
		  //This adds on the velocity of the wall
		  + d * growthRate)
      ;

    return tmpDat; 
  }
}
