/*
 * nrcHeader.h
 *
 *  Created on: Dec 26, 2013
 *      Author: cys
 */

#ifndef NRCHEADER_H_
#define NRCHEADER_H_

#include "ns3/packet.h"
#include "ns3/header.h"
#include "ns3/enum.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include <map>
#include <iostream>
#include <vector>

#define PORT 1234

namespace ns3
{
namespace nrc
{

enum NodeState{
	undecide      = 0,
	clusterhead   = 1,
	clustermember = 2,
	futureclusterhead =3
};

enum MessageType
{
  AODVTYPE_RREQ  = 1,   //!< AODVTYPE_RREQ     Cluster Control
  AODVTYPE_RREP  = 2,   //!< AODVTYPE_RREP     HELLO
  AODVTYPE_RERR  = 3,   //!< AODVTYPE_RERR     Send data
  AODVTYPE_RREP_ACK = 4 //!< AODVTYPE_RREP_ACK to be disscuss
};

enum ClusterMessage
{
	JOIN    = 1,
	ACK     = 2,
	FCH     = 3,
	QUERY   = 4,
	RESPONSE= 5,
	NCH     = 6,
	CGW     = 7
};

class nrcHeader: public ns3::Header
{
public:
	nrcHeader();
	nrcHeader(NodeState state, uint32_t nbnum,
			double rrt, double velocity, double location,double x,double y,
			std::string lane, std::vector<std::string> route);
	nrcHeader(const nrcHeader &header);
	virtual ~nrcHeader();

	///\name Header serialization/deserialization
	//\{
	static TypeId GetTypeId();
	TypeId GetInstanceTypeId() const;
	uint32_t GetSerializedSize() const;
	void Serialize(Buffer::Iterator start) const;
	uint32_t Deserialize(Buffer::Iterator start);
	void Print(std::ostream &os) const;



	//\}

	 ///\name Fields
	  //\{
	const std::string& getLane() const{return m_lane;}
	void setLane(const std::string& lane){m_lane = lane;}

	double getLocation() const{return m_location;}
	void setLocation(double location){m_location = location;}

	uint32_t getNbnum() const{return m_nbnum;}
	void setNbnum(uint32_t nbnum){m_nbnum = nbnum;}

	const std::vector<std::string>& getRoute() const{return m_route;}
	void setRoute(const std::vector<std::string>& route){m_route = route;}

	double getRrt() const{return m_rrt;}
	void setRrt(double rrt){m_rrt = rrt;}

	NodeState getState() const{return m_state;}
	void setState(NodeState state){m_state = state;}

	double getVelocity() const{return m_velocity;}
	void setVelocity(double velocity){m_velocity = velocity;}

	double getX() const
	{
		return m_x;
	}

	double getY() const
	{
		return m_y;
	}

	//Ipv4Address getCh() const{return m_ch;}
	//void setCh(Ipv4Address ch){	m_ch = ch;}

	//\}

private:
	NodeState    m_state;   //\ node state(un,ch,cm,fch)
	uint32_t     m_nbnum;   //\ number of neighbors
	double       m_rrt;     //\ Residual route time
	double       m_velocity;//\ speed
	double       m_location;//\ vehicle location
	double       m_x;
	double       m_y;
	//Ipv4Address  m_ch;      //\ CH IP Address
	std::string  m_lane;    //\ vehicle in which lane
	std::vector<std::string> m_route;//\ route information, represented by an array of strings

};


class TypeHeader : public Header
{
public:
  /// c-tor
  TypeHeader (MessageType t = AODVTYPE_RREQ);

  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  //\}

  /// Return type
  MessageType Get () const { return m_type; }
  /// Check that type if valid
  bool IsValid () const { return m_valid; }
  bool operator== (TypeHeader const & o) const;
private:
  MessageType m_type;
  bool m_valid;
};

std::ostream & operator<< (std::ostream & os, TypeHeader const & h);

class ClusterMessageHeader : public Header
{
public:
  /// c-tor
	ClusterMessageHeader (ClusterMessage t = JOIN);

  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  //\}

  /// Return type
  ClusterMessage Get () const { return m_type; }
  /// Check that type if valid
  bool IsValid () const { return m_valid; }
  bool operator== (ClusterMessageHeader const & o) const;
private:
  ClusterMessage m_type;
  bool m_valid;
};

std::ostream & operator<< (std::ostream & os, ClusterMessageHeader const & h);



/**
* \ingroup aodv
* \brief Route Reply (RREP) Message Format
  \verbatim
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |     Type      |R|A|    Reserved     |Prefix Sz|   Hop Count   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Destination IP address                    |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  Destination Sequence Number                  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                    Originator IP address                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                           Lifetime                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  \endverbatim
*/
class RrepHeader : public Header
{
public:
  /// c-tor
  RrepHeader (uint8_t prefixSize = 0, uint8_t hopCount = 0, Ipv4Address dst =
                Ipv4Address (), uint32_t dstSeqNo = 0, Ipv4Address origin =
                Ipv4Address (), Time lifetime = MilliSeconds (0));
  ///\name Header serialization/deserialization
  //\{
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const;
  uint32_t GetSerializedSize () const;
  void Serialize (Buffer::Iterator start) const;
  uint32_t Deserialize (Buffer::Iterator start);
  void Print (std::ostream &os) const;
  //\}

  ///\name Fields
  //\{
  void SetHopCount (uint8_t count) { m_hopCount = count; }
  uint8_t GetHopCount () const { return m_hopCount; }
  void SetDst (Ipv4Address a) { m_dst = a; }
  Ipv4Address GetDst () const { return m_dst; }
  void SetDstSeqno (uint32_t s) { m_dstSeqNo = s; }
  uint32_t GetDstSeqno () const { return m_dstSeqNo; }
  void SetOrigin (Ipv4Address a) { m_origin = a; }
  Ipv4Address GetOrigin () const { return m_origin; }
  void SetLifeTime (Time t);
  Time GetLifeTime () const;
  //\}

  ///\name Flags
  //\{
  void SetAckRequired (bool f);
  bool GetAckRequired () const;
  void SetPrefixSize (uint8_t sz);
  uint8_t GetPrefixSize () const;
  //\}

  /// Configure RREP to be a Hello message
  void SetHello (Ipv4Address src, uint32_t srcSeqNo, Time lifetime);

  bool operator== (RrepHeader const & o) const;
private:
  uint8_t       m_flags;                  ///< A - acknowledgment required flag
  uint8_t       m_prefixSize;         ///< Prefix Size
  uint8_t             m_hopCount;         ///< Hop Count
  Ipv4Address   m_dst;              ///< Destination IP Address
  uint32_t      m_dstSeqNo;         ///< Destination Sequence Number
  Ipv4Address     m_origin;           ///< Source IP Address
  uint32_t      m_lifeTime;         ///< Lifetime (in milliseconds)
};

std::ostream & operator<< (std::ostream & os, RrepHeader const &);


class FchHeader: public ns3::Header
{
public:
	FchHeader();
	FchHeader(std::vector<Ipv4Address> list);
	FchHeader(const FchHeader &header);
	virtual ~FchHeader();

	///\name Header serialization/deserialization
	//\{
	static TypeId GetTypeId();
	TypeId GetInstanceTypeId() const;
	uint32_t GetSerializedSize() const;
	void Serialize(Buffer::Iterator start) const;
	uint32_t Deserialize(Buffer::Iterator start);
	void Print(std::ostream &os) const;
	//\}

	///\name Fields
	//\{
	const std::vector<Ipv4Address>& getMemberlist() const
	{
		return m_memberlist;
	}
	void setMemberlist(const std::vector<Ipv4Address>& memberlist)
	{
		m_memberlist = memberlist;
	}
	//\}

private:
	std::vector<Ipv4Address> m_memberlist;//\ cluster member list
};

class nchHeader: public ns3::Header
{
public:
	nchHeader();
	nchHeader(std::map<Ipv4Address, double> list);
	nchHeader(const nchHeader &header);
	virtual ~nchHeader();

	///\name Header serialization/deserialization
	//\{
	static TypeId GetTypeId();
	TypeId GetInstanceTypeId() const;
	uint32_t GetSerializedSize() const;
	void Serialize(Buffer::Iterator start) const;
	uint32_t Deserialize(Buffer::Iterator start);
	void Print(std::ostream &os) const;

	//\}

	///\name Fields
	//\{
	const std::map<Ipv4Address, double>& getNch() const
	{
		return m_nch;
	}

	void setNch(const std::map<Ipv4Address, double>& nch)
	{
		m_nch = nch;
	}
	//\}

private:
	std::map<Ipv4Address,double> m_nch;//\ neighbor cluster head list
};


} /* namespace nrc */
} /* namespace ns3 */



#endif /* NRCHEADER_H_ */
