/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Based on
 *      NS-2 AODV model developed by the CMU/MONARCH group and optimized and
 *      tuned by Samir Das and Mahesh Marina, University of Cincinnati;
 *
 *      AODV-UU implementation by Erik Nordström of Uppsala University
 *      http://core.it.uu.se/core/index.php/AODV-UU
 *
 * Authors: Elena Buchatskaia <borovkovaes@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */

#include "neighbor.h"
#include "ns3/log.h"
#include <algorithm>

NS_LOG_COMPONENT_DEFINE ("nrc::Neighbors");

namespace ns3
{
namespace nrc
{
using namespace std;


Neighbors::Neighbors (Time delay) : 
  m_ntimer (Timer::CANCEL_ON_DESTROY)
{
  m_ntimer.SetDelay (delay);
  m_ntimer.SetFunction (&Neighbors::Purge, this);
  m_txErrorCallback = MakeCallback (&Neighbors::ProcessTxError, this);
}
/*
Neighbors::Neighbors(Time delay, Ipv4Address add):
 m_ntimer (Timer::CANCEL_ON_DESTROY),m_nodeadd(add)
{
 m_ntimer.SetDelay (delay);
 m_ntimer.SetFunction (&Neighbors::Purge, this);
 m_txErrorCallback = MakeCallback (&Neighbors::ProcessTxError, this);
}
*/
bool
Neighbors::IsNeighbor (Ipv4Address addr)
{
  //Purge ();
  for (std::vector<Neighbor>::const_iterator i = m_nb.begin ();
       i != m_nb.end (); ++i)
    {
      if (i->m_neighborAddress == addr)
        return true;
    }
  return false;
}

Time
Neighbors::GetExpireTime (Ipv4Address addr)
{
  Purge ();
  for (std::vector<Neighbor>::const_iterator i = m_nb.begin (); i
       != m_nb.end (); ++i)
    {
      if (i->m_neighborAddress == addr)
        return (i->m_expireTime - Simulator::Now ());
    }
  return Seconds (0);
}

void
Neighbors::Update (Ipv4Address addr, Time expire,nrcHeader nrcheader)
{
  for (std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i)
  {
	  if (i->m_neighborAddress == addr)
      {
        i->m_expireTime
          = std::max (expire + Simulator::Now (), i->m_expireTime);
        if (i->m_hardwareAddress == Mac48Address ())
          i->m_hardwareAddress = LookupMacAddress (i->m_neighborAddress);

        i->m_nrcheader=nrcheader;
        return;
      }
  }
  NS_LOG_LOGIC ("Open link to " << addr);
  //cout<<"Open link to " << addr <<endl;
 // Neighbor neighbor (addr, LookupMacAddress (addr), expire + Simulator::Now ());

  Neighbor neighbor (addr, LookupMacAddress (addr), expire + Simulator::Now (),nrcheader);
  m_nb.push_back (neighbor);
  Purge ();
 // this->PrintNeighbors();
}

struct CloseNeighbor
{
  bool operator() (const Neighbors::Neighbor & nb) const
  {
    return ((nb.m_expireTime < Simulator::Now ()) || nb.close);
  }
};

void
Neighbors::Purge ()
{
  if (m_nb.empty ())
    return;

  CloseNeighbor pred;
  if (!m_handleLinkFailure.IsNull ())
    {
	   for (std::vector<Neighbor>::iterator j = m_nb.begin (); j != m_nb.end (); ++j)
        {
          if (pred (*j))
            {
              NS_LOG_LOGIC ("Close link to " << j->m_neighborAddress);
            //  std::cout<<"Close link to " << j->m_neighborAddress<<std::endl;
              m_handleLinkFailure (j->m_neighborAddress);
            }
        }

    }
  m_nb.erase (std::remove_if (m_nb.begin (), m_nb.end (), pred), m_nb.end ());
  m_ntimer.Cancel ();
  m_ntimer.Schedule ();
 // this->PrintNeighbors();
}

void
Neighbors::ScheduleTimer ()
{
  m_ntimer.Cancel ();
  m_ntimer.Schedule ();
}

void
Neighbors::AddArpCache (Ptr<ArpCache> a)
{
  m_arp.push_back (a);
}

void
Neighbors::DelArpCache (Ptr<ArpCache> a)
{
  m_arp.erase (std::remove (m_arp.begin (), m_arp.end (), a), m_arp.end ());
}

Mac48Address
Neighbors::LookupMacAddress (Ipv4Address addr)
{
  Mac48Address hwaddr;
  for (std::vector<Ptr<ArpCache> >::const_iterator i = m_arp.begin ();
       i != m_arp.end (); ++i)
    {
      ArpCache::Entry * entry = (*i)->Lookup (addr);
      if (entry != 0 && entry->IsAlive () && !entry->IsExpired ())
        {
          hwaddr = Mac48Address::ConvertFrom (entry->GetMacAddress ());
          break;
        }
    }
  return hwaddr;
}

void
Neighbors::ProcessTxError (WifiMacHeader const & hdr)
{
  Mac48Address addr = hdr.GetAddr1 ();

  for (std::vector<Neighbor>::iterator i = m_nb.begin (); i != m_nb.end (); ++i)
    {
      if (i->m_hardwareAddress == addr)
        i->close = true;
    }
  Purge ();
}

void Neighbors::PrintNeighbors()
{
	std::cout<<"It's neighbor list is:"<<endl;
	for (std::vector<Neighbor>::iterator i = m_nb.begin();i!=m_nb.end();i++)
	{
		(*i).m_neighborAddress.Print(std::cout);
		std::cout<<"  rrt: "<<i->m_nrcheader.getRrt()<<" state: "<<i->m_nrcheader.getState()<<endl;

	}
	std::cout<<std::endl;
}

uint32_t Neighbors::getNeighborNum()
{
	return m_nb.size();
}

nrcHeader Neighbors::getNerghborHeader(uint32_t index)
{
	NS_ASSERT(index<m_nb.size());
	nrcHeader header = m_nb[index].m_nrcheader;
	return header;
}

std::vector<double> Neighbors::GetRRT()
{
	std::vector<double> rrt;
	std::vector<Neighbor>::iterator it;
	for(it=m_nb.begin();it!=m_nb.end();it++)
	{
		rrt.push_back((*it).m_nrcheader.getRrt());
	}
	return rrt;
}

vector<double> Neighbors::GetUndecideNodeRRT()
{
	std::vector<double> rrt;
	std::vector<Neighbor>::iterator it;
	for(it=m_nb.begin();it!=m_nb.end();it++)
	{
		if(it->m_nrcheader.getState()==undecide)
			rrt.push_back(it->m_nrcheader.getRrt());
	}
	return rrt;
}

bool Neighbors::getNerghborHeader(Ipv4Address index,nrcHeader& header)
{
	vector<Neighbor>::iterator it;
	for(it=m_nb.begin();it!=m_nb.end();it++)
	{
		if((*it).m_neighborAddress==index)
		{
			header=(*it).m_nrcheader;
			return true;
		}

	}
	return false;
}

double Neighbors::GetRRT(Ipv4Address& add)
{
	std::vector<Neighbor>::iterator it;
	double rrt = -1;
	//if(!this->IsNeighbor(add))
	//	return rrt;
	for (it = m_nb.begin(); it != m_nb.end(); it++)
	{
		if(it->m_neighborAddress==add)
		{
			rrt=it->m_nrcheader.getRrt();
			break;
		}
	}
	return rrt;
}

Ipv4Address Neighbors::getNeighborAddress(uint32_t index)
{
	NS_ASSERT(index<m_nb.size());
	Ipv4Address address = m_nb[index].m_neighborAddress;
	return address;
}

void Neighbors::CancelTimer()
{
	m_ntimer.Cancel();
}



std::map<Ipv4Address, nrcHeader> Neighbors::GetNodelist(NodeState s)
{
	vector<Neighbor>::iterator it;
	map<Ipv4Address, nrcHeader> m;
	Ipv4Address a;
	nrcHeader   h;
	for (it = m_nb.begin(); it != m_nb.end(); it++)
	{
		if ((*it).m_nrcheader.getState() == s)
		{
			a = (*it).m_neighborAddress;
			h = (*it).m_nrcheader;
			m.insert(map<Ipv4Address, nrcHeader>::value_type(a, h));
		}
	}
	return m;
}


} /* namespace nrc */
} /* namespace ns3 */
