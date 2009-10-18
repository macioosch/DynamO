/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2009  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#include "uenergy.hpp"
#include <boost/foreach.hpp>
#include <cmath>
#include "../../dynamics/interactions/captures.hpp"
#include "../../dynamics/include.hpp"
#include "../../dynamics/interactions/intEvent.hpp"
#include "../../base/is_simdata.hpp"

OPUEnergy::OPUEnergy(const DYNAMO::SimData* tmp, const XMLNode&):
  OP1PP(tmp,"UEnergy", 250),
  intECurrent(0.0),
  intEsqAcc(0.0),
  intEAcc(0.0)
{}

void 
OPUEnergy::changeSystem(OutputPlugin* Eplug)
{
  std::swap(Sim, static_cast<OPUEnergy*>(Eplug)->Sim);
  std::swap(intECurrent, static_cast<OPUEnergy*>(Eplug)->intECurrent);
}

void
OPUEnergy::initialise()
{
  intECurrent = Sim->dynamics.calcInternalEnergy();
}

Iflt 
OPUEnergy::getAvgSqU() const
{ 
  return intEsqAcc / ( Sim->dSysTime 
		       * pow(Sim->dynamics.units().unitEnergy(), 2)); 
}

Iflt 
OPUEnergy::getAvgU() const
{ 
  return intEAcc / ( Sim->dSysTime * Sim->dynamics.units().unitEnergy()); 
}

void 
OPUEnergy::A1ParticleChange(const C1ParticleData& PDat)
{
  intECurrent += PDat.getDeltaU();
}

void 
OPUEnergy::A2ParticleChange(const C2ParticleData& PDat)
{
  intECurrent += PDat.particle1_.getDeltaU()
    + PDat.particle2_.getDeltaU();
}

void 
OPUEnergy::stream(const Iflt& dt)
{
  intEAcc += intECurrent * dt;
  intEsqAcc += intECurrent * intECurrent * dt;
}

void
OPUEnergy::output(xmlw::XmlStream &XML)
{
  XML << xmlw::tag("CEnergy")
      << xmlw::tag("InternalEnergy")
      << xmlw::attr("Avg") << getAvgU()
      << xmlw::attr("SquareAvg") << getAvgSqU()
      << xmlw::attr("Current") << intECurrent
    / Sim->dynamics.units().unitEnergy()
      << xmlw::endtag("InternalEnergy")
      << xmlw::endtag("CEnergy");
}

void
OPUEnergy::periodicOutput()
{
  I_Pcout() << "U " << intECurrent / Sim->dynamics.units().unitEnergy() << ", ";
}
