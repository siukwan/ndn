/*
 * distance-based-forwarding.h
 *
 *  Created on: Mar 2, 2015
 *      Author: chen
 */

#ifndef DISTANCE_BASED_FORWARDING_H_
#define DISTANCE_BASED_FORWARDING_H_

#include "NodeSensor.h"
#include "LRUCache.h"

#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/timer.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "ns3/string.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ndn-header-helper.h"
#include "ns3/node.h"
#include "nrndn-Neighbors.h"

#include <vector>
#include <map>
#include <unordered_set>

namespace ns3
{
namespace ndn
{
namespace fw
{
namespace nrndn
{

class DistanceBasedForwarding: public ForwardingStrategy
{
public:

	static TypeId GetTypeId();

	DistanceBasedForwarding();
	virtual ~DistanceBasedForwarding();

	/**
	 * \brief	Start protocol operation,
	 *          You should execute Start() to make the forwarding strategy start to work
	 *          It will NOT start/stop automatically!
	 */
	void Start ();

	/**
	 * \brief	Stop protocol operation,
	 *          stop it using Stop().
	 *          It will NOT start/stop automatically!
	 */
	void Stop ();


	/**
	 * \brief Actual processing of incoming Ndn interests. Note, interests do not have payload
	 *
	 * Processing Interest packets
	 * @param face     incoming face
	 * @param interest Interest packet
	 */
	virtual void
	OnInterest(Ptr<Face> face, Ptr<Interest> interest);

	/**
	 * \brief Actual processing of incoming Ndn content objects
	 *
	 * Processing Data packets
	 * @param face    incoming face
	 * @param data    Data packet
	 */
	virtual void
	OnData(Ptr<Face> face, Ptr<Data> data);

	/**
	 * @brief Event fired every time face is added to NDN stack
	 * @param face face to be removed
	 */
	virtual void
	AddFace(Ptr<Face> face);

	/**
	 * @brief Event fired every time face is removed from NDN stack
	 * @param face face to be removed
	 *
	 * For example, when an application terminates, AppFace is removed and this method called by NDN stack.
	 */
	virtual void
	RemoveFace(Ptr<Face> face);

	// If the interest packet is hello message, its scope will be set as HELLO_MESSAGE = 2;
	enum
	{
		HELLO_MESSAGE = 2,
	};
protected:
	virtual void NotifyNewAggregate();

	virtual bool
	DoPropagateInterest(Ptr<Face> inFace, Ptr<const Interest> interest,
			Ptr<pit::Entry> pitEntry);

private:

	/**
	  * \brief	Process hello message
	  */
	void ProcessHello (Ptr<Interest> interest);
	/**
	 * \brief	Schedule next send of hello message
	 */
	void HelloTimerExpire ();
	/**
	 * \brief	send the hello message
	 *  		The hello message is for compare purpose, not useful for forwarding
	 */
	void SendHello ();


	/**
	 * \brief	the function which will be executed after DataPacketTimer expire
	 * @param	src	the data packet received
	 */
	void ForwardDataPacket(Ptr<Data> src);

	/**
	 * \brief	Send the data packet immediately,
	 * 			remember to wait a random interval before execute this function
	 */
	void SendDataPacket(Ptr<Data> data);

	/**
	 * \brief	Send the interest packet immediately,
	 * 			remember to wait a random interval before execute this function
	 */
	void SendInterestPacket(Ptr<Interest> interest);

	/**
	 * 	\brief	Expire the DataPacketTimer, if the TimerCallbackFunciton is already executed,
	 * 			it will do nothing
	 * 	@param  nodeId	node id
	 * 	@param  signature node signature
	 */
	void ExpireDataPacketTimer(uint32_t nodeId,uint32_t signature);

	/**
	 * 	\brief	recognize the duplicated data
	 *	@param	id    the node id of the data packet
	 *	@param  seq   the nonce of the data packet
	 *	\return	true if it is duplicated
	 */
	bool isDuplicatedData(uint32_t id, uint32_t signature);

	/**
	 * \brief	When a data packet which the node interested has been received,
	 * 			the upper layer will be notified.
	 * 													 ------------
	 * 			ForwardingStrategy<==Face::ReceiveData <-|	Face	|->Face::SendData==>other app/dev bind with face
	 * 				::OnData				/Interest	 ------------	    /Interest
	 */
	void NotifyUpperLayer(Ptr<Data> data);

private:
	typedef ForwardingStrategy super;

	//It is use for finding a scheduled sending event for a paticular data packet, nodeid+seq can locate a sending event
	std::map< uint32_t, std::map<uint32_t, EventId> > m_sendingDataEvent;

	/**
	  * Every HelloInterval the node broadcast a  Hello message
	  */
	Time HelloInterval;
	uint32_t AllowedHelloLoss;  ///< \brief Number of hello messages which may be loss for valid link

	Timer m_htimer;			///< \brief Hello timer

	//Provides uniform random variables.
	Ptr<UniformRandomVariable> m_uniformRandomVariable;

	Ptr<ndn::nrndn::NodeSensor> m_sensor;

	uint32_t				m_CacheSize;

	ndn::nrndn::cache::LRUCache<uint32_t,bool>
							m_dataSignatureSeen;///< \brief record the signature that is used to detect and discard duplicate data messages


	std::vector<Ptr<Face> >      m_outFaceList;///< \brief face list(NetDeviceFace) that interest/data packet will be sent down to
	std::vector<Ptr<Face> >      m_inFaceList;///< \brief face list(AppFace) that interest/data packet will be sent up to

	Ptr<Node>               m_node;///< \brief the node which forwarding strategy is installed

	uint32_t m_TTLMax;// \brief This value indicate that when a data is received by disinterested node, the max hop count it should be forwarded

	Time	m_offset;  	///< \brief record the last random value in scheduling the HELLO timer

	Neighbors m_nb;  ///< \brief Handle neighbors

	bool m_running;// \brief Tell if it should be running, initial is false;

	uint32_t m_runningCounter;// \brief Count how many apps are using this fw strategy. running will not stop until the counter is ZERO

};

} /* namespace nrndn */
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* DISTANCE_BASED_FORWARDING_H_ */
