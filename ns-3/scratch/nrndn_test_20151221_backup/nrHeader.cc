/*
 * nrHeader.cc
 *
 *  Created on: Jan 15, 2015
 *      Author: chen
 */

#include "nrHeader.h"

namespace ns3
{
namespace ndn
{
namespace nrndn
{

nrHeader::nrHeader():
		m_sourceId(0),
		m_x(0),
		m_y(0)
{
	m_route="";
	// TODO Auto-generated constructor stub

}

nrHeader::nrHeader(const uint32_t& sourceId,const double& x,const double& y,const std::vector<uint32_t>& priorityList):
		m_sourceId(sourceId),
		m_x(x),
		m_y(y),
		m_priorityList(priorityList)
{
	m_route="";
	// TODO Auto-generated constructor stub

}


nrHeader::~nrHeader()
{
	// TODO Auto-generated destructor stub
}

TypeId nrHeader::GetTypeId()
{
	static TypeId tid = TypeId ("ns3::ndn::nrndn::nrHeader")
	    .SetParent<Header> ()
	    .AddConstructor<nrHeader> ()
	    ;
	return tid;
}

TypeId nrHeader::GetInstanceTypeId() const
{
	return GetTypeId ();
}

uint32_t nrHeader::GetSerializedSize() const
{
	uint32_t size=0;
	size += sizeof(m_sourceId);
	size += sizeof(m_x);
	size += sizeof(m_y);

	//m_priorityList.size():
	size += sizeof(uint32_t);
	size += sizeof(uint32_t);
	size +=m_route.size();
	//each element of m_priorityList
	size += sizeof(uint32_t) * m_priorityList.size();

	return size;
}

void nrHeader::Serialize(Buffer::Iterator start) const
{
	Buffer::Iterator& i = start;
	i.WriteHtonU32(m_sourceId);
	i.Write((uint8_t*)&m_x,sizeof(m_x));
	i.Write((uint8_t*)&m_y,sizeof(m_y));

	i.WriteHtonU32(m_route.size());
	for(uint32_t j=0;j<m_route.size();++j)
	{
		//std::cout<<(int)(m_tree[j])<<" "<<std::endl;
		//需要强制转换每一个char为uint8_t才能成功序列化和反序列化
		uint8_t tmp = (uint8_t )(m_route[j]);
		i.Write((uint8_t*)&tmp,sizeof(tmp));
	}


	i.WriteHtonU32(m_priorityList.size());
	std::vector<uint32_t>::const_iterator it;
	for (it = m_priorityList.begin(); it != m_priorityList.end(); ++it)
	{
		i.WriteHtonU32(*it);
	}
}

uint32_t nrHeader::Deserialize(Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_sourceId	=	i.ReadNtohU32();
	i.Read((uint8_t*)&m_x,sizeof(m_x));
	i.Read((uint8_t*)&m_y,sizeof(m_y));

	uint32_t routesize  = i.ReadNtohU32();
	//std::cout<<"读取的大小："<<treesize<<std::endl;
	m_route="";
	for(uint32_t j = 0;j<routesize;++j)
	{//还原树
		uint8_t a;
		i.Read((uint8_t*)&a,sizeof(a));
		m_route+=(char)a;
		//std::cout<<a;
	}
	uint32_t size  = i.ReadNtohU32();
	for (uint32_t p = 0; p < size; p++)
	{
		m_priorityList.push_back(i.ReadNtohU32());
	}

	uint32_t dist = i.GetDistanceFrom(start);
	NS_ASSERT(dist == GetSerializedSize());
	return dist;
}

void nrHeader::Print(std::ostream& os) const
{
	os<<"nrHeader conatin: NodeID="<<m_sourceId<<"\t coordinate=("<<m_x<<","<<m_y<<") priorityList=";
	std::vector<uint32_t>::const_iterator it;
	for (it = m_priorityList.begin(); it != m_priorityList.end(); ++it)
	{
		os<<*it<<"\t";
	}
	os<<std::endl;
}

void nrHeader::setRoute(const std::vector<std::string>& route)
{
	m_route="";
	for(uint32_t i=0;i<route.size();++i)
		m_route+="^"+route[i];
}

std::string nrHeader::getSerializeRoute()
{
	return m_route;
}
void nrHeader::setSerializeRoute(std::string routeStr)
{
	m_route=routeStr;
}

std::vector<std::string> nrHeader::getRoute()
{
	std::vector<std::string> route(0);
	if(m_route=="") return route;
	m_route+="^";
	std::string seg="";
	for(uint32_t i=0;i<m_route.size();++i)
	{
		if(m_route[i]=='^')
		{
			if(seg!="")
				route.push_back(seg);
			seg="";
		}
		else
			seg+=m_route[i];
	}
	return route;
}

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
