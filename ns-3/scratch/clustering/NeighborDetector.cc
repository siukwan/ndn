/*
 * NeighborDetector.cc
 *
 *  Created on: Dec 7, 2013
 *      Author: cys
 */

#include "NeighborDetector.h"
#include "ns3/core-module.h"
#include "ns3/icmpv4.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/inet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/wifi-net-device.h"
#include "ns3/adhoc-wifi-mac.h"
#include <map>

#include "ClusterNode.h"
#include "NodeSensor.h"

NS_LOG_COMPONENT_DEFINE ("nrc::NeighborDetector");

namespace ns3
{
namespace nrc
{
using namespace std;

NS_OBJECT_ENSURE_REGISTERED (NeighborDetector);


ofstream NeighborDetector::out;
map<uint32_t,uint32_t> NeighborDetector::forwardtimes;
uint32_t NeighborDetector::m_emergency_seqNo =0;
map<uint32_t,uint32_t> NeighborDetector::msgRecvCounter;
bool NeighborDetector::m_flood = false;

TypeId NeighborDetector::GetTypeId(void)
{
	 static TypeId tid = TypeId ("NeighborDetector")
	    .SetParent<Application> ()
	    .AddConstructor<NeighborDetector> ()
	    .AddAttribute ("Verbose",
	                   "Produce usual output.",
	                   BooleanValue (false),
	                   MakeBooleanAccessor (&NeighborDetector::m_verbose),
	                   MakeBooleanChecker ())
	    .AddAttribute ("Interval", "Wait  interval  seconds between sending each packet.",
	                   TimeValue (Seconds (1)),
	                   MakeTimeAccessor (&NeighborDetector::m_interval),
	                   MakeTimeChecker ())
	    .AddAttribute ("Size", "The number of data bytes to be sent, real packet will be 8 (ICMP) + 20 (IP) bytes longer.",
	                   UintegerValue (56),
	                   MakeUintegerAccessor (&NeighborDetector::m_size),
	                   MakeUintegerChecker<uint32_t> (16))
	    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
	                   TimeValue (Seconds (1)),
	                   MakeTimeAccessor (&NeighborDetector::HelloInterval),
	                   MakeTimeChecker ())
	    .AddAttribute ("AllowedHelloLoss", "Number of hello messages which may be loss for valid link.",
	                   UintegerValue (2),
	                   MakeUintegerAccessor (&NeighborDetector::AllowedHelloLoss),
	                   MakeUintegerChecker<uint16_t> ())
	    .AddTraceSource ("Rtt",
	                     "The rtt calculated by the ping.",
	                     MakeTraceSourceAccessor (&NeighborDetector::m_traceRtt))
	    .AddAttribute ("UniformRv",
	                    "Access to the underlying UniformRandomVariable",
	                    StringValue ("ns3::UniformRandomVariable"),
	                    MakePointerAccessor (&NeighborDetector::m_uniformRandomVariable),
	                    MakePointerChecker<UniformRandomVariable> ());
	  return tid;
}

NeighborDetector::NeighborDetector()
		  : m_seqNo (0),
		    m_interval (Seconds (1)),
		    m_htimer (Timer::CANCEL_ON_DESTROY),
		    m_nchtimer(Timer::CANCEL_ON_DESTROY),
		    m_size (56),
		    m_socket (0),
		    m_seq (0),
		    m_verbose (false),
		    m_recv (0),
		    EnableHello (true),
		    HelloInterval (Seconds (1)),
		    m_nb (HelloInterval),
		    AllowedHelloLoss (2),
		    m_state(undecide),
		    m_rrt(0),
		    m_speedqueuelen(10),
		    m_rrttimer(Timer::CANCEL_ON_DESTROY),
		    m_nodetimer(Timer::CANCEL_ON_DESTROY),
		    m_printtimer(Timer::CANCEL_ON_DESTROY),
		    m_maxrrt_counter(0),
		    m_uncounter(0),
		    m_messagecounter(0),
		    pro(0.5162)
{
	NS_LOG_FUNCTION (this);

	if (EnableHello)
	{
	   m_nb.SetCallback (MakeCallback (&NeighborDetector::SendRerrWhenBreaksLinkToNextHop, this));
	 }

	m_uniformRandomVariable = CreateObject<UniformRandomVariable>();

	m_data=NULL;

	CHstatus=0;
	CMstatus=0;
	FCHstatus=0;
	UNstatus=0;
	clustersize=0;

	for (int i = 0; i < 4; i++)
		lifetime[i] = 0;
}

NeighborDetector::~NeighborDetector()
{
	NS_LOG_FUNCTION (this);

}

void NeighborDetector::Initialize()
{
	m_interface_id = 1; ///////////NotifyInterfaceUp (uint32_t i) its i mean the id of interface, not the id of node
	NS_LOG_FUNCTION (this << m_ipv4->GetAddress (m_interface_id, 0).GetLocal ());

	Ptr<Ipv4L3Protocol> l3 = m_ipv4->GetObject<Ipv4L3Protocol>();

	if (l3->GetNAddresses(m_interface_id) > 1)
	{
		cout<<"AODV does not work with more then one address per each interface."<<endl;
		NS_LOG_WARN ("AODV does not work with more then one address per each interface.");

	}


	Ipv4InterfaceAddress iface = l3->GetAddress(m_interface_id, 0);
//	std::cout << "The node id is " << this->GetNode()->GetId() << " IP is ";

//	iface.GetLocal().Print(std::cout);
//	std::cout << std::endl;

	if (iface.GetLocal() == Ipv4Address("127.0.0.1"))
		return;
	m_nb.setNodeadd(iface.GetLocal());
//	cout<<"a random number is "<<m_uniformRandomVariable->GetInteger(0, 100)<<endl;
	if (EnableHello)
	{
		m_htimer.SetFunction(&NeighborDetector::HelloTimerExpire, this);
		m_htimer.Schedule(
				MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)));

		m_nchtimer.SetFunction(&NeighborDetector::NchTimerExpire, this);
		m_nchtimer.Schedule(
						MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)));
	}

	m_rrttimer.SetFunction(&NeighborDetector::CalculateRRT,this);
	// Guarantee the calculation use the newest information
	m_rrttimer.Schedule(HelloInterval+Seconds(0.1));
	m_rrttimer.SetDelay(HelloInterval);

	m_start = Simulator::Now();
	FirstState();


	m_printtimer.SetFunction(&NeighborDetector::PrintState,this);
	m_printtimer.Schedule(Seconds(0.005));
	m_printtimer.SetDelay(HelloInterval);


	//TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	// Create a socket to listen only on this interface
	m_socket = Socket::CreateSocket(this->GetNode(),UdpSocketFactory::GetTypeId ());
	NS_ASSERT(m_socket != 0);
	m_socket->SetRecvCallback(MakeCallback(&NeighborDetector::Receive, this));
	m_socket->Bind(
			InetSocketAddress(iface.GetLocal(),
					PORT));
	m_socket->SetAllowBroadcast(true);
	m_socket->SetAttribute("IpTtl", UintegerValue(1));
	m_socketAddresses.insert(std::make_pair(m_socket, iface));

/*
	m_unicast = Socket::CreateSocket(this->GetNode(),UdpSocketFactory::GetTypeId ());
	NS_ASSERT(m_unicast != 0);
	m_unicast->SetRecvCallback(MakeCallback(&NeighborDetector::Receive, this));
	m_unicast->Bind(
				InetSocketAddress(iface.GetLocal(),
						PORT));
*/

	Ptr<NetDevice> dev = m_ipv4->GetNetDevice(
			m_ipv4->GetInterfaceForAddress(iface.GetLocal()));
	// Allow neighbor manager use this interface for layer 2 feedback if possible
	Ptr<WifiNetDevice> wifi = dev->GetObject<WifiNetDevice>();
	if (wifi == 0)
		return;
	Ptr<WifiMac> mac = wifi->GetMac();
	if (mac == 0)
		return;

	mac->TraceConnectWithoutContext("TxErrHeader", m_nb.GetTxErrorCallback());
	m_nb.AddArpCache(l3->GetInterface(m_interface_id)->GetArpCache());
}

void NeighborDetector::StartApplication(void)
{
	NS_LOG_FUNCTION (this);
	m_ipv4 = this->GetNode()->GetObject<Ipv4>();

	Initialize();

	if (EnableHello)
	{
		m_nb.ScheduleTimer();//The timer removes the timeout neighbors when the timer expires
	}

}

void NeighborDetector::StopApplication(void)
{
	//cout<<GetNode()->GetId()<<" stop at "<<Simulator::Now().GetSeconds()<<endl;
	lifetime[m_state]+=(Simulator::Now()-m_start).GetSeconds();
	m_htimer.Cancel();
	m_nodetimer.Cancel();
	m_rrttimer.Cancel();
	m_printtimer.Cancel();
	m_socket->Close();
	m_nb.CancelTimer();
	m_nb.Clear ();
}

void NeighborDetector::DoInitialize(void)
{
	if (m_sensor == 0)
	{
		m_sensor = m_node->GetObject<NodeSensor>();

		NS_ASSERT_MSG(m_sensor,"NeighborDetector::DoInitialize cannot find ns3::nrc::NodeSensor");
	}
	super::DoInitialize();
}

void NeighborDetector::DoDispose(void)
{
	NS_LOG_FUNCTION (this);
	m_socket = 0;
	m_ipv4 = 0;
	for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::iterator iter =
	         m_socketAddresses.begin (); iter != m_socketAddresses.end (); iter++)
	{
	   iter->first->Close ();
	}
	m_socketAddresses.clear ();
	Application::DoDispose ();
}

uint32_t NeighborDetector::GetApplicationId(void) const
{
	  NS_LOG_FUNCTION (this);
	  Ptr<Node> node = GetNode ();
	  for (uint32_t i = 0; i < node->GetNApplications (); ++i)
	    {
	      if (node->GetApplication (i) == this)
	        {
	          return i;
	        }
	    }
	  NS_ASSERT_MSG (false, "forgot to add application to node");
	  return 0; // quiet compiler
}



void NeighborDetector::Receive(Ptr<Socket> socket)
{
//	cout<<"receive"<<endl;
	this->RecvControlPacket(socket);
}

void NeighborDetector::Send()
{

}


void NeighborDetector::SendHello ()
{
//	if(this->GetNode()->GetId()==1)
//	cout<<this->GetNode()->GetId()<<" at "<<Simulator::Now().GetSeconds()<<" Send hello"<<endl;
	NS_LOG_FUNCTION (this);
	/* Broadcast a RREP with TTL = 1 with the RREP message fields set as follows:
	 *   Destination IP Address         The node's IP address.
	 *   Destination Sequence Number    The node's latest sequence number.
	 *   Hop Count                      0
	 *   Lifetime                       AllowedHelloLoss * HelloInterval
	 */
	for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
			m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)//usually only one terms
	{
		Ptr<Socket> socket = j->first;
		Ipv4InterfaceAddress iface = j->second;
		//\ reflash the state first
		RrepHeader helloHeader(/*prefix size=*/0, /*hops=*/0, /*dst=*/
				iface.GetLocal(), /*dst seqno=*/m_seqNo,
				/*origin=*/iface.GetLocal(),/*lifetime=*/
				Time(AllowedHelloLoss * HelloInterval));

		//if a clustermember send hello, its origin address contain its ch address
		if(m_state==clustermember)
		{
			helloHeader.SetOrigin(m_data->cm->ch);
			double stable = m_nb_stabletime[m_data->cm->ch];
			helloHeader.SetLifeTime(Seconds(stable));
		}
		TypeHeader tHeader(AODVTYPE_RREP);
		nrcHeader nrcheader = GetNrcHeader();
		//if(GetNode()->GetId()==1||GetNode()->GetId()==37)
		{
		//	cout<<GetNode()->GetId()<<" send velocity="<<nrcheader.getVelocity()<<" location= "<<nrcheader.getLocation()<<" at "<<Simulator::Now().GetSeconds()<<endl;

		}
		Ptr<Packet> packet = Create<Packet>();
		//\ sent the state, FILO
		packet ->AddHeader(nrcheader);
		packet->AddHeader(helloHeader);
		packet->AddHeader(tHeader);

		// Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
		Ipv4Address destination;
		if (iface.GetMask() == Ipv4Mask::GetOnes())
		{
			destination = Ipv4Address("255.255.255.255");
		}
		else
		{
			destination = iface.GetBroadcast();
		}
		socket->SendTo(packet, 0, InetSocketAddress(destination, PORT));
	}

	//cout<<this->GetNode()->GetId()<<" SendHello at "<<Simulator::Now().GetSeconds()<<endl;

}



///\name Handle link failure callback
//\{
void NeighborDetector::SendRerrWhenBreaksLinkToNextHop(Ipv4Address nextHop)
{

	//if(this->GetNode()->GetId()!=0)
	{
	//	std::cout<<this->GetNode()->GetId()<<" Find a break link of ";nextHop.Print(cout);cout<<" at time "<<Simulator::Now().GetSeconds()<<endl;
	}
//	m_nb.PrintNeighbors();

	//cout<<"state: "<<(uint32_t)m_state<<endl;
	switch(m_state)
	{
	case undecide:
	{

	}
		break;
	case clusterhead:
	{
		set<Ipv4Address>::iterator cm;
		for (cm = m_data->ch->m_cluster_memberlist.begin();
				cm != m_data->ch->m_cluster_memberlist.end();cm++)
		{
			if ((*cm) == nextHop)
			{
				set<Ipv4Address>::iterator del = cm;

	//			std::cout<<this->GetNode()->GetId()<<" Find a break link of ";nextHop.Print(cout);cout<<" at time "<<Simulator::Now().GetSeconds()<<endl;
				//cout<<"Here4"<<endl;
				m_data->ch->m_cluster_memberlist.erase(del); //Delete it from the CM list
			//	FutureClusterHeadElection();
				break;

			}
		}
	}
		break;
	case clustermember:
	{
		if(m_data->cm->ch==nextHop)
		{
			ChangeNodeState(undecide);
		}
	}
		break;
	case futureclusterhead:
	{
		vector<Ipv4Address>::iterator cm;
		for (cm = m_data->fch->m_cluster_memberlist.begin();
				cm != m_data->fch->m_cluster_memberlist.end();cm++)
		{
			if ((*cm) == nextHop)
			{
				vector<Ipv4Address>::iterator del = cm;
				m_data->fch->m_cluster_memberlist.erase(del); //Delete it from the CM list
				break;
			}
		}
	}
		break;
	default:
		NS_ASSERT_MSG(false, "there is an unknown node state");
		break;
	}
}



void NeighborDetector::RecvControlPacket(Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	Address sourceAddress;
	Ptr<Packet> packet = socket->RecvFrom(sourceAddress);
	InetSocketAddress inetSourceAddr = InetSocketAddress::ConvertFrom(
			sourceAddress);
	Ipv4Address sender = inetSourceAddr.GetIpv4();
	Ipv4Address receiver = m_socketAddresses[socket].GetLocal();
	NS_LOG_DEBUG ("AODV node " << this << " received a AODV packet from " << sender << " to " << receiver);

	//UpdateRouteToNeighbor(sender, receiver);
	TypeHeader tHeader(AODVTYPE_RREQ);
	packet->RemoveHeader(tHeader);
	if (!tHeader.IsValid())
	{
		NS_LOG_DEBUG ("AODV message " << packet->GetUid () << " with unknown type received: " << tHeader.Get () << ". Drop");
		return; // drop
	}
	switch (tHeader.Get())
	{
	case AODVTYPE_RREQ:
	{
		RecvRequest(packet, receiver, sender);
		break;
	}
	case AODVTYPE_RREP:
	{
		RecvReply(packet, receiver, sender);
		break;
	}
	case AODVTYPE_RERR:
	{
		RecvEmergency(packet, receiver, sender);
		break;
	}
	case AODVTYPE_RREP_ACK:
	{
		//RecvReplyAck(packet, receiver, sender);
		break;
	}
	}
}

void NeighborDetector::HelloTimerExpire()
{
	NS_LOG_FUNCTION (this);
	SendHello();
	m_htimer.Cancel();
	Time t = Time(0.01 * MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)));
	m_htimer.Schedule(HelloInterval - t);
}

int64_t NeighborDetector::AssignStreams(int64_t stream)
{
	NS_LOG_FUNCTION (this << stream);
	m_uniformRandomVariable->SetStream(stream);
	return 1;
}

double NeighborDetector::variance(vector<double> alm)
{
	double average=0;
	double result=0;
	vector<double>::iterator it;
	for(it=alm.begin();it!=alm.end();it++)
	{
		average+=(*it);
	}
	average = average / alm.size();
	for(it=alm.begin();it!=alm.end();it++)
	{
		result  +=((*it)-average)*((*it)-average);
	}
	result = result / alm.size();
	return result;
}

void NeighborDetector::CalculateRRT()
{
	nrcHeader neighbor;
	vector<double> alm;

	for(uint32_t i=0;i<m_nb.getNeighborNum();i++)
	{
		neighbor = m_nb.getNerghborHeader(i);
		nrcHeader myheader;
		double stable =CalculateStableTime(myheader,neighbor);
		m_nb_stabletime[m_nb.getNeighborAddress(i)]=stable;
		alm.push_back(stable);
	}

	m_rrt = variance(alm);
	//cout<<"Calculate the rrt: "<<m_rrt<<" at time "<<Simulator::Now().GetSeconds()<<" by "<<this->GetNode()->GetId()<<endl;
	m_rrttimer.Cancel();
	m_rrttimer.Schedule();

}

nrcHeader NeighborDetector::GetNrcHeader()
{
	uint32_t nbNum= m_nb.getNeighborNum();
	double speed = m_sensor->getSpeed();
	double pos   = m_sensor->getPos();
	const string& lane = m_sensor->getLane();
	double x = m_sensor->getX();
	double y = m_sensor->getY();
	const std::vector<std::string>& route=m_sensor->getNavigationRoute();
	return nrcHeader(m_state,nbNum,m_rrt,speed,pos,x,y,lane,route);
}


double NeighborDetector::CalculateStableTime(nrcHeader first,nrcHeader second)
{
	double alm = (numeric_limits<double>::max)();

	std::pair<bool, double> result =
			m_sensor->getDistanceWith(second.getX(),second.getY(),m_sensor->getNavigationRoute());
	double deltaD = result.second > 0 ? result.second : -result.second;
	double deltaV = m_sensor->getSpeed() - second.getVelocity();
	double Dist_current = deltaD;
	double Dist_previous = deltaD - deltaV;

	//if (Dist_previous < 0)
	//	Dist_previous = -Dist_previous;
	if (Dist_previous == 0 && Dist_current != 0)
		alm = (numeric_limits<double>::max)();
	else if (Dist_previous == 0 && Dist_current == 0)
		alm = 0;
	else if (Dist_current == 0)
		alm = (numeric_limits<double>::min)();
	else
	{
		//	alm = log(Dist_current / Dist_previous);
		alm = Dist_current / Dist_previous;
		if (alm < 0)
			alm = -alm;
		alm = log(alm);
	}

	return alm;

}

double NeighborDetector::CalculateSpeed(double lastspeed)
{
	vector<double>::iterator it;
	m_speedqueue.erase(m_speedqueue.begin());
	m_speedqueue.push_back(lastspeed);
	double average_speed=0;
	for(it=m_speedqueue.begin();it!=m_speedqueue.end();it++)
	{
		average_speed+=(*it);
	}
	average_speed = average_speed/(double)m_speedqueue.size();
//	m_velocity = average_speed;
	return average_speed;
}

void NeighborDetector::RecvReply(Ptr<Packet> p, Ipv4Address receiver, Ipv4Address sender)
{
	NS_LOG_FUNCTION (this << " src " << sender);
	RrepHeader rrepHeader;
	p->RemoveHeader(rrepHeader);
	nrcHeader nrcheader;
	p->RemoveHeader(nrcheader);
	Ipv4Address dst = rrepHeader.GetDst();
	NS_LOG_LOGIC ("RREP destination " << dst << " RREP origin " << rrepHeader.GetOrigin ());

	uint8_t hop = rrepHeader.GetHopCount() + 1;
	rrepHeader.SetHopCount(hop);

	//if(GetNode()->GetId()==225)
	//	cout<<"225 receive nrcheader from "<<sender<<" rrt="<<nrcheader.getRrt()<<" at "<<Simulator::Now().GetSeconds()<<endl;

	// If RREP is Hello message


	Ipv4Address ch = rrepHeader.GetOrigin();
	Ipv4Address cm = rrepHeader.GetDst();
	m_mhpair[cm].add=ch;
	m_mhpair[cm].time = rrepHeader.GetLifeTime().GetSeconds();

	if (dst == rrepHeader.GetOrigin())
	{
	// process whatever
		ProcessHello(rrepHeader, receiver,nrcheader);
		return;
	}
	else{
		ProcessHello(rrepHeader, receiver,nrcheader);
	}



}

void NeighborDetector::ProcessHello(const RrepHeader& rrepHeader,
		Ipv4Address receiverIfaceAddr,nrcHeader nrcheader)
{
	 NS_LOG_FUNCTION (this << "from " << rrepHeader.GetDst ());
	if (EnableHello)
	{
		m_nb.Update(rrepHeader.GetDst(),Time(AllowedHelloLoss * HelloInterval),nrcheader);
	}

}


void NeighborDetector::SendTo(Ptr<Socket> socket, Ptr<Packet> p, uint32_t flags,
		const Address& toAddress)
{
	socket->SendTo(p, 0, toAddress);
	m_messagecounter++;
}

void NeighborDetector::Forward(Ptr<Socket> socket, Ptr<Packet> p,
		uint32_t flags, const Address& toAddress,uint32_t seq)
{
	socket->SendTo(p, 0, toAddress);
	forwardtimes[seq]++;
	//cout<<Simulator::Now().GetSeconds()<<" forward message, forward times:"<<forwardtimes[seq]<<endl;
}

void NeighborDetector::doUndecide()
{
	map<Ipv4Address, nrcHeader> chlist;
	chlist = m_nb.GetNodelist(clusterhead);
	map<Ipv4Address, nrcHeader>::iterator chit;

	Ipv4Address  ch_choice;
	double lowestid=(numeric_limits<double>::max)();
	double chid;


	for(chit=chlist.begin();chit!=chlist.end();chit++)
	{
		chid = chit->second.getRrt();
		if(chid<lowestid)
		{
			ch_choice = chit->first;
			lowestid = chid;
		}
	}

	if(lowestid<m_rrt)
	{
		Ptr<Socket> socket = Socket::CreateSocket(this->GetNode(),UdpSocketFactory::GetTypeId ());;
		TypeHeader tHeader(AODVTYPE_RREQ);
		ClusterMessageHeader cheader(JOIN);
		Ptr<Packet> packet = Create<Packet>();
		packet->AddHeader(cheader);
		packet->AddHeader(tHeader);
		SendTo(socket,packet, 0, InetSocketAddress(ch_choice, PORT));
		m_data->un->tojoin = ch_choice;
	}
	else
	{
		std::vector<double> nbrrt = m_nb.GetRRT();
		if(m_uncounter>3)
		vector<double>	nbrrt=m_nb.GetUndecideNodeRRT();
		vector<double>::iterator i = min_element(nbrrt.begin(), nbrrt.end());
		if (i != nbrrt.end())
		{
			if (m_rrt < (*i))
			{
				m_maxrrt_counter++;	// wins 2 times become a leader;
				m_uncounter=0;
				if (m_maxrrt_counter > 2)
				{
					ChangeNodeState(clusterhead);
					m_maxrrt_counter = 0;
				}
			}
			else
			{
				m_maxrrt_counter = 0;
				m_uncounter++;
			}
		}

	}

	m_nodetimer.Cancel();
	m_nodetimer.Schedule();
}

void NeighborDetector::doClusterHead()
{
	//1.check_cm_exist
	checkcmexist();

	//2.Set Cluster GateWay
	SetClusterGateWay();

	//4.check_ch_suitable
	checkchsuitable();

	m_nodetimer.Cancel();
	m_nodetimer.Schedule();
}

void NeighborDetector::doClusterMember()
{

	if(!m_nb.IsNeighbor(m_data->cm->ch))//Lose contact
	{
		ChangeNodeState(undecide);
		return;
	}
	else
	{//CH change its state
		nrcHeader header;
		bool success=m_nb.getNerghborHeader(m_data->cm->ch,header);
		if(header.getState()!=clusterhead)
		{
		//	if(GetNode()->GetId()==251)
			{
				if(!success)cout<<"Can not find header"<<endl;
				//cout<<GetNode()->GetId()<<" change to un at "<<Simulator::Now().GetSeconds()<<endl;

			}
			ChangeNodeState(undecide);
			return;
		}
	}


	//cout<<"doClusterMember"<<endl;
	m_nodetimer.Cancel();
	m_nodetimer.Schedule();
}

void NeighborDetector::doFutureClusterHead()
{
	cout<<"doFutureClusterHead"<<endl;
	NS_ASSERT(false);
	//Request to join a cluster
	map<Ipv4Address, nrcHeader> chlist;
	chlist = m_nb.GetNodelist(clusterhead);
	map<Ipv4Address, nrcHeader>::iterator chit;

	Ipv4Address  ch_choice;
	double stable=0;
	double moststable=0;
	double ch_rrt=0;

	for(chit=chlist.begin();chit!=chlist.end();chit++)
	{
	//	stable=CalculateStableTime(GetNrcHeader(),(*chit).second);
		stable = m_nb_stabletime[chit->first];
		if(moststable<stable)
		{
			if(!m_data->fch->m_ch_blacklist.count(chit->first))
			{
				moststable = stable;
				ch_choice = chit->first;
				ch_rrt = chit->second.getRrt();
			}
			else
			{
			//	cout << chit->first << " is in FCH node "
			//			<< this->GetNode()->GetId() << "'s CH blacklist"
			//			<< endl;
			}
		}
	}
	if(moststable>0 && ch_rrt>m_rrt)
	{
		Ptr<Socket> socket = Socket::CreateSocket(this->GetNode(),UdpSocketFactory::GetTypeId ());;
		TypeHeader tHeader(AODVTYPE_RREQ);
		ClusterMessageHeader cheader(QUERY);
		FchHeader  fchheader(m_data->fch->m_cluster_memberlist);
		Ptr<Packet> packet = Create<Packet>();
		packet->AddHeader(fchheader);
		packet->AddHeader(cheader);
		packet->AddHeader(tHeader);
		SendTo(socket,packet, 0, InetSocketAddress(ch_choice, PORT));
		m_data->fch->tojoin = ch_choice;
		//Simulator::Schedule(Seconds(0.9),&NeighborDetector::checkisFCH,this);
		//if(287==GetNode()->GetId())
		//cout<<GetNode()->GetId()<<" send QUERY request to "<<ch_choice<<endl;
		m_nodetimer.Cancel();
		m_nodetimer.Schedule();
		return;
	}

	stable=0;
	moststable=0;
	ch_rrt=0;
	chlist = m_nb.GetNodelist(futureclusterhead);

	for(chit=chlist.begin();chit!=chlist.end();chit++)
	{
		stable = chit->second.getRrt();
		if (moststable < stable)
		{
			moststable = stable;
		}
	}
	if(m_rrt>=moststable)
	{
		ChangeNodeState(clusterhead);
	}

	m_nodetimer.Cancel();
	m_nodetimer.Schedule();
}

void NeighborDetector::FirstState()
{
	if(m_data!=NULL)
		delete m_data;
	m_data=NULL;
	m_state=undecide;
	UNstatus++;
	m_data =new NodeData(undecide);
	m_nodetimer.SetFunction(&NeighborDetector::doUndecide, this);
	m_start=Simulator::Now();
	m_nodetimer.Schedule(HelloInterval+HelloInterval+Seconds(0.1));
	m_nodetimer.SetDelay(HelloInterval);
}


void NeighborDetector::ChangeNodeState(NodeState state)
{
//	cout<<"m_nodetimer cancel at "<<Simulator::Now().GetSeconds()<<endl;
	m_nodetimer.Cancel();

	if(m_data!=NULL)
		delete m_data;
	m_data=NULL;

	lifetime[m_state]+=(Simulator::Now()-m_start).GetSeconds();

	m_state = state;

	switch (state)
	{
	case undecide:
	{
		UNstatus++;
		m_data =new NodeData(undecide);
		m_nodetimer.SetFunction(&NeighborDetector::doUndecide, this);
	//	cout<<GetNode()->GetId()<<"Change to undecide"<<endl;
		break;
	}
	case clusterhead:
	{
		CHstatus++;
	    m_data =new NodeData(clusterhead);
		m_nodetimer.SetFunction(&NeighborDetector::doClusterHead, this);
		//if(GetNode()->GetId()==130)
		//	cout<<GetNode()->GetId()<<"Change to clusterhead"<<endl;
		break;
	}
	case clustermember:
	{
		CMstatus++;
		m_data =new NodeData(clustermember);
		m_nodetimer.SetFunction(&NeighborDetector::doClusterMember, this);
	//	cout<<GetNode()->GetId()<<"Change to clustermember"<<endl;
		break;
	}
	case futureclusterhead:
	{
		FCHstatus++;
		m_data =new NodeData(futureclusterhead);
		insertclustermember();
		m_nodetimer.SetFunction(&NeighborDetector::doFutureClusterHead, this);
	//	cout<<GetNode()->GetId()<<"Change to futureclusterhead"<<endl;
		break;
	}
	default:
		NS_ASSERT_MSG(false, "Change to an unknown node state");
		break;
	}

	m_start=Simulator::Now();
	m_nodetimer.Schedule();
}

//-----------------------------------------------------------------------------
// UN functions
//-----------------------------------------------------------------------------
void NeighborDetector::checkisUN()
{
	if(m_state == undecide)
		m_data->un->m_ch_blacklist.insert(m_data->un->tojoin);
}


//-----------------------------------------------------------------------------
// CM functions
//-----------------------------------------------------------------------------
void NeighborDetector::SendNeighborCHList()
{

	map<Ipv4Address,double> list;
	for(uint32_t i=0;i<m_nb.getNeighborNum();i++)
	{
		Ipv4Address add = m_nb.getNeighborAddress(i);
		nrcHeader nrcheader=m_nb.getNerghborHeader(i);
		if (nrcheader.getState() == clusterhead && add != m_data->cm->ch)
		{
			double stable=0;
			if(m_nb_stabletime.count(add))
			{
				stable = m_nb_stabletime[add];
			}
			else
			{
			//	stable = CalculateStableTime(GetNrcHeader(),nrcheader);
			}
			list[add]=stable;
		}

	}

	for(uint32_t i=0;i<m_nb.getNeighborNum();i++)
	{
		Ipv4Address add = m_nb.getNeighborAddress(i);
		nrcHeader nrcheader=m_nb.getNerghborHeader(i);
		if (nrcheader.getState() == clustermember && m_mhpair[add].add != m_data->cm->ch)
		{
			if(!list.count(m_mhpair[add].add))
			{
				list[m_mhpair[add].add] =
						m_mhpair[add].time < m_nb_stabletime[add] ?
								m_mhpair[add].time : m_nb_stabletime[add];

			}
		}
	}

	if(list.size()==0)
	{
		return;
	}
//	cout<<GetNode()->GetId()<<" send nb ch list at "<<Simulator::Now().GetSeconds()<<endl;

	Ptr<Socket> socket = Socket::CreateSocket(this->GetNode(),UdpSocketFactory::GetTypeId ());;
//	TypeHeader tHeader(AODVTYPE_RREP_ACK);
	TypeHeader tHeader(AODVTYPE_RREQ);
	ClusterMessageHeader cheader(NCH);
	nchHeader ncheader(list);
	Ptr<Packet> packet = Create<Packet>();
	packet->AddHeader(ncheader);
	packet->AddHeader(cheader);
	packet->AddHeader(tHeader);
	socket->SendTo(packet, 0, InetSocketAddress(m_data->cm->ch, PORT));
}

void NeighborDetector::NchTimerExpire()
{
	NS_LOG_FUNCTION (this);
	if (m_state == clustermember)
	{
	//	cout<<"CM "<<GetNode()->GetId()<<" cancel CGW at "<<Simulator::Now().GetSeconds()<<endl;
		m_data->cm->ClusterGateWay=false;//clean CGW flag
		SendNeighborCHList();
	}
	m_nchtimer.Cancel();
	Time t = Time(
			0.01 * MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)));
	m_nchtimer.Schedule(HelloInterval - t);
}

void NeighborDetector::RecvReplyAck(Ptr<Packet> p, Ipv4Address receiver,
		Ipv4Address src)
{
	cout<<"Receive NCH"<<endl;
	ClusterMessageHeader header;
	p->RemoveHeader(header);
	if (!header.IsValid())
	{
		NS_LOG_DEBUG ("Cluster control message " << p->GetUid () << " with unknown type received: " << header.Get () << ". Drop");
		cout<<"Cluster control message " << p->GetUid () << " with unknown type received: " << header.Get () << ". Drop"<<endl;
		return;	// drop
	}
	if(m_state!=clusterhead)return;
	switch (header.Get())
	{
	case NCH:
		cout<<"here"<<endl;
		break;
	default:
		break;
	}
}
//-----------------------------------------------------------------------------
// CH functions
//-----------------------------------------------------------------------------
bool NeighborDetector::insertleaveposcm(Ipv4Address& src)
{

	vector<string> overlap = CalculateOverlap(src);

	if(overlap.empty())
	{
	//	cout<<"No overlap between "<<src<<" and "<<m_socketAddresses.begin()->second.GetLocal()<<endl;
		return false;
	}
	string end = overlap.back();
	m_data->ch->m_leave_pos_cm.insert(
			multimap<string, Ipv4Address>::value_type(end,
					src));
	return true;
}

vector<string> NeighborDetector::CalculateOverlap(Ipv4Address& src)
{
	vector<string> overlap;
	uint32_t i,j;
	const vector<string>& path1 = m_sensor->getNavigationRoute();
	nrcHeader nrcheader;
	bool success=m_nb.getNerghborHeader(src,nrcheader);
	if(!success)
	{
	//	cout<<"JOIN node not exist in neighbor list"<<endl;
		return overlap;
	}

	std::vector<string> path2 = nrcheader.getRoute();


	bool found=false;
	for (i = 0; i < path1.size(); i++)
	{
		for (j = 0; j < path2.size(); j++)
		{
			//if(m_node->GetId()==0)
			//   cout<<path1[i]<<"     "<<path2[j]<<endl;
			if(0==path1[i].compare(path2[j]))
			{
				found=true;
				break;
			}
		}
		if(found)
			break;
	}
	for(;i<path1.size()&&j<path2.size();i++,j++)
	{
		if(0==path1[i].compare(path2[j]))
			overlap.push_back(path1[i]);
		else
			break;
	}
	return overlap;
}

void NeighborDetector::checkcmexist()
{
//	m_nb.Purge();
    set<Ipv4Address>::iterator cm;
	for (cm = m_data->ch->m_cluster_memberlist.begin();
			cm != m_data->ch->m_cluster_memberlist.end(); )
	{
		set<Ipv4Address>::iterator temp = cm;
		cm++;
		bool exist = m_nb.IsNeighbor(*temp);

		if(exist)
		{
			Ipv4InterfaceAddress iface = m_socketAddresses.begin()->second;
			Ipv4Address local = iface.GetLocal();
			Ipv4Address remote= *temp;
			if(m_mhpair.count(remote)&&local!=m_mhpair[remote].add)
			{
				m_data->ch->m_cluster_memberlist.erase(temp);     //Delete it from the CM list
		     }
		}
		else
		{
			m_data->ch->m_cluster_memberlist.erase(temp); //Delete it from the CM list
		}
	}

}

void NeighborDetector::nearintersection()
{

}

void NeighborDetector::checkchsuitable()
{
	bool change = false;
	map<Ipv4Address,nrcHeader> chlist = m_nb.GetNodelist(clusterhead);

	vector<double> nbrrt;
	//vector<double> nbrrt=m_nb.GetRRT();
	map<Ipv4Address,nrcHeader>::iterator chit;
	for(chit= chlist.begin();chit!=chlist.end();chit++)
	{
		nbrrt.push_back(chit->second.getRrt());
	}
	//vector<double> nbrrt = m_nb.GetRRT();
	vector<double>::iterator i = min_element(nbrrt.begin(), nbrrt.end());
	if (i != nbrrt.end())
	{
		if (m_rrt > (*i))
		{
			{
				change=true;
				m_maxrrt_counter = 0;
			}
		}
		else
			m_maxrrt_counter = 0;
	}

	if(!m_data->ch->m_cluster_memberlist.empty())
	{
		m_data->ch->m_cmlistemptytime = Simulator::Now();
	}

	if(Simulator::Now().GetSeconds()-m_data->ch->m_cmlistemptytime.GetSeconds()>10.0)
	{
		change = true;
	}
	if(change)
	{

		ChangeNodeState(undecide);
		m_maxrrt_counter = 0;
	}
	return;
}

bool NeighborDetector::deleteleaveposcm(Ipv4Address& src)
{
	multimap<string,Ipv4Address>::iterator i;
	for(i=m_data->ch->m_leave_pos_cm.begin();i!=m_data->ch->m_leave_pos_cm.end();i++)
	{
		if((*i).second==src)
		{
			break;
		}
	}
	if(i!=m_data->ch->m_leave_pos_cm.end())
		m_data->ch->m_leave_pos_cm.erase(i);
	else
		return false;
	return true;
}

bool NeighborDetector::CheckNearIntersection()
{
	return false;
}

void NeighborDetector::FutureClusterHeadElection()
{

}



int NeighborDetector::electfchfromset(std::vector<Ipv4Address>& v)
{
	uint32_t i;
	uint32_t index=0;
	if(v.size()==1)return 0;
	//vector<double> rrt = m_nb.GetRRT();
	std::vector<Ipv4Address>::iterator it;
	double max=0;
	double rrt=0;

	for(i=0;i<v.size();i++)
	{
		rrt=m_nb.GetRRT(v[i]);
		if(rrt>max)
		{
			index=i;
			max=rrt;
		}
    }
	return index;
}

void NeighborDetector::SetClusterGateWay()
{
	//cout<<"nb  ch list size:"<<m_data->ch->m_nchlist.size()<<endl;
	map<Ipv4Address,map<double, Ipv4Address> >::iterator it;
	for(it=m_data->ch->m_nchlist.begin();it!=m_data->ch->m_nchlist.end();it++)
	{
		bool send=true;
		map<Ipv4Address,AddTimePair>::iterator k;
		for (k = m_mhpair.begin(); k != m_mhpair.end(); k++)
		{
			if (k->second.add == it->first)
			{
				send = false;
			//	if(k->first!=k->second.add)
			//	cout << "CM connect with other CH: " << it->first
			//			<< " CH connect with this CH's member " << k->first
			//			<< " it's ch:" << k->second.add << " at "
			//			<< Simulator::Now().GetSeconds() << endl;
				break;
			}
		}
		if (send)
		{
			//cout<<"Begin = "<<it->second.begin()->first<<endl;
			//cout<<"End   = "<<it->second.rbegin()->first<<endl;
			Ipv4Address cgw = it->second.rbegin()->second;// last element is good.
			Ptr<Socket> socket = Socket::CreateSocket(this->GetNode(),
					UdpSocketFactory::GetTypeId());
			TypeHeader tHeader(AODVTYPE_RREQ);
			ClusterMessageHeader cheader(CGW);
			Ptr<Packet> packet = Create<Packet>();
			packet->AddHeader(cheader);
			packet->AddHeader(tHeader);
			socket->SendTo(packet, 0, InetSocketAddress(cgw, PORT));
			//cout<<GetNode()->GetId()<<" send CGW msg to "<<cgw<<endl;
		}

	}
}


//-----------------------------------------------------------------------------
// Receive packets
//-----------------------------------------------------------------------------
void NeighborDetector::RecvRequest(Ptr<Packet> p, Ipv4Address receiver,
		Ipv4Address src)
{
	switch(m_state)
	{
	case undecide:
		UndecideRecvRequest(p,receiver,src);
		break;
	case clusterhead:
		ClusterHeadRecvRequest(p,receiver,src);
		break;
	case clustermember:
		ClusterMemberRecvRequest(p,receiver,src);
		break;
	case futureclusterhead:
		FutureClusterHeadRecvRequest(p,receiver,src);
		break;
	default:
		NS_ASSERT_MSG(false, "there is an unknown node state");
		break;
	}
}

void NeighborDetector::UndecideRecvRequest(Ptr<Packet> p, Ipv4Address receiver,
		Ipv4Address src)
{
    ClusterMessageHeader header;
    p->RemoveHeader(header);
    if(!header.IsValid())
    {
		NS_LOG_DEBUG ("Cluster control message " << p->GetUid () << " with unknown type received: " << header.Get () << ". Drop");
        return;// drop
    }
	switch (header.Get())
	{
	case ACK:
	{
		if(m_data->un->tojoin==src)
		{
		//	if(GetNode()->GetId()==130)
		//	cout<<this->GetNode()->GetId()<<" join CH "<<src<<" at "<<Simulator::Now().GetSeconds()<<endl;
		}
		else {
		//	cout<<"not match ch"<<endl;
			return;
		}
		ChangeNodeState(clustermember);
		NS_ASSERT(m_data!=NULL);
		NS_ASSERT_MSG(m_data->cm!=NULL,"Can not read cm data,not a cm?");
		m_data->cm->ch=src;
		//RecvReply(packet, receiver, sender);

		break;
	}
	default:
			break;
	}

}

void NeighborDetector::ClusterHeadRecvRequest(Ptr<Packet> p,
		Ipv4Address receiver, Ipv4Address src)
{
	ClusterMessageHeader header;
	p->RemoveHeader(header);
	if (!header.IsValid())
	{
		NS_LOG_DEBUG ("Cluster control message " << p->GetUid () << " with unknown type received: " << header.Get () << ". Drop");
		return;	// drop
	}
	switch (header.Get())
	{
	case JOIN:
	{
		if (!insertleaveposcm(src))
			return;

		NS_ASSERT_MSG(m_data->ch!=NULL, "Can not read cm data,not a CH?");

		/*
		if(GetNode()->GetId()==251)
		{
			cout << GetNode()->GetId() << " receive JOIN from " << src
					<< " CM list before: size="
					<< m_data->ch->m_cluster_memberlist.size() << endl;
			set<Ipv4Address>::iterator i;
			for (i = m_data->ch->m_cluster_memberlist.begin();
				i != m_data->ch->m_cluster_memberlist.end(); i++)
					cout<<(*i)<<"  ";
			cout<<endl;
		}
		*/

		m_data->ch->m_cluster_memberlist.insert(src);

		FutureClusterHeadElection();

		Ptr<Socket> socket = Socket::CreateSocket(this->GetNode(),
				UdpSocketFactory::GetTypeId());
		TypeHeader tHeader(AODVTYPE_RREQ);
		ClusterMessageHeader cheader(ACK);
		Ptr<Packet> packet = Create<Packet>();
		packet->AddHeader(cheader);
		packet->AddHeader(tHeader);
		SendTo(socket,packet, 0, InetSocketAddress(src, PORT));
/*
		if (GetNode()->GetId() == 251)
		{
			cout << GetNode()->GetId() << " receive JOIN from " << src
					<< " CM list after: " << endl;
			set<Ipv4Address>::iterator i;
			for (i = m_data->ch->m_cluster_memberlist.begin();
					i != m_data->ch->m_cluster_memberlist.end(); i++)
				cout << (*i) << "  ";
			cout << endl;
		}
*/
		break;
	}
	case QUERY:
	{
		NS_ASSERT(false);
		if (!insertleaveposcm(src))
			return;
	//	cout << this->GetNode()->GetId() << " receive QUERY from " << src
	//			<< endl;
		NS_ASSERT_MSG(m_data->ch!=NULL, "Can not read cm data,not a CH?");
		m_data->ch->m_cluster_memberlist.insert(
				set<Ipv4Address>::value_type(src));

		FchHeader  fchheader;
	//	vector<Ipv4Address> grantlist;
		p->RemoveHeader(fchheader);
		vector<Ipv4Address>::const_iterator it;
		for(it=fchheader.getMemberlist().begin();it!=fchheader.getMemberlist().end();it++)
		{
			Ipv4Address add = (*it);
			if(insertleaveposcm(add))
			{
				m_data->ch->m_cluster_memberlist.insert(
							set<Ipv4Address>::value_type(*it));
		//		grantlist.push_back(add);
			}
		}

		Ptr<Socket> socket = Socket::CreateSocket(this->GetNode(),
				UdpSocketFactory::GetTypeId());
	//	FchHeader  grantheader;
	//	fchheader.setMemberlist(grantlist);
		TypeHeader tHeader(AODVTYPE_RREQ);
		ClusterMessageHeader cheader(RESPONSE);
		Ptr<Packet> packet = Create<Packet>();
	//	packet->AddHeader(grantheader);
		packet->AddHeader(cheader);
		packet->AddHeader(tHeader);
		SendTo(socket,packet, 0, InetSocketAddress(src, PORT));
		break;
	}
	case NCH:
	{
//		cout<<"Receive NCH"<<endl;
		nchHeader ncheader;
		p->RemoveHeader(ncheader);
		map<Ipv4Address, double> list = ncheader.getNch();
		map<Ipv4Address, double>::iterator it;
		for(it=list.begin();it!=list.end();it++)
		{
			if(!m_nb.IsNeighbor(it->first))
			{
				double stable =
					it->second < m_nb_stabletime[src] ?
							it->second : m_nb_stabletime[src];
			m_data->ch->m_nchlist[it->first][stable]=src;
			}
		}
		break;
	}

	default:
		break;
	}
}

void NeighborDetector::ClusterMemberRecvRequest(Ptr<Packet> p,
		Ipv4Address receiver, Ipv4Address src)
{
	ClusterMessageHeader header;
	p->RemoveHeader(header);
	if (!header.IsValid())
	{
		NS_LOG_DEBUG ("Cluster control message " << p->GetUid () << " with unknown type received: " << header.Get () << ". Drop");
		return;			// drop
	}

	if (src!=m_data->cm->ch)
	{
		NS_LOG_DEBUG ("Clusterhead control message from" << src << " is not belong to the CH of this CM: " << m_data->cm->ch << ". Drop");
	//	cout<<"Clusterhead control message from" << src << " is not belong to the CH of this CM: " << m_data->cm->ch << ". Drop"<<endl;
		return;
	}
	switch (header.Get())
	{
	case FCH:
	{
		RrepHeader rrepHeader;
		p->RemoveHeader(rrepHeader);

		for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
				m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)//usually only one terms
		{
			Ptr<Socket> socket = j->first;
			Ipv4InterfaceAddress iface = j->second;
			if(rrepHeader.GetDst()==iface.GetLocal())
			{
				Ipv4Address oldch = m_data->cm->ch;
				ChangeNodeState(futureclusterhead);
				m_data->fch->m_ch_blacklist.insert(oldch);//make sure it will not join back
				break;
			}
			else
			{
				m_data->cm->ch = rrepHeader.GetDst();
			}
		}
		break;
	}
	case CGW:
	{
		m_data->cm->ClusterGateWay=true;
	//	cout<<"CM "<<GetNode()->GetId()<<" become CGW at "<<Simulator::Now().GetSeconds()<<endl;
		break;
	}
	default:
		break;
	}
}

void NeighborDetector::FutureClusterHeadRecvRequest(Ptr<Packet> p,
		Ipv4Address receiver, Ipv4Address src)
{
	NS_ASSERT(false);
	ClusterMessageHeader header;
	p->RemoveHeader(header);
	if (!header.IsValid())
	{
		NS_LOG_DEBUG ("Cluster control message " << p->GetUid () << " with unknown type received: " << header.Get () << ". Drop");
		return;			// drop
	}
	switch (header.Get())
	{
	case RESPONSE:
	{
		// Inform other CM nodes
//		FchHeader grantheader;
//		p->RemoveHeader(grantheader);

		for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
						m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)//usually only one terms
		{
			Ptr<Socket> socket = j->first;
			Ipv4InterfaceAddress iface = j->second;
			RrepHeader rrepHeader(/*prefix size=*/0, /*hops=*/0, /*dst=*/
							src, /*dst seqno=*/m_seqNo,
							/*origin=*/iface.GetLocal(),/*lifetime=*/
							Time(AllowedHelloLoss * HelloInterval));

			TypeHeader tHeader(AODVTYPE_RREQ);
			ClusterMessageHeader cheader(FCH);
			Ptr<Packet> packet = Create<Packet>();
			packet->AddHeader(rrepHeader);
			packet->AddHeader(cheader);
			packet->AddHeader(tHeader);
			Ipv4Address destination;
			if (iface.GetMask() == Ipv4Mask::GetOnes())
			{
				destination = Ipv4Address("255.255.255.255");
			}
			else
			{
				destination = iface.GetBroadcast();
			}
			SendTo(socket,packet, 0, InetSocketAddress(destination, PORT));
		}

		ChangeNodeState(clustermember);
		NS_ASSERT(m_data!=NULL);
		NS_ASSERT_MSG(m_data->cm!=NULL,"Can not read cm data,not a cm?");
		m_data->cm->ch=src;
		break;
	}
	default:
		break;
	}
}



//-----------------------------------------------------------------------------
// FCH functions
//-----------------------------------------------------------------------------
void NeighborDetector::insertclustermember()
{
	uint32_t size = m_nb.getNeighborNum();
	for(uint32_t i =0;i<size;i++)
	{
		Ipv4Address address = m_nb.getNeighborAddress(i);
		//m_data->fch->m_cluster_memberlist.insert(
			//	set<Ipv4Address>::value_type(address));
		m_data->fch->m_cluster_memberlist.push_back(address);
	}
}



void NeighborDetector::checkisFCH()
{
	if(m_state == futureclusterhead)
		m_data->fch->m_ch_blacklist.insert(m_data->fch->tojoin);
}


//-----------------------------------------------------------------------------
// Print state functions
//-----------------------------------------------------------------------------

void NeighborDetector::open(string path)
{
	out.open(path.data(),ios::out);
}

void NeighborDetector::closefile()
{
	out.close();
}

void NeighborDetector::PrintState()
{
	uint32_t nodeId = GetNode()->GetId ();
	Ptr<MobilityModel> mobModel = GetNode()->GetObject<MobilityModel> ();
	Vector3D pos = mobModel->GetPosition ();
	switch (m_state)
	{
	case undecide:
	{
		out << "At " << Simulator::Now().GetSeconds() << " node " << nodeId
				<< ": Position(" << pos.x << ", " << pos.y << ", " << pos.z
				<< "); State:  UN, finding CH! " << std::endl;
		break;
	}
	case clusterhead:
	{
		out << "At " << Simulator::Now().GetSeconds() << " node " << nodeId
				<< ": Position(" << pos.x << ", " << pos.y << ", " << pos.z
				<< "); State:  CH, CM list: ";
		set<Ipv4Address>::iterator i;
		for(i=m_data->ch->m_cluster_memberlist.begin();i!=m_data->ch->m_cluster_memberlist.end();i++)
			out<<(*i)<<"  ";
		out<<endl;
		clustersize+=m_data->ch->m_cluster_memberlist.size()+1;
		break;
	}
	case clustermember:
	{
		  out << "At " << Simulator::Now ().GetSeconds () << " node " << nodeId
		            << ": Position(" << pos.x << ", " << pos.y << ", " << pos.z
		            << "); State:  CM, belonging to CH "<<m_data->cm->ch <<std::endl;
		break;
	}
	case futureclusterhead:
	{
		  out << "At " << Simulator::Now ().GetSeconds () << " node " << nodeId
		            << ": Position(" << pos.x << ", " << pos.y << ", " << pos.z
		            << "); State: FCH! CM list: ";
		vector<Ipv4Address>::iterator i;
		for(i=m_data->fch->m_cluster_memberlist.begin();i!=m_data->fch->m_cluster_memberlist.end();i++)
			out<<(*i)<<"  ";
		out<<endl;
		clustersize+=m_data->fch->m_cluster_memberlist.size()+1;
		break;
	}
	default:
		NS_ASSERT_MSG(false, "Change to an unknown node state");
		break;
	}
	m_printtimer.Cancel();
	m_printtimer.Schedule();
}

void NeighborDetector::statistics(std::ofstream& out)
{
	double averageclustersize=0;
	if((lifetime[clusterhead]+lifetime[futureclusterhead])!=0)
		averageclustersize=((double)clustersize)/(lifetime[clusterhead]+lifetime[futureclusterhead]);

    out<<GetNode()->GetId()<<"	"
    	<<m_stopTime.GetSeconds()-m_startTime.GetSeconds()<<"	"
    	<<lifetime[clusterhead]<<"	"
    	<<CHstatus<<"	"
    	<<averageclustersize<<"	"
    	<<lifetime[clustermember]<<"	"
    	<<CMstatus<<"	"
    	<<lifetime[undecide]<<"	"
    	<<UNstatus<<"	"
    	<<lifetime[futureclusterhead]<<"	"
    	<<FCHstatus<<"	"
    	<<(CHstatus+CMstatus+UNstatus+FCHstatus)<<"	"
        <<m_messagecounter
    	<<endl;
}

void NeighborDetector::BroadcastCarCrash(uint32_t seq)
{
	if(!forwardtimes.count(seq))
	{
		forwardtimes[seq]=0;// first time
	//	cout<<GetNode()->GetId()<<" broadcast emergency message, sequence num: "<<seq<<" at "<<Simulator::Now().GetSeconds()<<endl;
	}
	for (std::map<Ptr<Socket>, Ipv4InterfaceAddress>::const_iterator j =
			m_socketAddresses.begin(); j != m_socketAddresses.end(); ++j)//usually only one terms
	{
		Ptr<Socket> socket = j->first;
		Ipv4InterfaceAddress iface = j->second;
		RrepHeader EmergencyHeader(/*prefix size=*/0, /*hops=*/0, /*dst=*/
				iface.GetLocal(), /*dst seqno=*/seq,
				/*origin=*/iface.GetLocal(),/*lifetime=*/
				Time(AllowedHelloLoss * HelloInterval));

		TypeHeader tHeader(AODVTYPE_RERR);
		Ptr<Packet> packet = Create<Packet>();
		//\ sent the state, FILO
		packet->AddHeader(EmergencyHeader);
		packet->AddHeader(tHeader);

		// Send to all-hosts broadcast if on /32 addr, subnet-directed otherwise
		Ipv4Address destination;
		if (iface.GetMask() == Ipv4Mask::GetOnes())
		{
			destination = Ipv4Address("255.255.255.255");
		}
		else
		{
			destination = iface.GetBroadcast();
		}
		Time t = Time(MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)));
		Simulator::Schedule(t,&NeighborDetector::Forward,this,socket,packet,0,InetSocketAddress(destination, PORT),seq);
	//	socket->SendTo(packet, 0, InetSocketAddress(destination, PORT));

	}
}

void NeighborDetector::RecvEmergency(Ptr<Packet> p, Ipv4Address receiver,
		Ipv4Address src)
{

	RrepHeader rrepHeader;
	p->RemoveHeader(rrepHeader);
	if (m_flood)
	{
		if (!m_emgmsgset.count(rrepHeader.GetDstSeqno()))
		{
			//	cout<<GetNode()->GetId()<<" receive emergency message, seqNo: "<<rrepHeader.GetDstSeqno()<<endl;
			m_emgmsgset.insert(rrepHeader.GetDstSeqno());
			BroadcastCarCrash(rrepHeader.GetDstSeqno());
		}
	}
	else
	{
		if (!m_emgmsgset.count(rrepHeader.GetDstSeqno()))
		{
			//	cout<<GetNode()->GetId()<<" receive emergency message, seqNo: "<<rrepHeader.GetDstSeqno()<<endl;
			m_emgmsgset.insert(rrepHeader.GetDstSeqno());
			if (m_state == clusterhead || m_state == futureclusterhead
					|| m_state == undecide)
			{
				BroadcastCarCrash(rrepHeader.GetDstSeqno());
			}
			else if (m_state == clustermember)
			{
				if (m_data->cm->ClusterGateWay)
					BroadcastCarCrash(rrepHeader.GetDstSeqno());
			}
		}
	}
}
void NeighborDetector::msgCounter()
{
	for(uint32_t i=0;i<m_emergency_seqNo;i++)
	{
		if(m_emgmsgset.count(i))
			msgRecvCounter[i]++;
	}
}

void NeighborDetector::initializeCounter()
{
	for(uint32_t i=0;i<m_emergency_seqNo;i++)
		msgRecvCounter[i]=0;
}

void NeighborDetector::printMsgRecvCounter(ofstream& msgout)
{
	map<uint32_t,uint32_t>::iterator it;
	for(it = forwardtimes.begin();it!=forwardtimes.end();it++)
		msgout<<"Message Seq:"<<it->first<<"\tMessage forward times: "<<it->second<<endl;

	for(it = msgRecvCounter.begin();it!=msgRecvCounter.end();it++)
		msgout<<"Message Seq:"<<it->first<<"\tMessage receives times: "<<it->second<<endl;
}

void NeighborDetector::ScheduleEmergencyMessage(Time time)
{
	Simulator::Schedule (time, &NeighborDetector::BroadcastCarCrash,this,m_emergency_seqNo++);
}


} /* namespace nrc */
} /* namespace ns3 */


