/*
 * NeighborDetector.h
 *
 *  Created on: Dec 7, 2013
 *      Author: cys
 */

#ifndef NEIGHBORDETECTOR_H_
#define NEIGHBORDETECTOR_H_

#include "ns3/application.h"
#include "ns3/traced-callback.h"
#include "ns3/mobility-module.h"
#include "ns3/nstime.h"
#include "ns3/average.h"
#include "ns3/simulator.h"
#include "ns3/aodv-module.h"
#include "neighbor.h"
#include "nrcHeader.h"
#include "ClusterNode.h"
#include "NodeSensor.h"
#include <map>

namespace ns3
{
namespace nrc
{

class NeighborDetector: public Application
{
public:
	static TypeId GetTypeId(void);
	NeighborDetector();
	virtual ~NeighborDetector();

	/**
	 * Assign a fixed random variable stream number to the random variables
	 * used by this model.  Return the number of streams (possibly zero) that
	 * have been assigned.
	 *
	 * \param stream first stream index to use
	 * \return the number of stream indices assigned by this model
	 */
	int64_t AssignStreams(int64_t stream);

	void statistics(std::ofstream& out);
	static void initializeCounter();
	static void printMsgRecvCounter(std::ofstream& msgout);
	void msgCounter();

	void ScheduleEmergencyMessage(Time time);

	static bool m_flood;
	static void open(std::string path);
	static void closefile();

protected:
	virtual void DoInitialize(void);
	typedef Application super;

private:
	// inherited from Application base class.
	virtual void StartApplication(void);
	virtual void StopApplication(void);
	virtual void DoDispose(void);
	uint32_t GetApplicationId(void) const;
	void Receive(Ptr<Socket> socket);
	void Send();

	//\ Initialize the application
	void Initialize();
	//\ Send hello (unicast hello one by one)
	void SendHello();
	//\ Process hello message
	void ProcessHello(RrepHeader const & rrepHeader,
			Ipv4Address receiverIfaceAddr, nrcHeader nrcheader);
	//\ Schedule next send of hello message
	void HelloTimerExpire();

	void SendTo(Ptr<Socket> socket, Ptr<Packet> p, uint32_t flags,
			const Address &toAddress);
	void Forward(Ptr<Socket> socket, Ptr<Packet> p, uint32_t flags,
			const Address &toAddress, uint32_t seq);

	void BroadcastCarCrash(uint32_t seq);

	//\ ****Calculate RRT related******///
	//\ Calculate the rrt
	void CalculateRRT();
	//\ set the nrcHeader
	nrcHeader GetNrcHeader();
	//\ calculate stable tiem with single neighbor
	double CalculateStableTime(nrcHeader first, nrcHeader second);
	double variance(std::vector<double> alm);

	void PrintState();

	double CalculateSpeed(double lastspeed);

	//**************Neighbor Detection*******************///

	/// Initiate RERR
	void SendRerrWhenBreaksLinkToNextHop(Ipv4Address nextHop);
	/// Receive and process control packet
	void RecvControlPacket(Ptr<Socket> socket);
	/// Receive RREP
	void RecvReply(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender);
	/// Receive RREQ
	void RecvRequest(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src);

	/// Receive Emergency message
	void RecvEmergency(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src);

	//*************Handle cluster nodes receive packets************//
	void UndecideRecvRequest(Ptr<Packet> p, Ipv4Address receiver,
			Ipv4Address src);
	void ClusterHeadRecvRequest(Ptr<Packet> p, Ipv4Address receiver,
			Ipv4Address src);
	void ClusterMemberRecvRequest(Ptr<Packet> p, Ipv4Address receiver,
			Ipv4Address src);
	void FutureClusterHeadRecvRequest(Ptr<Packet> p, Ipv4Address receiver,
			Ipv4Address src);

	//*************Handle cluster nodes************//
	//   UN node
	void doUndecide();
	//   CH node
	void doClusterHead();
	//   CM node
	void doClusterMember();
	//   FCH node
	void doFutureClusterHead();
	//   change timer
	void ChangeNodeState(NodeState state);
	void FirstState();

	// UN functions
	void checkisUN();

	// CH functions
	bool insertleaveposcm(Ipv4Address& src);
	std::vector<std::string> CalculateOverlap(Ipv4Address& src);
	void checkcmexist();
	void nearintersection();
	void checkchsuitable();
	bool deleteleaveposcm(Ipv4Address& src);
	bool CheckNearIntersection();
	void FutureClusterHeadElection();
	int electfchfromset(std::vector<Ipv4Address>& v);
	void SetClusterGateWay();
	//void deleteclustermember(Ipv4Address &src);
	//void printCM();

	//CM functions
	void SendNeighborCHList();
	void NchTimerExpire();
	void RecvReplyAck(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address src);

	// FCH functions
	void insertclustermember();  //Add all neighbors to its list
	void checkisFCH();

	/// Provides uniform random variables.
	Ptr<UniformRandomVariable> m_uniformRandomVariable;

	uint32_t m_interface_id;
	/// IP protocol
	Ptr<Ipv4> m_ipv4;

	/// Request sequence number
	uint32_t m_seqNo;
	/// Wait  interval  seconds between sending each packet
	Time m_interval;

	/// Hello timer
	Timer m_htimer;

	/// nch timer, only useful for clustermember
	Timer m_nchtimer;
	/**
	 * Specifies  the number of data bytes to be sent.
	 * The default is 56, which translates into 64 ICMP data bytes when combined with the 8 bytes of ICMP header data.
	 */
	uint32_t m_size;
	Ptr<Socket> m_socket;
	//Ptr<Socket> m_unicast;
	uint16_t m_seq;
	TracedCallback<Time> m_traceRtt;
	/// produce ping-style output if true
	bool m_verbose;
	/// received packets counter
	uint32_t m_recv;
	/// Start time to report total ping time
	Time m_started;
	/// Average rtt is ms
	Average<double> m_avgRtt;
	/// Next packet will be sent
	EventId m_next;
	/// All sent but not answered packets. Map icmp seqno -> when sent
	std::map<uint16_t, Time> m_sent;

	bool EnableHello;             ///< Indicates whether a hello messages enable
	/**
	 * Every HelloInterval the node checks whether it has sent a broadcast  within the last HelloInterval.
	 * If it has not, it MAY broadcast a  Hello message
	 */
	Time HelloInterval;
	/// Handle neighbors
	Neighbors m_nb;
	uint32_t AllowedHelloLoss; ///< Number of hello messages which may be loss for valid link

	/// Raw socket per each IP interface, map socket -> iface address (IP + mask)
	std::map<Ptr<Socket>, Ipv4InterfaceAddress> m_socketAddresses;

	// Node status
	NodeState m_state;
	//\ Residual route time
	double m_rrt;

	std::vector<double> m_speedqueue;
	uint32_t m_speedqueuelen;

	//\ Reflash RRT timer
	Timer m_rrttimer;

	//\  Node function timer
	Timer m_nodetimer;
	//ClusterNode *cn;

	Timer m_printtimer;

	// Cluster node data
	NodeData *m_data;

	//Print state;
	static std::ofstream out;

	//  become max rrt node times
	uint32_t m_maxrrt_counter;

	// un counter
	uint32_t m_uncounter;

	// statistic information
	uint32_t m_messagecounter;
	double lifetime[4];

	uint32_t clustersize;
	uint32_t CHstatus;
	uint32_t CMstatus;
	uint32_t FCHstatus;
	uint32_t UNstatus;
	Time m_start;

	static std::map<uint32_t, uint32_t> forwardtimes;
	static std::map<uint32_t, uint32_t> msgRecvCounter;

	double pro;  // the probability of two car can pass an intersection together

	static uint32_t m_emergency_seqNo;

	std::set<uint32_t> m_emgmsgset;

	std::map<Ipv4Address, double> m_nb_stabletime;

	std::map<Ipv4Address, AddTimePair> m_mhpair; // record the clusterhead of a clustermember

	Ptr<NodeSensor>	m_sensor;

};

} /* namespace nrc */
} /* namespace ns3 */

#endif /* NEIGHBORDETECTOR_H_ */
