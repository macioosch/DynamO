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

#ifndef OPDens_H
#define OPDens_H

#include "outputplugin.hpp"
#include <map>

class CComplex;

class OPDens: public OutputPlugin
{
 public:
  OPDens(DYNAMO::SimData*);
  ~OPDens();

  void collisionUpdate(const CIntEvent &, const CIntEventData &b);

  void output(xmlw::XmlStream &); 

  void periodicOutput();

  virtual OutputPlugin *Clone() const { return new OPDens(*this); };
  
 protected:

  std::map<long,std::map<long,CComplex> > xybin,densdens;

  long counter,collcount;
};

#endif
