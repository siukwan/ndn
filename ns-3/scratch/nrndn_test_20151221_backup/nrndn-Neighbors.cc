/*
 * nrndn-Neighbors.cc
 *
 *  Created on: Jan 29, 2015
 *      Author: chen
 */

#include "nrndn-Neighbors.h"
#include "ns3/log.h"
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("NrndnNeighbors");

namespace ns3
{
namespace ndn
{
namespace fw
{
namespace nrndn
{

Neighbors::Neighbors(Time delay) :
		  m_ntimer (Timer::CANCEL_ON_DESTROY)
{
	 m_ntimer.SetDelay (delay);
	 m_ntimer.SetFunction (&Neighbors::Purge, this);
}

Neighbors::~Neighbors()
{
	// TODO Auto-generated destructor stub
}

Time Neighbors::GetExpireTime(uint32_t id)
{
	  Purge ();
	  std::unordered_map<uint32_t,Neighbor>::const_iterator it = m_nb.find(id);
	  if(it!=m_nb.end())
	  {
		  //Found the neighbor
		  return (it->second.m_expireTime - Simulator::Now ());
	  }

	  return Seconds (0);
}

bool Neighbors::IsNeighbor(uint32_t id)
{
	  Purge ();
	  std::unordered_map<uint32_t,Neighbor>::const_iterator it = m_nb.find(id);
	  if(it!=m_nb.end())
	  {
		  //Found the neighbor
		  return true;
	  }

	  return false;
}

void Neighbors::Update(const uint32_t& id, const double& x,const double& y,const Time& expire)
{
	std::unordered_map<uint32_t,Neighbor>::iterator it = m_nb.find(id);
	if (it != m_nb.end())
	{
		//Found the neighbor
		// setp 1. update the expire time
		 it->second.m_expireTime
		   = std::max (expire + Simulator::Now (), it->second.m_expireTime);
		 // setp 2. update the x coordinate
		 it->second.m_x = x;
		 // setp 3. update the y coordinate
		 it->second.m_y = y;
		 // Finish update
		 return ;
	}

	NS_LOG_LOGIC ("Open link to " << id);
	// class Neighbor does not have default constructor, so cannot use the "[]" operator like this
	//m_nb[id]=Neighbor(lane, position,expire + Simulator::Now ());
	// I think implementing default constructor will cause unnecessary copy and initialize

	//Use insert instead:
	m_nb.insert(std::unordered_map<uint32_t,Neighbor>::
			value_type(id,Neighbor(x, y,expire + Simulator::Now ())));

	Purge ();
}

struct CloseNeighbor
{
  bool operator() (const Neighbors::Neighbor & nb) const
  {
    return ((nb.m_expireTime < Simulator::Now ()) );
  }
};

void Neighbors::Purge()
{
	if (m_nb.empty())
		return;

	CloseNeighbor pred;
	if (!m_handleLinkFailure.IsNull())
	{
		std::unordered_map<uint32_t,Neighbor>::iterator j;
		for ( j = m_nb.begin(); j != m_nb.end();)
		{
			if (pred(j->second))
			{
				NS_LOG_LOGIC("Close link to " << j->first);
				m_handleLinkFailure(j->first);
				m_nb.erase(j++);//Remember to postincrement the iterator
			}
			else
				++j;
		}
	}
	//erase an element from a standard associative container(map) is not like
	//the sequence container, use the erase method directly. Remember to
	//postincrement the iterator. (See <<effective STL>>item9)
	//Here, See the difference in ns3::AODV::Neighbors
	m_ntimer.Cancel();
	m_ntimer.Schedule();
}

void Neighbors::ScheduleTimer()
{
  m_ntimer.Cancel ();
  m_ntimer.Schedule ();
}

void Neighbors::CancelTimer()
{
	m_ntimer.Cancel ();
}
} /* namespace nrndn */
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */


