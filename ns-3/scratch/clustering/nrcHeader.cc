/*
 * nrcHeader.cc
 *
 *  Created on: Dec 26, 2013
 *      Author: cys
 */

#include "nrcHeader.h"
#include "ns3/address-utils.h"

namespace ns3
{
namespace nrc
{

nrcHeader::nrcHeader() :
		m_state(undecide), m_nbnum(0), m_rrt(0), m_velocity(0), m_location(0),m_x(0),m_y(0)
        //,m_ch("127.0.0.1")
{
	// TODO Auto-generated constructor stub

}

nrcHeader::nrcHeader(NodeState state, uint32_t nbnum,
		double rrt, double velocity, double location,double x,double y,
		std::string lane, std::vector<std::string> route):
		m_state(state), m_nbnum(nbnum), m_rrt(rrt), m_velocity(velocity), m_location(location),
		m_x(x),m_y(y),
		m_lane(lane),m_route(route)
{
}

nrcHeader::nrcHeader(const nrcHeader& header):
		m_state(header.m_state), m_nbnum(header.m_nbnum), m_rrt(header.m_rrt), m_velocity(header.m_velocity), m_location(header.m_location),
	//	m_ch(header.m_ch),
		m_x(header.m_x),m_y(header.m_y),
		m_lane(header.m_lane),m_route(header.m_route)
{
}

nrcHeader::~nrcHeader()
{
	// TODO Auto-generated destructor stub
}

TypeId nrcHeader::GetTypeId()
{
	static TypeId tid = TypeId ("nrcHeader")
	    .SetParent<Header> ()
	    .AddConstructor<nrcHeader> ()
	    ;
	return tid;
}

TypeId nrcHeader::GetInstanceTypeId() const
{
	return GetTypeId ();
}

uint32_t nrcHeader::GetSerializedSize() const
{
	uint32_t size=1;
	size += sizeof(m_nbnum);
	size += sizeof(m_rrt);
	size += sizeof(m_velocity);
	size += sizeof(m_location);
	size += sizeof(m_x);
	size += sizeof(m_y);

	size += sizeof(uint32_t);
	size += m_lane.size();

	size += sizeof(uint32_t);
	for (uint32_t p = 0; p < m_route.size(); p++)
	{
		size += sizeof(uint32_t);  //\ m_route[p] size's number of byte
		size += m_route[p].size(); //\ m_route[p] size
	}

	return size;
}

void nrcHeader::Serialize(Buffer::Iterator start) const
{
	Buffer::Iterator& i = start;
	i.WriteU8 ((uint8_t) m_state);
	i.WriteU32(m_nbnum);
	i.Write((uint8_t*)&m_rrt,     sizeof(m_rrt));
	i.Write((uint8_t*)&m_velocity,sizeof(m_velocity));
	i.Write((uint8_t*)&m_location,sizeof(m_location));
	i.Write((uint8_t*)&m_x,sizeof(m_x));
	i.Write((uint8_t*)&m_y,sizeof(m_y));

	//WriteTo (i, m_ch);

	i.WriteU32((uint32_t)m_lane.size());
	i.Write((uint8_t*) m_lane.data(), m_lane.size());

	i.WriteU32((uint32_t)m_route.size());
	for (uint32_t p = 0; p < m_route.size(); p++)
	{
		i.WriteU32((uint32_t)m_route[p].size());
		i.Write((uint8_t*) m_route[p].data(), m_route[p].size());
	}
}

uint32_t nrcHeader::Deserialize(Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	uint8_t type = i.ReadU8();
	switch (type)
	{
	case undecide:
	case clusterhead:
	case clustermember:
	case futureclusterhead:
	{
		m_state = (NodeState) type;
		break;
	}
	default:
	//	m_valid = false;
		;
	}

	m_nbnum = i.ReadU32();
	i.Read((uint8_t*)&m_rrt,sizeof(m_rrt));
	i.Read((uint8_t*)&m_velocity,sizeof(m_velocity));
	i.Read((uint8_t*)&m_location,sizeof(m_location));
	i.Read((uint8_t*)&m_x,sizeof(m_x));
	i.Read((uint8_t*)&m_y,sizeof(m_y));

	//ReadFrom (i, m_ch);

	uint32_t length    =i.ReadU32();
	char* data         =new char[length+1];
	i.Read((uint8_t*)data,length);
	data[length]='\0';
	std::string lane(data);
	m_lane=lane;
	delete[] data;

	m_route.clear();
	//deserialize route:
	uint32_t size = i.ReadU32();

	for (uint32_t p = 0; p < size; p++)
	{
		length = i.ReadU32();
		data = new char[length+1];
		i.Read((uint8_t*)data,length);
		data[length]='\0';
		std::string edge(data);
		m_route.push_back(edge);
		delete[] data;
	}

	uint32_t dist = i.GetDistanceFrom(start);
	NS_ASSERT(dist == GetSerializedSize());
	return dist;
}



void nrcHeader::Print(std::ostream& os) const
{
}

//-----------------------------------------------------------------------------
// TypeHeader
//-----------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED (TypeHeader);

TypeHeader::TypeHeader (MessageType t) :
  m_type (t), m_valid (true)
{
}

TypeId
TypeHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("TypeHeader")
    .SetParent<Header> ()
    .AddConstructor<TypeHeader> ()
  ;
  return tid;
}

TypeId
TypeHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
TypeHeader::GetSerializedSize () const
{
  return 1;
}

void
TypeHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 ((uint8_t) m_type);
}

uint32_t
TypeHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();
  m_valid = true;
  switch (type)
    {
    case AODVTYPE_RREQ:
    case AODVTYPE_RREP:
    case AODVTYPE_RERR:
    case AODVTYPE_RREP_ACK:
      {
        m_type = (MessageType) type;
        break;
      }
    default:
      m_valid = false;
    }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
TypeHeader::Print (std::ostream &os) const
{
  switch (m_type)
    {
    case AODVTYPE_RREQ:
      {
        os << "RREQ";
        break;
      }
    case AODVTYPE_RREP:
      {
        os << "RREP";
        break;
      }
    case AODVTYPE_RERR:
      {
        os << "RERR";
        break;
      }
    case AODVTYPE_RREP_ACK:
      {
        os << "RREP_ACK";
        break;
      }
    default:
      os << "UNKNOWN_TYPE";
    }
}

bool
TypeHeader::operator== (TypeHeader const & o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream &
operator<< (std::ostream & os, TypeHeader const & h)
{
  h.Print (os);
  return os;
}
//-----------------------------------------------------------------------------
// ClusterMessageHeader
//-----------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED (ClusterMessageHeader);

ClusterMessageHeader::ClusterMessageHeader (ClusterMessage t) :
  m_type (t), m_valid (true)
{
}

TypeId
ClusterMessageHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("ClusterMessageHeader")
    .SetParent<Header> ()
    .AddConstructor<ClusterMessageHeader> ()
  ;
  return tid;
}

TypeId
ClusterMessageHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
ClusterMessageHeader::GetSerializedSize () const
{
  return 1;
}

void
ClusterMessageHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 ((uint8_t) m_type);
}

uint32_t
ClusterMessageHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  uint8_t type = i.ReadU8 ();
  m_valid = true;
  switch (type)
    {
    case JOIN:
    case ACK:
    case FCH:
    case QUERY:
    case RESPONSE:
    case NCH:
    case CGW:
      {
        m_type = (ClusterMessage) type;
        break;
      }
    default:
      m_valid = false;
    }
  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
ClusterMessageHeader::Print (std::ostream &os) const
{
  switch (m_type)
    {
    case JOIN:
      {
        os << "JOIN";
        break;
      }
    case ACK:
      {
        os << "ACK";
        break;
      }
    case FCH:
      {
        os << "FCH";
        break;
      }
    case QUERY:
      {
        os << "QUERY";
        break;
      }
    case RESPONSE:
      {
        os << "RESPONSE";
        break;
      }
    case NCH:
      {
        os << "NCH";
        break;
      }
    case CGW:
      {
        os << "CGW";
        break;
      }
    default:
      os << "UNKNOWN_TYPE";
    }
}

bool
ClusterMessageHeader::operator== (ClusterMessageHeader const & o) const
{
  return (m_type == o.m_type && m_valid == o.m_valid);
}

std::ostream &
operator<< (std::ostream & os, ClusterMessageHeader const & h)
{
  h.Print (os);
  return os;
}


//-----------------------------------------------------------------------------
// RREP
//-----------------------------------------------------------------------------

RrepHeader::RrepHeader (uint8_t prefixSize, uint8_t hopCount, Ipv4Address dst,
                        uint32_t dstSeqNo, Ipv4Address origin, Time lifeTime) :
  m_flags (0), m_prefixSize (prefixSize), m_hopCount (hopCount),
  m_dst (dst), m_dstSeqNo (dstSeqNo), m_origin (origin)
{
  m_lifeTime = uint32_t (lifeTime.GetMilliSeconds ());
}

NS_OBJECT_ENSURE_REGISTERED (RrepHeader);

TypeId
RrepHeader::GetTypeId ()
{
  static TypeId tid = TypeId ("RrepHeader")
    .SetParent<Header> ()
    .AddConstructor<RrepHeader> ()
  ;
  return tid;
}

TypeId
RrepHeader::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
RrepHeader::GetSerializedSize () const
{
  return 19;
}

void
RrepHeader::Serialize (Buffer::Iterator i) const
{
  i.WriteU8 (m_flags);
  i.WriteU8 (m_prefixSize);
  i.WriteU8 (m_hopCount);
  WriteTo (i, m_dst);
  i.WriteHtonU32 (m_dstSeqNo);
  WriteTo (i, m_origin);
  i.WriteHtonU32 (m_lifeTime);
}

uint32_t
RrepHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;

  m_flags = i.ReadU8 ();
  m_prefixSize = i.ReadU8 ();
  m_hopCount = i.ReadU8 ();
  ReadFrom (i, m_dst);
  m_dstSeqNo = i.ReadNtohU32 ();
  ReadFrom (i, m_origin);
  m_lifeTime = i.ReadNtohU32 ();

  uint32_t dist = i.GetDistanceFrom (start);
  NS_ASSERT (dist == GetSerializedSize ());
  return dist;
}

void
RrepHeader::Print (std::ostream &os) const
{
  os << "destination: ipv4 " << m_dst << " sequence number " << m_dstSeqNo;
  if (m_prefixSize != 0)
    {
      os << " prefix size " << m_prefixSize;
    }
  os << " source ipv4 " << m_origin << " lifetime " << m_lifeTime
     << " acknowledgment required flag " << (*this).GetAckRequired ();
}

void
RrepHeader::SetLifeTime (Time t)
{
  m_lifeTime = t.GetMilliSeconds ();
}

Time
RrepHeader::GetLifeTime () const
{
  Time t (MilliSeconds (m_lifeTime));
  return t;
}

void
RrepHeader::SetAckRequired (bool f)
{
  if (f)
    m_flags |= (1 << 6);
  else
    m_flags &= ~(1 << 6);
}

bool
RrepHeader::GetAckRequired () const
{
  return (m_flags & (1 << 6));
}

void
RrepHeader::SetPrefixSize (uint8_t sz)
{
  m_prefixSize = sz;
}

uint8_t
RrepHeader::GetPrefixSize () const
{
  return m_prefixSize;
}

bool
RrepHeader::operator== (RrepHeader const & o) const
{
  return (m_flags == o.m_flags && m_prefixSize == o.m_prefixSize &&
          m_hopCount == o.m_hopCount && m_dst == o.m_dst && m_dstSeqNo == o.m_dstSeqNo &&
          m_origin == o.m_origin && m_lifeTime == o.m_lifeTime);
}

void
RrepHeader::SetHello (Ipv4Address origin, uint32_t srcSeqNo, Time lifetime)
{
  m_flags = 0;
  m_prefixSize = 0;
  m_hopCount = 0;
  m_dst = origin;
  m_dstSeqNo = srcSeqNo;
  m_origin = origin;
  m_lifeTime = lifetime.GetMilliSeconds ();
}

std::ostream &
operator<< (std::ostream & os, RrepHeader const & h)
{
  h.Print (os);
  return os;
}

//-----------------------------------------------------------------------------
// fchHeader
//-----------------------------------------------------------------------------

FchHeader::FchHeader()
{
}

FchHeader::FchHeader(std::vector<Ipv4Address> list):m_memberlist(list)
{
}

FchHeader::FchHeader(const FchHeader& header):m_memberlist(header.m_memberlist)
{
}

FchHeader::~FchHeader()
{
}

TypeId FchHeader::GetTypeId()
{
	static TypeId tid = TypeId ("FchHeader")
	    .SetParent<Header> ()
	    .AddConstructor<FchHeader> ()
	    ;
	return tid;
}

TypeId FchHeader::GetInstanceTypeId() const
{
	return GetTypeId ();
}

uint32_t FchHeader::GetSerializedSize() const
{
	uint32_t size = 0;
	size += sizeof(uint32_t);
	uint32_t length = m_memberlist.size();
	size += sizeof(uint32_t)*length;  //\ m_route[p] size's number of byte
	return size;
}

void FchHeader::Serialize(Buffer::Iterator start) const
{
	Buffer::Iterator i = start;
	i.WriteU32(m_memberlist.size());
	for(uint32_t k=0;k<m_memberlist.size();k++)
	{
		WriteTo (i, m_memberlist[k]);
	}
}

uint32_t FchHeader::Deserialize(Buffer::Iterator start)
{
	Buffer::Iterator i = start;

	m_memberlist.clear();
	Ipv4Address addr;
	uint32_t size = i.ReadU32();
	for (uint32_t p = 0; p < size; p++)
	{
		ReadFrom (i, addr);
		m_memberlist.push_back(addr);
	}

	uint32_t dist = i.GetDistanceFrom(start);
	NS_ASSERT(dist == GetSerializedSize());
	return dist;
}

void FchHeader::Print(std::ostream& os) const
{
}


//-----------------------------------------------------------------------------
//  nchHeader
//-----------------------------------------------------------------------------

nchHeader::nchHeader()
{
}

nchHeader::nchHeader(std::map<Ipv4Address,double> list):m_nch(list)
{
}

nchHeader::nchHeader(const nchHeader& header):m_nch(header.m_nch)
{
}

nchHeader::~nchHeader()
{
}

TypeId nchHeader::GetTypeId()
{
	static TypeId tid = TypeId ("nchHeader")
	    .SetParent<Header> ()
	    .AddConstructor<nchHeader> ()
	    ;
	return tid;
}

TypeId nchHeader::GetInstanceTypeId() const
{
	return GetTypeId ();
}

uint32_t nchHeader::GetSerializedSize() const
{
	uint32_t size = 0;
	size += sizeof(uint32_t);
	uint32_t length = m_nch.size();
	size += (sizeof(uint32_t)+sizeof(double))*length;
	return size;
}

void nchHeader::Serialize(Buffer::Iterator start) const
{
	Buffer::Iterator i = start;
	i.WriteU32(m_nch.size());
	std::map<Ipv4Address, double>::const_iterator it;
	for(it=m_nch.begin();it!=m_nch.end();it++)
	{
		WriteTo (i, it->first);
		i.Write((uint8_t*)&it->second,sizeof(it->second));
	}
}

uint32_t nchHeader::Deserialize(Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_nch.clear();
	Ipv4Address addr;
	double stabletime;
	uint32_t size = i.ReadU32();
	for (uint32_t p = 0; p < size; p++)
	{
		ReadFrom (i, addr);
		i.Read((uint8_t*)&stabletime,sizeof(stabletime));
		m_nch[addr]=stabletime;
	}
	uint32_t dist = i.GetDistanceFrom(start);
	NS_ASSERT(dist == GetSerializedSize());
	return dist;
}

void nchHeader::Print(std::ostream& os) const
{
}

} /* namespace nrc */
} /* namespace ns3 */


