/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2008  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#include "dumbsched.hpp"
#include "../dynamics/interactions/intEvent.hpp"
#include "../simulation/particle.hpp"
#include "../dynamics/dynamics.hpp"
#include "../dynamics/liouvillean/liouvillean.hpp"
#include "../dynamics/BC/BC.hpp"
#include "../dynamics/BC/LEBC.hpp"
#include "../base/is_simdata.hpp"
#include "../base/is_base.hpp"
#include "../dynamics/globals/globEvent.hpp"
#include "../dynamics/systems/system.hpp"
#include <cmath> //for huge val
#include "../extcode/xmlParser.h"
#include "../dynamics/globals/global.hpp"
#include "../dynamics/globals/globEvent.hpp"
#include "../dynamics/globals/neighbourList.hpp"
#include "../dynamics/locals/local.hpp"
#include "../dynamics/locals/localEvent.hpp"

CSDumb::CSDumb(const XMLNode& XML, 
				 const DYNAMO::SimData* Sim):
  CScheduler(Sim,"DumbScheduler", NULL)
{ 
  I_cout() << "Dumb Scheduler Algorithmn";
  operator<<(XML);
}

CSDumb::CSDumb(const DYNAMO::SimData* Sim, CSSorter* ns):
  CScheduler(Sim,"DumbScheduler", ns)
{ I_cout() << "Dumb Scheduler Algorithmn"; }

void 
CSDumb::operator<<(const XMLNode& XML)
{
  sorter.set_ptr(CSSorter::getClass(XML.getChildNode("Sorter"), Sim));
}

void
CSDumb::initialise()
{
  I_cout() << "Reinitialising on collision " << Sim->lNColl;
  
  sorter->clear();
  sorter->resize(Sim->lN);
  eventCount.clear();
  eventCount.resize(Sim->lN, 0);
  
  //Now initialise the interactions
  BOOST_FOREACH(const CParticle& part, Sim->vParticleList)
    addEvents(part);
  
  sorter->init();
}

void 
CSDumb::outputXML(xmlw::XmlStream& XML) const
{
  XML << xmlw::attr("Type") << "Dumb"
      << xmlw::tag("Sorter")
      << sorter
      << xmlw::endtag("Sorter");
}

void 
CSDumb::addEvents(const CParticle& part)
{  
  //Add the global events
  BOOST_FOREACH(const smrtPlugPtr<CGlobal>& glob, Sim->Dynamics.getGlobals())
    if (glob->isInteraction(part))
      sorter->push(glob->getEvent(part), part.getID());
  
  //Add the local cell events
  BOOST_FOREACH(const smrtPlugPtr<CLocal>& local, Sim->Dynamics.getLocals())
    if (local->isInteraction(part))
      sorter->push(local->getEvent(part), part.getID());

  //Add the interaction events
  BOOST_FOREACH(const CParticle& part2, Sim->vParticleList)
    if (part2 != part)
      {
	CIntEvent eevent(Sim->Dynamics.getEvent(part, part2));
	
	if (eevent.getType() != NONE)
	  sorter->push(intPart(eevent, eventCount[part2.getID()]), 
		       part.getID());
      }
}
