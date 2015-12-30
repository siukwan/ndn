/*
 * ndn-navigation-route-heuristic-forwarding.h
 *
 *  Created on: Jan 14, 2015
 *      Author: chen
 */

#ifndef NDN_NAVIGATION_ROUTE_HEURISTIC_FORWARDING_H_
#define NDN_NAVIGATION_ROUTE_HEURISTIC_FORWARDING_H_

#include "ns3/ndnSIM/model/fw/green-yellow-red.h"
#include "ns3/timer.h"
#include "ns3/simulator.h"
#include "ns3/random-variable.h"
#include "ns3/string.h"
#include "ns3/random-variable-stream.h"
#include "ns3/ndn-header-helper.h"

#include "LRUCache.h"
#include "NodeSensor.h"
#include "ndn-nr-pit-impl.h"
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

/**
 * @brief This forward strategy is Navigation Route Heuristic forwarding strategy
 *        Different with traditional forwarding strategy, You should execute
 *        Start() to make the forwarding strategy start to work, and stop it using Stop().
 *        It will NOT start/stop automatically!
 */
class NavigationRouteHeuristic: public GreenYellowRed
{
public:
	static TypeId
	GetTypeId(void);



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


	/**
	 * @brief Default constructor
	 */
	NavigationRouteHeuristic ();

	/**
	 * @brief Default constructor
	 */
	virtual ~NavigationRouteHeuristic ();

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

	//uint32_t GetCacheSize() const { return m_CacheSize;	}
	//void SetCacheSize(uint32_t cacheSize);

	// If the interest packet is hello message, its scope will be set as HELLO_MESSAGE = 2;
	enum
	{
		HELLO_MESSAGE = 2,
	};

protected:
	virtual void
	WillSatisfyPendingInterest(Ptr<Face> inFace, Ptr<pit::Entry> pitEntry);

	virtual bool
	DoPropagateInterest(Ptr<Face> inFace, Ptr<const Interest> interest,
			Ptr<pit::Entry> pitEntry);

	virtual void
	WillEraseTimedOutPendingInterest(Ptr<pit::Entry> pitEntry);

	virtual void
	DidReceiveValidNack(Ptr<Face> incomingFace, uint32_t nackCode,
			Ptr<const Interest> nack, Ptr<pit::Entry> pitEntry);

	virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object

	virtual void DoInitialize(void);



private:
	//Some Utils functions aboute navigation route

	/**
	 * @brief Get priority list of interest packet
	 * @param route		the navigation route which is to be compared
	 * \return priority list
	 */
	std::vector<uint32_t> GetPriorityList(const vector<string>& route);

	/**
	 * @brief Get priority list of interest packet. default route is the local node's navigation route
	 * \return priority list
	 */
	std::vector<uint32_t> GetPriorityList();

	/**
	 * @brief Get priority list of data packet from the source node
	 * @param dataName	the name of the data packet
	 * \return priority list
	 */
	std::vector<uint32_t> GetPriorityListOfDataSource(const Name& dataName);


	/**
	 * @brief	Get priority list of data packet from the forwarding node which is interested about the data
	 * 			if a node is interest about the data, it will know which node behind will interested.
	 * 			First sort the interest nodes, 	then add not interested nodes
	 * 			list structure:
	 * 			slot1	|	interest node 1 |
	 * 			slot2	|	interest node 2 |
	 * 			slop...i|...................|
	 * 			slot i+1|disinterested node1|
	 * 			slot i+2|disinterested node2|
	 * 			slot i.n|...................|
	 * @param interestNodes	the list of the interested
	 * @param recPri 		priority list from last hop
	 * \return priority list
	 */
	std::vector<uint32_t>
	GetPriorityListOfDataForwarderInterestd(
			const std::unordered_set<uint32_t>& interestNodes,
			const std::vector<uint32_t>& recPri);

	/**
	 * @brief	Get priority list of data packet from the forwarding node which is disinterested about the data
	 * 			if a node is disinterested about the data, it will NOT know which node behind will interested.
	 * 			Just Sort the neighbors by distance
	 * 			List structure:
	 * 			slot 1	|neighbor node1|
	 * 			slot 2	|neighbor node2|
	 * 			slot ...|..............|
	 *
	 *@param recPri 		priority list from last hop
	 * \return priority list
	 */
	std::vector<uint32_t> GetPriorityListOfDataForwarderDisinterestd(const std::vector<uint32_t>& recPri);

	/**
	 * @brief Get Customize data for navigation route heuristic forwarding
	 * @param type		type of Packet which is to sent
	 * @param srcPayload the original payload from src packet
	 * @param dataName  the data name of the Packet. If it is an interest packet, just give empty reference: *((Name*)NULL)
	 * \return	Payload packet
	 */
	Ptr<Packet> GetNrPayload(HeaderHelper::Type type, Ptr<const Packet> srcPayload,const Name& dataName = *((Name*)NULL));

	/**
	  * \brief	Process hello message
	  */
	void ProcessHello (Ptr<Interest> interest);

	/**
	 * \brief	Tell the direction of the Packet from
	 *          the front and behind is Only base on the moving direction of received Packet
	 * \return	(true,+value) if it is from the front
	 * 			(true,-value) if it is from behind
	 * 			(false,-1)    if it is not on the route of the packet
	 *
	*/
	pair<bool, double>
				packetFromDirection(Ptr<Interest> interest);

	/**
	 * \brief	Given a route information, this method should tell whether the received node
	 *			is on the route of the packet
	 *			It should satisfy 2 conditions:
	 *			1. the packet and the received node is moving to the same direction
	 *			2. the received node is on the route of the packet
	 *	\return	true if it is on the route of the packet
	 */
	//bool onTheRouteOfPacket(Ptr<Interest> interest);

	/**
	 * 	\brief	recognize the duplicated interest
	 *	@param	id    the node id of the interest packet
	 *	@param  seq   the nonce of the interest packet
	 *	\return	true if it is duplicated
	 */
	bool isDuplicatedInterest(uint32_t id, uint32_t nonce);

	/**
	 * 	\brief	recognize the duplicated data
	 *	@param	id    the node id of the data packet
	 *	@param  seq   the nonce of the data packet
	 *	\return	true if it is duplicated
	 */
	bool isDuplicatedData(uint32_t id, uint32_t signature);

	/**
	 * 	\brief	this designed is different from normal NDN nodes.
	 * 			Each node is selfish, and only concern about the information which may helpful for itself.
	 * 			This function will determinate whether the data will interested by the node itself,(eg, on the route of itself)
	 * 			If the data will not interested by the node itself, it will not use this data
	 * 			to satisfy the interest packet sent from other nodes to it.
	 *	@param	data    data packet
	 *	\return	Ptr<pit::Entry> if it will interested(In PIT)
	 */
	Ptr<pit::Entry>
	WillInterestedData(Ptr<const Data> data);

	/**
	 * \brief	drop the data
	 * 			Simply do nothing
	 */
	void DropPacket();

	/**
	 * \brief	drop the data
	 * 			may record the data for statistics reason
	 */
	void DropDataPacket(Ptr<Data> data);

	/**
	 * \brief	drop the interest packet
	 * 			may record the data for statistics reason
	 */
	void DropInterestePacket(Ptr<Interest> interest);

	/**
	 * 	\brief	mark the duplicated interest
	 *	@param	interest    Interest packet
	 */
	//void MarkDuplicatedInterest(Ptr<const Interest> interest);

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

	/**
	 * 	\brief	Determin whether a given location is in the front of the node
	 *	@param	lane	lane of given location
	 *	@param	pos		offset of lane of given location
	 *	@param  route   navigation route
	 *	\return	(true,distance)
	 *					 if it is in the front of the node, and along with the distance
	 *			(false,0)
	 *					otherwise
	 */
	//pair<bool, double> IsPositionInFront(const string& lane,const double &pos,const std::vector<std::string>& route);

	/**
	 * 	\brief	Determin whether a rest route from the interest packet is covered by the local node
	 * 	        eg.
	 * 	        remote route in interest packet:
	 * 	        ======.=====|=============|===========|===========|========|
	 * 	        	R1			R2			 R3				R4			R5
	 * 	        Route of local node:
	 * 	        ============|=============|======.====|===========|========|======|......
	 * 	        	R6			R7			 R3	 ^			R4			R5		R8
	 * 	        								 |=>current position of node
	 * 	        That means this node cover the rest route of the interest packet
	 *	@param	remoteRoute	the route to be compared
	 *	\return	(true) if it is covered by the local node
	 *			(false)	otherwise
	 */
	bool PitCoverTheRestOfRoute(const vector<string>& remoteRoute);


	vector<string> ExtractRouteFromName(const Name& name);

	/**
	 * 	\brief	Expire the InterestPacketTimer, if the TimerCallbackFunciton is already executed,
	 * 			it will do nothing
	 * 	@param  nodeId	node id
	 *	@param  seq		sequence number
	 */
	void ExpireInterestPacketTimer(uint32_t nodeId,uint32_t seq);

	/**
	 * 	\brief	Expire the DataPacketTimer, if the TimerCallbackFunciton is already executed,
	 * 			it will do nothing
	 * 	@param  nodeId	node id
	 * 	@param  signature node signature
	 */
	void ExpireDataPacketTimer(uint32_t nodeId,uint32_t signature);

	/**
	 * \brief	Broadcast stop message
	 * 			That is: change the nackType from NORMAL_INTEREST to NACK_LOOP,
	 * 			representing it is a Duplicated Interest
	 */
	void BroadcastStopMessage(Ptr<Interest> src);

	/**
	 * \brief	Broadcast stop message
	 * 			Stop message contains an empty priority list,
	 * 			representing it is a Duplicated Data
	 */
	void BroadcastStopMessage(Ptr<Data> src);

	/**
	 * \brief	the function which will be executed after InterestPacketTimer expire
	 */
	void ForwardInterestPacket(Ptr<Interest>);

	/**
	 * \brief	the function which will be executed after DataPacketTimer expire
	 * @param	src	the data packet received
	 * @param	newPriorityList the new priority list generated at OnData() function
	 * @param	IsClearhopCountTag  indicating that whether to reset the hop count tag to 0
	 *                              True means needs to reset to 0;
	 */
	void ForwardDataPacket(Ptr<Data> src,std::vector<uint32_t> newPriorityList,bool IsClearhopCountTag);

	/**
	 * \brief	Send the interest packet immediately,
	 * 			remember to wait a random interval before execute this function
	 */
	void SendInterestPacket(Ptr<Interest> interest);

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
	 * \brief	To be discussed: Whether to store the data packet or not?
	 * 			2015/02/19:	Temporarily do nothing.
	 */
	void ToContentStore(Ptr<Data> data);

	/**
	 * \brief	When a data packet which the node interested has been received,
	 * 			the upper layer will be notified.
	 * 													 ------------
	 * 			ForwardingStrategy<==Face::ReceiveData <-|	Face	|->Face::SendData==>other app/dev bind with face
	 * 				::OnData				/Interest	 ------------	    /Interest
	 */
	void NotifyUpperLayer(Ptr<Data> data);

private:
	typedef GreenYellowRed super;
	/**
	  * Every HelloInterval the node broadcast a  Hello message
	  */
	Time HelloInterval;
	uint32_t AllowedHelloLoss;  ///< \brief Number of hello messages which may be loss for valid link

	Timer m_htimer;			///< \brief Hello timer

	//It is use for finding a scheduled sending event for a paticular interest packet, nodeid+seq can locate a sending event
	std::map< uint32_t, std::map<uint32_t, EventId> > m_sendingInterestEvent;

	//It is use for finding a scheduled sending event for a paticular data packet, nodeid+seq can locate a sending event
	std::map< uint32_t, std::map<uint32_t, EventId> > m_sendingDataEvent;

	//when a packet is about to send, it should wait several time slot before it is actuall send
	//Default is 0.05, so the sending frequency is 20Hz
	Time m_timeSlot;

	//Provides uniform random variables.
	Ptr<UniformRandomVariable> m_uniformRandomVariable;

	Ptr<ndn::nrndn::NodeSensor> m_sensor;

	Ptr<pit::nrndn::NrPitImpl> m_nrpit; ///< \brief Reference to PIT to which this forwarding strategy is associated

	uint32_t				m_CacheSize;

	ndn::nrndn::cache::LRUCache<uint32_t,bool>
							m_interestNonceSeen;///< \brief record the randomly-genenerated bytestring that is used to detect and discard duplicate Interest messages
	ndn::nrndn::cache::LRUCache<uint32_t,bool>
							m_dataSignatureSeen;///< \brief record the signature that is used to detect and discard duplicate data messages


	vector<Ptr<Face> >      m_outFaceList;///< \brief face list(NetDeviceFace) that interest/data packet will be sent down to
	vector<Ptr<Face> >      m_inFaceList;///< \brief face list(AppFace) that interest/data packet will be sent up to

	Ptr<Node>               m_node;///< \brief the node which forwarding strategy is installed

	Time	m_offset;  	///< \brief record the last random value in scheduling the HELLO timer

	Neighbors m_nb;  ///< \brief Handle neighbors

	bool m_running;// \brief Tell if it should be running, initial is false;

	uint32_t m_runningCounter;// \brief Count how many apps are using this fw strategy. running will not stop until the counter is ZERO

	bool m_HelloLogEnable;// \brief the switch which can turn on the log on Functions about hello messages

	uint32_t m_gap;// \brief the time gap between interested nodes and disinterested nodes for sending a data packet;

	uint32_t m_TTLMax;// \brief This value indicate that when a data is received by disinterested node, the max hop count it should be forwarded

	bool NoFwStop;// \brief When the PIT covers the nodes behind, no broadcast stop message
};
} /* namespace nrndn */
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_NAVIGATION_ROUTE_HEURISTIC_FORWARDING_H_ */
