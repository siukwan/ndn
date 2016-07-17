/*
 * CDS-based-forwarding.h
 *
 *  Created on: 2015��3��4��
 *      Author: chen
 */

#ifndef SCRATCH_NRNDN_CDS_BASED_FORWARDING_H_
#define SCRATCH_NRNDN_CDS_BASED_FORWARDING_H_

#include "NodeSensor.h"
#include "LRUCache.h"
#include "nrndn-Neighbors.h"

#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/timer.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "ns3/string.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ndn-header-helper.h"
#include "ns3/node.h"

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

class CDSBasedForwarding: public ForwardingStrategy
{
public:
	enum state
	{
		dominator, dominatee
	};
	static TypeId GetTypeId (void);
	CDSBasedForwarding();
	virtual	~CDSBasedForwarding();

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
	 * \brief Actual processing of incoming Ndn interests.
	 *        overwrite the ForwardingStrategy::OnInterest
	 *        to do some works using navigation route
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

	const Ptr<ndn::nrndn::NodeSensor>& getSensor() const
	{
		return m_sensor;
	}

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
	virtual bool
	DoPropagateInterest(Ptr<Face> inFace, Ptr<const Interest> interest,
			Ptr<pit::Entry> pitEntry);

	virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object


private:
	//Some Utils functions aboute navigation route

	/**
	 * @brief Get Customize data for navigation route heuristic forwarding
	 *
	 * \return	Payload packet
	 */
	//Ptr<Packet> GetNrPayload();

	/**
	  * \brief	Process hello message
	  */
	void ProcessHello (Ptr<Interest> interest);

	/**
	 * 	\brief	recognize the duplicated data
	 *	@param	id    the node id of the data packet
	 *	@param  seq   the nonce of the data packet
	 *	\return	true if it is duplicated
	 */
	bool isDuplicatedData(uint32_t id, uint32_t signature);

	/**
	 * \brief	drop the data
	 * 			Simply do nothing
	 */
	void DropPacket();

	/**
	 * \brief	Schedule next send of hello message
	 */
	void HelloTimerExpire ();

	/**
	 * \brief	When a neighbor loses contact with it, this
	 *          funciton will be invoked
	 */
	void FindBreaksLinkToNextHop(uint32_t BreakLinkNodeId);

	/**
	 * \brief	send the hello message
	 */
	void SendHello ();

	vector<string> ExtractRouteFromName(const Name& name);

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
	 * \brief	just for lookup easier
	 */
	std::unordered_set<uint32_t>
	converVectorList(const std::vector<uint32_t>& list);

	/**
	 * \brief	When a data packet which the node interested has been received,
	 * 			the upper layer will be notified.
	 * 													 ------------
	 * 			ForwardingStrategy<==Face::ReceiveData <-|	Face	|->Face::SendData==>other app/dev bind with face
	 * 				::OnData				/Interest	 ------------	    /Interest
	 */
	void NotifyUpperLayer(Ptr<Data> data);

	/**
	 * \brief	Send the interest packet immediately,
	 * 			remember to wait a random interval before execute this function
	 */
	void SendInterestPacket(Ptr<Interest> interest);

	/**
	 * \brief	Calculate the Multipoint relay set
	 */
	void CalculateMPR();

	/**
	 * \brief	Calculate the stable time
	 */
	//double CalculateStableTime(Vector2D x, Vector2D y);

	void SCDSConstructionByID();

private:
	typedef ForwardingStrategy super;
	typedef std::unordered_map<uint32_t, Neighbors::Neighbor> NeighborList;
	typedef std::unordered_map<uint32_t, std::vector<uint32_t> > TwoHopNeighborList;
	typedef std::unordered_map<uint32_t, std::vector<uint32_t> > NeighborMPRList;
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

	Time	m_offset;  	///< \brief record the last random value in scheduling the HELLO timer

	Neighbors m_nb;  ///< \brief Handle neighbors

	TwoHopNeighborList m_2hopNb;
	NeighborMPRList    m_nbMPRList;

	bool m_running;// \brief Tell if it should be running, initial is false;

	uint32_t m_runningCounter;// \brief Count how many apps are using this fw strategy. running will not stop until the counter is ZERO

	bool m_HelloLogEnable;// \brief the switch which can turn on the log on Functions about hello messages

	state m_state; //\brief the status of the node

	std::vector<uint32_t> m_mpr;//MPR list
};

} /* namespace nrndn */
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* SCRATCH_NRNDN_CDS_BASED_FORWARDING_H_ */
