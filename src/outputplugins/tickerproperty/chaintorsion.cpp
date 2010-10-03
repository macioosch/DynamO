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

#include "chaintorsion.hpp"
#include <boost/foreach.hpp>
#include "../../extcode/xmlwriter.hpp"
#include "../../dynamics/include.hpp"
#include "../../dynamics/ranges/1range.hpp"
#include <boost/foreach.hpp>
#include <vector>
#include "../../datatypes/vector.hpp"
#include "../../base/is_simdata.hpp"
#include "../../dynamics/topology/include.hpp"
#include "../../dynamics/liouvillean/liouvillean.hpp"
#include "../../dynamics/BC/None.hpp"

OPCTorsion::OPCTorsion(const DYNAMO::SimData* tmp, const XMLNode&):
  OPTicker(tmp,"Torsion")
{}

void 
OPCTorsion::initialise()
{
  BOOST_FOREACH(const magnet::ClonePtr<Topology>& plugPtr, Sim->dynamics.getTopology())
    if (dynamic_cast<const CTChain*>(plugPtr.get_ptr()) != NULL)
      chains.push_back(CTCdata(dynamic_cast<const CTChain*>(plugPtr.get_ptr()), 
			       0.005, 0.005, 0.01));

  if (!Sim->dynamics.BCTypeTest<BCNone>())
    M_throw() << "Can only use this plugin with Null BC's"
	      << "\nPositions must be unwrapped";
}

void 
OPCTorsion::changeSystem(OutputPlugin* plug)
{
  std::swap(Sim, static_cast<OPCTorsion*>(plug)->Sim);
  
#ifdef DYNAMO_DEBUG
  if (chains.size() != static_cast<OPCTorsion*>(plug)->chains.size())
    M_throw() << "CTorsion chain data size mismatch in replex exchange";
#endif

  std::list<CTCdata>::iterator iPtr1 = chains.begin(), 
    iPtr2 = static_cast<OPCTorsion*>(plug)->chains.begin();

  while(iPtr1 != chains.end())
    {

#ifdef DYNAMO_DEBUG
      if (iPtr1->chainPtr->getName() != iPtr2->chainPtr->getName())
	M_throw() << "Chain name mismatch when swapping chain plugins";
#endif

      std::swap(iPtr1->chainPtr, iPtr2->chainPtr);

      ++iPtr1;
      ++iPtr2;     
    }
}

void 
OPCTorsion::ticker()
{
  BOOST_FOREACH(CTCdata& dat,chains)
    {
      double sysGamma  = 0.0;
      long count = 0;
      BOOST_FOREACH(const magnet::ClonePtr<CRange>& range,  dat.chainPtr->getMolecules())
	{
	  if (range->size() < 3)//Need three for curv and torsion
	    break;

#ifdef DYNAMO_DEBUG
	  if (NDIM != 3)
	    M_throw() << "Not implemented chain curvature in non 3 dimensional systems";
#endif
	  
	  Vector tmp;
	  std::vector<Vector> dr1;
	  std::vector<Vector> dr2;
	  std::vector<Vector> dr3;
	  std::vector<Vector> vec;

	  //Calc first and second derivatives
	  for (CRange::iterator it = range->begin() + 1; it != range->end() - 1; it++)
	    {
	      tmp = 0.5 * (Sim->particleList[*(it+1)].getPosition()
			   - Sim->particleList[*(it-1)].getPosition());

	      dr1.push_back(tmp);
	      
	      tmp = Sim->particleList[*(it+1)].getPosition() 
		- (2.0 * Sim->particleList[*it].getPosition())
		+ Sim->particleList[*(it-1)].getPosition();
	      
	      dr2.push_back(tmp);
	      
	      vec.push_back(dr1.back() ^ dr2.back());
	    }
	  
	  //Create third derivative
	  for (std::vector<Vector  >::iterator it2 = dr2.begin() + 1; it2 != dr2.end() - 1; it2++)
	    dr3.push_back(0.5 * (*(it2+1) - *(it2-1)));
	  
	  size_t derivsize = dr3.size();

	  //Gamma Calc
	  double gamma = 0.0;
	  double fsum = 0.0;

	  for (unsigned int i = 0; i < derivsize; i++)
	    {
	      double torsion = ((vec[i+1]) | (dr3[i])) / (vec[i+1].nrm2()); //Torsion
	      double curvature = (vec[i+1].nrm()) / pow(dr1[i+1].nrm(), 3); //Curvature

	      double instGamma = torsion / curvature;
	      gamma += instGamma;

	      double helixradius = 1.0/(curvature * (1.0+instGamma*instGamma));

	      double minradius = HUGE_VAL;

	      for (CRange::iterator it1 = range->begin(); 
		   it1 != range->end(); it1++)
		//Check this particle is not the same, or adjacent
		if (*it1 != *(range->begin()+2+i)
		    && *it1 != *(range->begin()+1+i)
		    && *it1 != *(range->begin()+3+i))
		  for (CRange::iterator it2 = range->begin() + 1; 
		       it2 != range->end() - 1; it2++)
		    //Check this particle is not the same, or adjacent to the studied particle
		    if (*it1 != *it2
			&& *it2 != *(range->begin()+2+i)
			&& *it2 != *(range->begin()+1+i)
			&& *it2 != *(range->begin()+3+i))
		      {
			//We have three points, calculate the lengths
			//of the triangle sides
			double a = (Sim->particleList[*it1].getPosition() 
				  - Sim->particleList[*it2].getPosition()).nrm(),
			  b = (Sim->particleList[*(range->begin()+2+i)].getPosition() 
				  - Sim->particleList[*it2].getPosition()).nrm(),
			  c = (Sim->particleList[*it1].getPosition() 
			       - Sim->particleList[*(range->begin()+2+i)].getPosition()).nrm();

			//Now calc the area of the triangle
			double s = (a + b + c) / 2.0;
			double A = std::sqrt(s * (s - a) * (s - b) * (s - c));
			double R = a * b * c / (4.0 * A);
			if (R < minradius) minradius = R;			 
		      }
	      fsum += minradius / helixradius;
	    }

	  gamma /= derivsize;
	  sysGamma += gamma;
	  fsum /= derivsize;

	  ++count;
	  //Restrict the data collection to reasonable bounds
	  if (gamma < 10 && gamma > -10)
	    dat.gammaMol.addVal(gamma);

	  dat.f.addVal(fsum);
	}

      if (sysGamma < 10 && sysGamma > -10)
	dat.gammaSys.addVal(sysGamma/count);
    }
}

void 
OPCTorsion::output(xml::XmlStream& XML)
{
  XML << xml::tag("ChainTorsion");
  
  BOOST_FOREACH(CTCdata& dat, chains)
    {
      XML << xml::tag(dat.chainPtr->getName().c_str())
	  << xml::tag("MolecularHistogram");
      
      dat.gammaMol.outputHistogram(XML, 1.0);
      
      XML << xml::endtag("MolecularHistogram")
	  << xml::tag("SystemHistogram");
      
      dat.gammaSys.outputHistogram(XML, 1.0);
      
      XML << xml::endtag("SystemHistogram")
	  << xml::tag("FHistogram");
      
      dat.f.outputHistogram(XML, 1.0);
      
      XML << xml::endtag("FHistogram")
	  << xml::endtag(dat.chainPtr->getName().c_str());
    }

  XML << xml::endtag("ChainTorsion");
}
