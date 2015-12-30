/*
 * distance-based-forwarding.cc
 *
 *  Created on: Mar 2, 2015
 *      Author: chen
 */

#include "distance-based-forwarding.h"
#include "nrHeader.h"
#include "nrUtils.h"

#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndn-data.h"
#include "ns3/core-module.h"
#include "ns3/ndn-face.h"
#include "ns3/log.h"
#include "ns3/timer.h"
#include <utility>
#include <vector>

NS_LOG_COMPONENT_DEFINE ("ndn.fw.DistanceBasedForwarding");

namespace ns3
{
namespace ndn
{
namespace fw
{
namespace nrndn
{

NS_OBJECT_ENSURE_REGISTERED (DistanceBasedForwarding);

TypeId DistanceBasedForwarding::GetTypeId()
{
	  static TypeId tid = TypeId ("ns3::ndn::fw::nrndn::DistanceBasedForwarding")
	    .SetGroupName ("Ndn")
	    .SetParent<ForwardingStrategy> ()
	    .AddConstructor<DistanceBasedForwarding>()
	    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
	   	            TimeValue (Seconds (1)),
	   	            MakeTimeAccessor (&DistanceBasedForwarding::HelloInterval),
	   	            MakeTimeChecker ())
	    .AddAttribute ("AllowedHelloLoss", "Number of hello messages which may be loss for valid link.",
	   	            UintegerValue (2),
	   	            MakeUintegerAccessor (&DistanceBasedForwarding::AllowedHelloLoss),
	   	            MakeUintegerChecker<uint32_t> ())
	    .AddAttribute ("UniformRv", "Access to the underlying UniformRandomVariable",
	      		 StringValue ("ns3::UniformRandomVariable"),
	       		 MakePointerAccessor (&DistanceBasedForwarding::m_uniformRandomVariable),
	       		 MakePointerChecker<UniformRandomVariable> ())
	    .AddAttribute ("TTLMax", "This value indicate that when a data is received by disinterested node, the max hop count it should be forwarded",
	    		         UintegerValue (3),
	    		         MakeUintegerAccessor (&DistanceBasedForwarding::m_TTLMax),
	    		         MakeUintegerChecker<uint32_t> ())
	            ;
	 return tid;
}

DistanceBasedForwarding::DistanceBasedForwarding():
		HelloInterval (Seconds (1)),
		AllowedHelloLoss (2),
		m_htimer (Timer::CANCEL_ON_DESTROY),
		m_CacheSize(5000),// Cache size can not change. Because if you change the size, the m_interestNonceSeen and m_dataNonceSeen also need to change. It is really unnecessary
		m_dataSignatureSeen(m_CacheSize),
		m_TTLMax(5),
		m_nb (HelloInterval),
		m_running(false),
		m_runningCounter(0)
{
	// TODO Auto-generated constructor stub
	m_htimer.SetFunction (&DistanceBasedForwarding::HelloTimerExpire, this);
}

void DistanceBasedForwarding::Start()
{
	NS_LOG_FUNCTION (this);
	if(!m_runningCounter)
	{
		m_running = true;
		m_offset = MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100));
		m_htimer.Schedule(m_offset);
		m_nb.ScheduleTimer();
	}
	m_runningCounter++;
}

void DistanceBasedForwarding::Stop()
{
	NS_LOG_FUNCTION (this);
	if(m_runningCounter)
		m_runningCounter--;
	else
		return;

	if(m_runningCounter)
		return;
	m_running = false;
	m_htimer.Cancel();
	m_nb.CancelTimer();
}

DistanceBasedForwarding::~DistanceBasedForwarding()
{
	// TODO Auto-generated destructor stub
}

void DistanceBasedForwarding::OnInterest(Ptr<Face> face, Ptr<Interest> interest)
{
	if(!m_running) return;

	if(HELLO_MESSAGE==interest->GetScope())
	{
		ProcessHello(interest);
	}
	else
		NS_ASSERT_MSG(false,
				"Not suppose to receive Interest Packet, except for HELLO message");

	return;
}

void DistanceBasedForwarding::OnData(Ptr<Face> face, Ptr<Data> data)
{
	if(!m_running) return;
	NS_LOG_FUNCTION (this);
	if(Face::APPLICATION & face->GetFlags())
	{
		NS_LOG_DEBUG("Get data packet from APPLICATION");
		// This is the source data from the upper node application (eg, nrProducer) of itself

		// 1.Set the payload
		Ptr<Packet> payload	= Create<Packet> (*data->GetPayload());
		ndn::nrndn::nrHeader nrheader;
		double x= m_sensor->getX();
		double y= m_sensor->getY();
		nrheader.setX(x);
		nrheader.setY(y);
		nrheader.setSourceId(m_node->GetId());
		payload->AddHeader(nrheader);
		data->SetPayload(payload);

		// 2. record the Data Packet(only record the forwarded packet)
		m_dataSignatureSeen.Put(data->GetSignature(),true);

		// 3. Then forward the data packet directly
		Simulator::Schedule(
				MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)),
				&DistanceBasedForwarding::SendDataPacket, this, data);

		// 4. Although it is from itself, include into the receive record
		NotifyUpperLayer(data);

		return;
	}

	//If the data packet has already been sent, do not proceed the packet
	if (m_dataSignatureSeen.Get(data->GetSignature()))
	{
		NS_LOG_DEBUG(
				"The Data packet has already been sent, do not proceed the packet of "<<data->GetSignature());
		return;
	}

	Ptr<const Packet> nrPayload	= data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	uint32_t nodeId=nrheader.getSourceId();
	uint32_t signature=data->GetSignature();

	if(isDuplicatedData(nodeId,signature))
	{
		ExpireDataPacketTimer(nodeId,signature);
		return;
	}

	NotifyUpperLayer(data);

	FwHopCountTag hopCountTag;
	nrPayload->PeekPacketTag(hopCountTag);
	bool isTTLReachMax = (hopCountTag.Get() > m_TTLMax);

	if (isTTLReachMax)
	{
		NS_LOG_DEBUG(
				"The Data packet has reach max TTL, do not proceed the packet of "<<data->GetSignature());
		return;
	}

	std::pair<bool, double> msgdirection = m_sensor->getDistanceWith(nrheader.getX(),
			nrheader.getY(), m_sensor->getNavigationRoute());

	Time sendInterval;
	double random = m_uniformRandomVariable->GetInteger(0, 20);
	double distance = msgdirection.second >= 0 ? msgdirection.second : -msgdirection.second;

	sendInterval=MilliSeconds(random);
	if(distance>0)
		sendInterval+=Seconds(1/distance);

	m_sendingDataEvent[nodeId][signature]=
			Simulator::Schedule(sendInterval,
			&DistanceBasedForwarding::ForwardDataPacket, this, data);
	return;
}

void DistanceBasedForwarding::AddFace(Ptr<Face> face)
{
	NS_LOG_FUNCTION(this);
	if(Face::APPLICATION==face->GetFlags())
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" add application face "<<face->GetId());
		m_inFaceList.push_back(face);
	}
	else
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" add NOT application face "<<face->GetId());
		m_outFaceList.push_back(face);
	}
}

void DistanceBasedForwarding::RemoveFace(Ptr<Face> face)
{
	NS_LOG_FUNCTION(this);
	if(Face::APPLICATION==face->GetFlags())
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" remove application face "<<face->GetId());
		m_inFaceList.erase(find(m_inFaceList.begin(),m_inFaceList.end(),face));
	}
	else
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" remove NOT application face "<<face->GetId());
		m_outFaceList.erase(find(m_outFaceList.begin(),m_outFaceList.end(),face));
	}
}

void DistanceBasedForwarding::NotifyNewAggregate()
{

	if (m_sensor == 0)
	{
		m_sensor = GetObject<ndn::nrndn::NodeSensor>();
	}

	if (m_node == 0)
	{
		m_node = GetObject<Node>();
	}

	super::NotifyNewAggregate();
}

void DistanceBasedForwarding::ForwardDataPacket(Ptr<Data> src)
{
	if(!m_running) return;
	NS_LOG_FUNCTION (this);
	uint32_t sourceId=0;
	uint32_t signature=0;
	// 1. record the Data Packet(only record the forwarded packet)
	m_dataSignatureSeen.Put(src->GetSignature(),true);

	// 2. prepare the data
	Ptr<Packet> nrPayload=src->GetPayload()->Copy();
	//Ptr<Packet> newPayload	= Create<Packet> ();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->RemoveHeader(nrheader);


	double x= m_sensor->getX();
	double y= m_sensor->getY();
	sourceId = nrheader.getSourceId();
	signature = src->GetSignature();

	// 	2.1 setup nrheader, source id do not change
	nrheader.setX(x);
	nrheader.setY(y);
	nrPayload->AddHeader(nrheader);

	// 	2.2 setup FwHopCountTag
	//FwHopCountTag hopCountTag;
	//nrPayload->PeekPacketTag( hopCountTag);
	//newPayload->AddPacketTag(hopCountTag);

	// 	2.3 copy the data packet, and install new payload to data
	Ptr<Data> data = Create<Data>(*src);
	data->SetPayload(nrPayload);

	// 3. Send the data Packet. Already wait, so no schedule
	SendDataPacket(data);

	// 4. record the forward times,转发的时候记录转发次数
	ndn::nrndn::nrUtils::IncreaseForwardCounter(sourceId,signature);
}

void DistanceBasedForwarding::SendDataPacket(Ptr<Data> data)
{
	if(!m_running) return;
	std::vector<Ptr<Face> >::iterator fit;
	for (fit = m_outFaceList.begin(); fit != m_outFaceList.end(); ++fit)
	{
		(*fit)->SendData(data);//通过各个接口发送data
		ndn::nrndn::nrUtils::AggrateDataPacketSize(data);//统计发送data的数量
	}
}

void DistanceBasedForwarding::ExpireDataPacketTimer(uint32_t nodeId,
		uint32_t signature)
{//终止时钟
	NS_LOG_FUNCTION (this<< "ExpireDataPacketTimer id\t"<<nodeId<<"\tsignature:"<<signature);
	//1. Find the waiting timer
	EventId& eventid = m_sendingDataEvent[nodeId][signature];

	//2. cancel the timer if it is still running
	eventid.Cancel();
}

bool DistanceBasedForwarding::isDuplicatedData(uint32_t id, uint32_t signature)
{//判断是否是重复的数据
	NS_LOG_FUNCTION (this);
	if(!m_sendingDataEvent.count(id))
		return false;
	else
		return m_sendingDataEvent[id].count(signature);
}

bool DistanceBasedForwarding::DoPropagateInterest(Ptr<Face> inFace,
		Ptr<const Interest> interest, Ptr<pit::Entry> pitEntry)
{
	NS_ASSERT_MSG(false,"Not suppose to propagate interest");
	return false;
}

void DistanceBasedForwarding::ProcessHello(Ptr<Interest> interest)
{
	if(!m_running) return;
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader(nrheader);
	m_nb.Update(nrheader.getSourceId(),nrheader.getX(),nrheader.getY(),Time (AllowedHelloLoss * HelloInterval));
}

void DistanceBasedForwarding::HelloTimerExpire()
{
	if(!m_running) return;
	NS_LOG_FUNCTION(this);

	SendHello();

	m_htimer.Cancel();
	Time base(HelloInterval - m_offset);
	m_offset = MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100));
	m_htimer.Schedule(base + m_offset);
}

void DistanceBasedForwarding::SendHello()
{
	if(!m_running) return;
	NS_LOG_FUNCTION(this);

	const double& x		= m_sensor->getX();
	const double& y		= m_sensor->getY();
	const string& LaneName=m_sensor->getLane();
	//1.setup name
	Ptr<Name> name = ns3::Create<Name>('/'+LaneName);

	//2. setup payload
	Ptr<Packet> newPayload	= Create<Packet> ();
	ndn::nrndn::nrHeader nrheader;
	nrheader.setX(x);
	nrheader.setY(y);
	nrheader.setSourceId(m_node->GetId());
	newPayload->AddHeader(nrheader);

	//3. setup interest packet
	Ptr<Interest> interest	= Create<Interest> (newPayload);
	interest->SetScope(HELLO_MESSAGE);	// The flag indicate it is hello message
	interest->SetName(name); //interest name is lane;

	//4. send the hello message
	SendInterestPacket(interest);
}

void DistanceBasedForwarding::SendInterestPacket(Ptr<Interest> interest)
{
	if(!m_running) return;
	NS_LOG_FUNCTION (this);

	//    if the node has multiple out Netdevice face, send the interest package to them all
	//    makde sure this is a NetDeviceFace!!!!!!!!!!!1
	vector<Ptr<Face> >::iterator fit;
	for(fit=m_outFaceList.begin();fit!=m_outFaceList.end();++fit)
	{
		(*fit)->SendInterest(interest);
		ndn::nrndn::nrUtils::AggrateInterestPacketSize(interest);
	}
}

void DistanceBasedForwarding::NotifyUpperLayer(Ptr<Data> data)
{
	if(!m_running) return;
	// 1. record the Data Packet received
	m_dataSignatureSeen.Put(data->GetSignature(),true);

	// 2. notify upper layer
	std::vector<Ptr<Face> >::iterator fit;
	for (fit = m_inFaceList.begin(); fit != m_inFaceList.end(); ++fit)
	{
		//App::OnData() will be executed,
		//including nrProducer::OnData.
		//But none of its business, just ignore
		(*fit)->SendData(data);
	}

}

} /* namespace nrndn */
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */
