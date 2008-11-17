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

#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../base/is_base.hpp"
#include "sorters/cbt.hpp"
#include "../dynamics/interactions/intEvent.hpp"
#include "../dynamics/globals/globEvent.hpp"
#include <vector>

class CParticle;

class CScheduler: public DYNAMO::SimBase_const
{
public:
  CScheduler(const DYNAMO::SimData* const, const char *, CSSorter*);
  
  virtual ~CScheduler() = 0;

  virtual void initialise() = 0;
  
  inline void fullUpdate(const CParticle& part)
  {
    invalidateEvents(part);
    addEvents(part);
    sort(part);
  }

  inline void fullUpdate(const CParticle& p1, const CParticle& p2)
  {
    //Both must be invalidated at once to reduce the number of invalid
    //events in the queue
    invalidateEvents(p1);
    invalidateEvents(p2);
    addEvents(p1);
    addEvents(p2);
    sort(p1);
    sort(p2);
  }

  void invalidateEvents(const CParticle&);

  virtual void addEvents(const CParticle&) = 0;

  void sort(const CParticle&);

  void popNextEvent();

  void pushEvent(const CParticle&, const intPart&);
  
  void stream(const Iflt& dt) {  sorter->stream(dt); }
  
  const CIntEvent earliestIntEvent() const;

  const CGlobEvent earliestGlobEvent() const;

  const CLocalEvent earliestLocalEvent() const;

  EEventType nextEventType() const;

  virtual void rebuildList() = 0;

  friend xmlw::XmlStream& operator<<(xmlw::XmlStream&, const CScheduler&);

  static CScheduler* getClass(const XMLNode&, const DYNAMO::SimData*);

  virtual void operator<<(const XMLNode&) = 0;
  
  void rescaleTimes(const Iflt& scale) { sorter->rescaleTimes(scale); }

  const smrtPlugPtr<CSSorter>& getSorter() const { return sorter; }
  
protected:
  mutable smrtPlugPtr<CSSorter> sorter;
  mutable std::vector<unsigned long long> eventCount;
  
  virtual void outputXML(xmlw::XmlStream&) const = 0;
};

#endif
