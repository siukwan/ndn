/*
 * nrConsumer.h
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */

#ifndef NRCONSUMER_H_
#define NRCONSUMER_H_

#include "NodeSensor.h"
#include "ns3/ndnSIM/apps/ndn-consumer-cbr.h"
#include <string>


namespace ns3
{
namespace ndn
{
namespace nrndn
{

class nrConsumer: public ndn::ConsumerCbr
{
public:

	static TypeId GetTypeId ();

	nrConsumer();
	virtual ~nrConsumer();

protected:
	  // inherited from App base class. Originally they were private
	  virtual void
	  StartApplication ();    ///< @brief Called at time specified by Start

	  virtual void
	  StopApplication ();     ///< @brief Called at time specified by Stop

	  virtual void
	  OnData (Ptr<const Data> contentObject);

	  /**
	   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN protocol
	   */
	  virtual void
	  ScheduleNextPacket ();

	  /**
	   * \brief get the current route for the interests
	   */
	  std::vector<std::string>
	  GetCurrentInterest();

	  /**
	   * @brief It will do the similar action like ConsumerCbr::ScheduleNextPacket do
	   * 		However, I change some details of it, so use this function to replace it.
	   */
	  void  doConsumerCbrScheduleNextPacket();

	  /**
	   * @brief Actually send packet, it will take place in Consumer::SendPacket
	   */
	  void SendPacket ();

	  virtual void
	  OnTimeout (uint32_t sequenceNumber);

	  /**
	    * @brief Get Customize data for navigation route heuristic forwarding
	    */
	  //Ptr<Packet> GetNrPayload();

	  // inherited from Object class
	  virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object

	  virtual void DoInitialize(void);

	  virtual void
	  OnInterest (Ptr<const Interest> interest);

	  bool IsInterestData(const Name& name);
private:
	  typedef ConsumerCbr super;
	  Ptr<NodeSensor> m_sensor;
	  Ptr<fw::nrndn::NavigationRouteHeuristic>		m_forwardingStrategy;

	  uint32_t m_virtualPayloadSize;
	  //Ptr<ForwardingStrategy>		m_forwardingStrategy;
};

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NRCONSUMER_H_ */
