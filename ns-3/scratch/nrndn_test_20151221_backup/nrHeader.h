/*
 * nrHeader.h
 *
 *  Created on: Jan 15, 2015
 *      Author: chen
 */

#ifndef NRHEADER_H_
#define NRHEADER_H_

#include "ns3/header.h"
#include "ns3/address-utils.h"
#include "ns3/ndn-name.h"


#include <vector>

namespace ns3
{
namespace ndn
{
namespace nrndn
{

class nrHeader: public Header
{
public:
	nrHeader();
	nrHeader(const uint32_t& sourceId,
			const double& x,
			const double& y,
			const std::vector<uint32_t>& priorityList);
	virtual ~nrHeader();

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


	const std::vector<uint32_t>& getPriorityList() const
	{
		return m_priorityList;
	}

	void setPriorityList(const std::vector<uint32_t>& priorityList)
	{
		m_priorityList = priorityList;
	}

	uint32_t getSourceId() const
	{
		return m_sourceId;
	}

	void setSourceId(uint32_t sourceId)
	{
		m_sourceId = sourceId;
	}

	double getX() const
	{
		return m_x;
	}

	void setX(double x)
	{
		m_x=x;
	}

	double getY() const
	{
		return m_y;
	}

	void setY(double y)
	{
		m_y = y;
	}

	//\}

private:
	uint32_t		m_sourceId;	//\ (source)	id of source node (source)
	double			m_x;		//\ (forwarder)	forwarder x coordinate, not source node position!!!!
	double 			m_y;    	//\ (forwarder)	forwarder y coordinate, not source node position!!!!
	std::vector<uint32_t>
				  	m_priorityList;//\(forwarder)	priority list indicating the gap between transmitting
};

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NRHEADER_H_ */
