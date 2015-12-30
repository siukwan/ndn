/*
 * nrConsumer.cc
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */

#include "ns3/core-module.h"
#include "ns3/log.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndn-app.h"
#include "ns3/ndn-face.h"

#include "ndn-navigation-route-heuristic-forwarding.h"
#include "nrConsumer.h"
#include "NodeSensor.h"
#include "nrHeader.h"
#include "nrUtils.h"

NS_LOG_COMPONENT_DEFINE ("ndn.nrndn.nrConsumer");

namespace ns3
{
namespace ndn
{
namespace nrndn
{
NS_OBJECT_ENSURE_REGISTERED (nrConsumer);


TypeId nrConsumer::GetTypeId()
{
	static TypeId tid = TypeId ("ns3::ndn::nrndn::nrConsumer")
		    .SetGroupName ("Nrndn")
		    .SetParent<ConsumerCbr> ()
		    .AddConstructor<nrConsumer> ()
//		    .AddAttribute("sensor", "The vehicle sensor used by the nrConsumer.",
//		    	   	    		PointerValue (),
//		    	   	    		MakePointerAccessor (&nrConsumer::m_sensor),
//		    	   	    		MakePointerChecker<ns3::ndn::nrndn::NodeSensor> ())
		    .AddAttribute ("PayloadSize", "Virtual payload size for traffic Content packets",
		    		            UintegerValue (1024),
		    	                MakeUintegerAccessor (&nrConsumer::m_virtualPayloadSize),
		    		            MakeUintegerChecker<uint32_t> ())
		    ;
		  return tid;
}

nrConsumer::nrConsumer():
		m_virtualPayloadSize(1024)
{
	// TODO Auto-generated constructor stub

}

nrConsumer::~nrConsumer()
{
	// TODO Auto-generated destructor stub
}

void nrConsumer::StartApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_forwardingStrategy->Start();
	// retransmission timer expiration is not necessary here, just cancel the event
	//m_retxEvent.Cancel ();
	super::StartApplication();
}

void nrConsumer::StopApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	m_forwardingStrategy->Stop();
	super::StopApplication();
}

void nrConsumer::ScheduleNextPacket()
{
	//1. Reflash the Interest
	 std::vector<std::string> interest=GetCurrentInterest();
	 std::string prefix;

	 std::vector<std::string>::reverse_iterator it;
	 for(it=interest.rbegin();it!=interest.rend();++it)
	 {
		 prefix+=*it;
	 }

	 //2. set the Interest (reverse of  the residual navigation route)
	this->Consumer::SetAttribute("Prefix", StringValue(prefix));
	NS_LOG_INFO ("Node "<<GetNode()->GetId()<<" now is interestd on "<<prefix.data());

	//3. Schedule next packet
	//ConsumerCbr::ScheduleNextPacket();
	doConsumerCbrScheduleNextPacket();
}

std::vector<std::string> nrConsumer::GetCurrentInterest()
{
	std::string prefix = "/";
	std::string str;
	std::vector<std::string> result;
	Ptr<NodeSensor> sensor = this->GetNode()->GetObject<NodeSensor>();
	const std::string& currentLane = sensor->getLane();
	std::vector<std::string>::const_iterator it;
	const std::vector<std::string>& route = sensor->getNavigationRoute();

	for(it=route.begin();it!=route.end();++it)
	{
		//std::cout<<this->GetNode()->GetId()<<" "<<*it <<"\t"<<currentLane.data() <<std::endl;
		if(*it==currentLane)
			break;
	}
	for(;it!=route.end();++it)
	{
		str=prefix+(*it);
		result.push_back(str);
	}
	return result;
}


void nrConsumer::doConsumerCbrScheduleNextPacket()
{
	  if (m_firstTime)
	    {
	      m_sendEvent = Simulator::Schedule (Seconds (0.0),
	                                         &nrConsumer::SendPacket, this);
	      m_firstTime = false;
	    }
	  else if (!m_sendEvent.IsRunning ())
	    m_sendEvent = Simulator::Schedule (
	                                       (m_random == 0) ?
	                                         Seconds(1.0 / m_frequency)
	                                       :
	                                         Seconds(m_random->GetValue ()),
	                                       &nrConsumer::SendPacket, this);
}



void nrConsumer::SendPacket()
{
	  if (!m_active) return;

	  NS_LOG_FUNCTION_NOARGS ();

	  uint32_t seq=std::numeric_limits<uint32_t>::max (); //invalid

	  if (m_seqMax != std::numeric_limits<uint32_t>::max())
	  {
		if (m_seq >= m_seqMax)
		{
			return; // we are totally done
		}
	  }

	  seq = m_seq++;

	  Ptr<Name> nameWithSequence = Create<Name> (m_interestName);
	  nameWithSequence->appendSeqNum (seq);

	  Ptr<Interest> interest = Create<Interest> ();
	  interest->SetNonce               (m_rand.GetValue ());
	  interest->SetName                (nameWithSequence);
	  interest->SetInterestLifetime    (m_interestLifeTime);

	  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
	  NS_LOG_INFO ("> Interest for " <<nameWithSequence->toUri()<<" seq "<< seq);

	  //WillSendOutInterest (seq);

	  FwHopCountTag hopCountTag;
	  interest->GetPayload ()->AddPacketTag (hopCountTag);

	  m_transmittedInterests (interest, this, m_face);
	  m_face->ReceiveInterest (interest);

	  ScheduleNextPacket ();
}


/*
Ptr<Packet> nrConsumer::GetNrPayload()
{
	Ptr<Packet> nrPayload	= Create<Packet> ();
	Ptr<Node> node			= GetNode();
	Ptr<NodeSensor> sensor	= node->GetObject<NodeSensor>();
	Ptr<ns3::ndn::fw::nrndn::NavigationRouteHeuristic> fwStrategy
							= node->GetObject<ns3::ndn::fw::nrndn::NavigationRouteHeuristic>();
     NS_ASSERT_MSG (fwStrategy!=0,"nrConsumer::GetNrPayload: "
    		 "ns3::ndn::fw::NavigationRouteHeuristic should be installed to a node "
    		 "while using ns3::ndn::nrndn::nrConsumer");

	double position			= sensor->getPos();
	std::string lane 		= sensor->getLane();
	std::vector<uint32_t> priorityList=fwStrategy->GetPriorityList();

	nrHeader nrheader(node->GetId(),position,lane,priorityList);
	nrPayload->AddHeader(nrheader);
	return nrPayload;
}
*/

void nrConsumer::OnData(Ptr<const Data> data)
{
	NS_LOG_FUNCTION (this);
	Ptr<Packet> nrPayload	= data->GetPayload()->Copy();
	const Name& name = data->GetName();
	nrHeader nrheader;
	nrPayload->RemoveHeader(nrheader);
	uint32_t nodeId=nrheader.getSourceId();
	uint32_t signature=data->GetSignature();
	uint32_t packetPayloadSize = nrPayload->GetSize();

	NS_LOG_DEBUG("At time "<<Simulator::Now().GetSeconds()<<":"<<m_node->GetId()<<"\treceived data "<<name.toUri()<<" from "<<nodeId<<"\tSignature "<<signature<<"\t forwarded by("<<nrheader.getX()<<","<<nrheader.getY()<<")");
	NS_LOG_DEBUG("payload Size:"<<packetPayloadSize);

	NS_ASSERT_MSG(packetPayloadSize == m_virtualPayloadSize,"packetPayloadSize is not equal to "<<m_virtualPayloadSize);

	double delay = Simulator::Now().GetSeconds() - data->GetTimestamp().GetSeconds();
	nrUtils::InsertTransmissionDelayItem(nodeId,signature,delay);
	if(IsInterestData(data->GetName()))
		nrUtils::IncreaseInterestedNodeCounter(nodeId,signature);
	else
		nrUtils::IncreaseDisinterestedNodeCounter(nodeId,signature);
	//NS_LOG_UNCOND("At time "<<Simulator::Now().GetSeconds()<<":"<<m_node->GetId()<<"\treceived data "<<name.toUri()<<" from "<<nodeId<<"\tSignature "<<signature);
}

void nrConsumer::NotifyNewAggregate()
{
  super::NotifyNewAggregate ();
}

void nrConsumer::DoInitialize(void)
{
	if (m_forwardingStrategy == 0)
	{
		//m_forwardingStrategy = GetObject<fw::nrndn::NavigationRouteHeuristic>();
		Ptr<ForwardingStrategy> forwardingStrategy=m_node->GetObject<ForwardingStrategy>();
		NS_ASSERT_MSG(forwardingStrategy,"nrConsumer::DoInitialize cannot find ns3::ndn::fw::ForwardingStrategy");
		m_forwardingStrategy = DynamicCast<fw::nrndn::NavigationRouteHeuristic>(forwardingStrategy);
	}
	if (m_sensor == 0)
	{
		m_sensor =  m_node->GetObject<ndn::nrndn::NodeSensor>();
		NS_ASSERT_MSG(m_sensor,"nrConsumer::DoInitialize cannot find ns3::ndn::nrndn::NodeSensor");
	}
	super::DoInitialize();
}

void nrConsumer::OnTimeout(uint32_t sequenceNumber)
{
	return;
}

void nrConsumer::OnInterest(Ptr<const Interest> interest)
{
	NS_ASSERT_MSG(false,"nrConsumer should not be supposed to "
			"receive Interest Packet!!");
}

bool nrConsumer::IsInterestData(const Name& name)
{
	std::vector<std::string> result;
	Ptr<NodeSensor> sensor = this->GetNode()->GetObject<NodeSensor>();
	const std::string& currentLane = sensor->getLane();
	std::vector<std::string>::const_iterator it;
	std::vector<std::string>::const_iterator it2;
	const std::vector<std::string>& route = sensor->getNavigationRoute();

	it =std::find(route.begin(),route.end(),currentLane);

	it2=std::find(it,route.end(),name.get(0).toUri());

	return (it2!=route.end());
}

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
