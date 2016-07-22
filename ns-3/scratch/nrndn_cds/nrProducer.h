/*
 * nrProducer.h
 *
 *  Created on: Jan 12, 2015
 *      Author: chen
 */

#ifndef NRPRODUCER_H_
#define NRPRODUCER_H_

#include "ns3/ndn-app.h"

#include "ns3/ptr.h"
#include "ns3/ndn-name.h"
#include "ns3/ndn-data.h"

#include "ndn-navigation-route-heuristic-forwarding.h"
#include "CDS-based-forwarding.h"
#include "distance-based-forwarding.h"
#include "NodeSensor.h"

#include <string>

namespace ns3
{
namespace ndn
{
namespace nrndn
{

/**
 * @ingroup nrndn
 * @brief A simple Interest-sink application
 *
 *	A navigation route based data producer,
 *	which produces the data with the traffic
 *	information of the route segment at which the
 *	node currently be.
 */

class nrProducer: public App
{
public:
	static TypeId GetTypeId();

	nrProducer();
	virtual ~nrProducer();

	// inherited from NdnApp, however, nrProducer should not suppose to receive Interest Packet
	void OnInterest(Ptr<const Interest> interest);

	/*
	 * @brief	It is used for a node to send some traffic data from time to time. Kind of like OnInterest, however, it is
	 * 			sent when it is necessary, NOT at the time an interest packet arrives. The PIT is on standby, in case a
	 * 			traffic is needed to broadcast.	It is different from the normal NDN	protocol in this case.
	 * 			So this application is used for disseminate some future data instead of existing data.
	 */
	void OnSendingTrafficData();

	void laneChange(std::string, std::string);

	void addAccident();

	bool IsActive();

	  /**
	 * \brief Decide whether a lane is interested
	 */
	bool IsInterestLane(const std::string& lane);

	void ScheduleAccident(double Time);

protected:
	// inherited from Application base class.
	virtual void
	StartApplication();    // Called at time specified by Start

	virtual void
	StopApplication();     // Called at time specified by Stop

	virtual void DoInitialize(void);

	// inherited from Object class
	virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object

	virtual void OnData (Ptr<const Data> contentObject);

private:
	void setContentStore(std::string);


private:
	UniformVariable m_rand; ///< @brief nonce generator
	Name m_prefix;
	Name m_postfix;
	uint32_t m_virtualPayloadSize;
	Time m_freshness;

	uint32_t m_signature;
	Name m_keyLocator;

	typedef App super;
	Ptr<fw::nrndn::NavigationRouteHeuristic>	m_forwardingStrategy;
	Ptr<fw::nrndn::CDSBasedForwarding>			m_CDSBasedForwarding;
	Ptr<fw::nrndn::DistanceBasedForwarding>		m_DistanceForwarding;
	Ptr<NodeSensor>	m_sensor;

	//A list indicates that when it will broadcast an accident message(aka, traffic data)
	std::set<double> m_accidentList;
};

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NRPRODUCER_H_ */
