/*  DYNAMO:- Event driven molecular dynamics simulator 
    http://www.marcusbannerman.co.uk/dynamo
    Copyright (C) 2010  Marcus N Campbell Bannerman <m.bannerman@gmail.com>

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

#pragma once

#include "system.hpp"

class CSSchedMaintainer: public System
{
public:
  CSSchedMaintainer(DYNAMO::SimData*, double, std::string);
  
  virtual System* Clone() const { return new CSSchedMaintainer(*this); }

  virtual void runEvent() const;

  virtual void initialise(size_t);

  virtual void operator<<(const XMLNode&) {}

  void setdt(double);

  void increasedt(double);

protected:
  virtual void outputXML(xml::XmlStream&) const {}

  double periodt;
};
