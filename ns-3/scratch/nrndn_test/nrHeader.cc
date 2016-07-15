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
	// TODO Auto-generated constructor stub

}

nrHeader::nrHeader(const uint32_t& sourceId,const double& x,const double& y,const std::vector<uint32_t>& priorityList):
		m_sourceId(sourceId),
		m_forwardId(9999999),
		m_x(x),
		m_y(y),
		m_priorityList(priorityList),
		m_tree("")
{
	// TODO Auto-generated constructor stub

}
nrHeader::nrHeader(const uint32_t& sourceId,const double& x,const double& y,const std::vector<uint32_t>& priorityList,std::string tree):
		m_sourceId(sourceId),
		m_forwardId(9999999),
		m_x(x),
		m_y(y),
		m_priorityList(priorityList),
		m_tree(tree),
		m_lane("no")
{
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
	size += sizeof(m_forwardId);
	size += sizeof(m_x);
	size += sizeof(m_y);

	//m_priorityList.size():
	size += sizeof(uint32_t);

	//each element of m_priorityList
	size += sizeof(uint32_t) * m_priorityList.size();

	//2016.1.17增加兴趣树的大小：
	size += sizeof(uint32_t);//兴趣树的大小
	size += m_tree.size();
	size += sizeof(uint32_t);
	size += m_lane.size();
	return size;
}

void nrHeader::Serialize(Buffer::Iterator start) const
{
	Buffer::Iterator& i = start;
	i.WriteHtonU32(m_sourceId);
	i.WriteHtonU32(m_forwardId);
	i.Write((uint8_t*)&m_x,sizeof(m_x));
	i.Write((uint8_t*)&m_y,sizeof(m_y));

	//2016年1月19日，小锟添加，兴趣树的序列化
	i.WriteHtonU32(m_tree.size());
	//std::cout<<m_tree<<std::endl<<"header,treesize:"<<m_tree.size()<<" totalSize:"<<GetSerializedSize()<<std::endl;
	for(uint32_t j=0;j<m_tree.size();++j)
	{
		//std::cout<<(int)(m_tree[j])<<" "<<std::endl;
		//需要强制转换每一个char为uint8_t才能成功序列化和反序列化
		uint8_t tmp = (uint8_t )(m_tree[j]);
		i.Write((uint8_t*)&tmp,sizeof(tmp));
	}

	//2016年7月16日，小锟添加，当前道路的序列化
	i.WriteHtonU32(m_lane.size());
	for(uint32_t j=0;j<m_lane.size();++j)
	{
		//需要强制转换每一个char为uint8_t才能成功序列化和反序列化
		uint8_t tmp = (uint8_t )(m_lane[j]);
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
	m_forwardId	=	i.ReadNtohU32();
	i.Read((uint8_t*)&m_x,sizeof(m_x));
	i.Read((uint8_t*)&m_y,sizeof(m_y));

	uint32_t treesize  = i.ReadNtohU32();
	//std::cout<<"读取的大小："<<treesize<<std::endl;
	m_tree="";
	for(uint32_t j = 0;j<treesize;++j)
	{//还原树
		uint8_t a;
		i.Read((uint8_t*)&a,sizeof(a));
		m_tree+=(char)a;
		//std::cout<<a;
	}
	
	uint32_t lanesize  = i.ReadNtohU32();
	m_lane="";
	for(uint32_t j = 0;j<lanesize;++j)
	{//还原树
		uint8_t a;
		i.Read((uint8_t*)&a,sizeof(a));
		m_lane+=(char)a;
		//std::cout<<a;
	}
	
	
	uint32_t size  = i.ReadNtohU32();
	for (uint32_t p = 0; p < size; p++)
	{
		m_priorityList.push_back(i.ReadNtohU32());
	}
	uint32_t dist = i.GetDistanceFrom(start);
	//std::cout<<"读取的size"<<dist<<" 原来的size"<<GetSerializedSize()<<std::endl;
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

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
