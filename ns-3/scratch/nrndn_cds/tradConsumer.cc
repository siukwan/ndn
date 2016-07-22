/*
 * tradConsumer.cc
 *
 *  Created on: Mar 2, 2015
 *      Author: chen
 */

#include "tradConsumer.h"

#include "nrUtils.h"
#include "nrHeader.h"
#include "NodeSensor.h"

#include "ns3/ndn-data.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("ndn.nrndn.tradConsumer");

namespace ns3
{
namespace ndn
{
namespace nrndn
{

NS_OBJECT_ENSURE_REGISTERED (tradConsumer);

tradConsumer::tradConsumer():
		m_virtualPayloadSize(0)
{
	// TODO Auto-generated constructor stub

}

TypeId tradConsumer::GetTypeId()
{
	static TypeId tid = TypeId ("ns3::ndn::nrndn::tradConsumer")
		    .SetGroupName ("Nrndn")
		    .SetParent<App> ()
		    .AddConstructor<tradConsumer> ()
		    .AddAttribute ("PayloadSize", "Virtual payload size for traffic Content packets",
		                    UintegerValue (1024),
		                    MakeUintegerAccessor (&tradConsumer::m_virtualPayloadSize),
		                    MakeUintegerChecker<uint32_t> ())
		   ;
	return tid;
}

tradConsumer::~tradConsumer()
{
	// TODO Auto-generated destructor stub
}

void tradConsumer::StartApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	super::StartApplication();
}

void tradConsumer::StopApplication()
{
	NS_LOG_FUNCTION_NOARGS ();
	super::StopApplication();
}

void tradConsumer::OnData(Ptr<const Data> data)
{
	NS_LOG_FUNCTION(this);
	Ptr<Packet> nrPayload = data->GetPayload()->Copy();
	const Name& name = data->GetName();
	nrHeader nrheader;
	nrPayload->RemoveHeader(nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	uint32_t signature = data->GetSignature();
	uint32_t packetPayloadSize = nrPayload->GetSize();

	NS_LOG_DEBUG(
			"At time "<<Simulator::Now().GetSeconds()<<":"<<m_node->GetId()
			<<"\treceived data "<<name.toUri()<<" from "<<nodeId<<"\tSignature "
			<<signature);
	NS_LOG_DEBUG("payload Size:"<<packetPayloadSize);

	NS_ASSERT_MSG(packetPayloadSize == m_virtualPayloadSize,"packetPayloadSize is not equal to "<<m_virtualPayloadSize);


	double delay = Simulator::Now().GetSeconds() - data->GetTimestamp().GetSeconds();
	nrUtils::InsertTransmissionDelayItem(nodeId,signature,delay);
	if(IsInterestData(data->GetName()))
		nrUtils::IncreaseInterestedNodeCounter(nodeId,signature);
	else
		nrUtils::IncreaseDisinterestedNodeCounter(nodeId,signature);
}

bool tradConsumer::IsInterestData(const Name& name)
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


/*
//小锟新添加
void tradConsumer::SendPacket()
{
	  if (!m_active) return;
	  cout<<"consumer.cc 发送兴趣包"<<endl;

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
	  //std::cout<<"准备出错\n";
	  ScheduleNextPacket ();
	//  std::cout<<"已经出错\n";

	  //std::cout<<"ScheduleNextPacket \n";
}

//计划下一个包
void tradConsumer::ScheduleNextPacket()
{
	//1. refresh the Interest
	std::vector<std::string> interest=GetCurrentInterest();
	std::string prefix="";
	std::vector<std::string>::reverse_iterator it;
	 for(it=interest.rbegin();it!=interest.rend();++it)
	 {
		 prefix+=*it;
	 }
	//	std::cout<<"test\n";

	 //2. set the Interest (reverse of  the residual navigation route)
	//std::cout<<prefix<<std::endl;
	if(prefix=="")
	{//兴趣为空，直接返回
		std::cout<<"ID:"<<GetNode()->GetId()<<" Prefix为空"<<std::endl;
		doConsumerCbrScheduleNextPacket();
		return;
	}
	 // if (m_firstTime)
	this->Consumer::SetAttribute("Prefix", StringValue(prefix));
	//std::cout<<"test2\n";
	NS_LOG_INFO ("Node "<<GetNode()->GetId()<<" now is interestd on "<<prefix.data());
	//std::cout<<GetNode()->GetId()<<" ";
	//getchar();
	//std::cout<<"test3\n";
	//3. Schedule next packet
	//ConsumerCbr::ScheduleNextPacket();
	doConsumerCbrScheduleNextPacket();
}

void nrConsumer::doConsumerCbrScheduleNextPacket()
{
	  if (m_firstTime)
	    {//第一次发送兴趣包
	      m_sendEvent = Simulator::Schedule (Seconds (0.0), &nrConsumer::SendPacket, this);
	      m_firstTime = false;
	    }
	  else if (!m_sendEvent.IsRunning ())
		  m_sendEvent = Simulator::Schedule (Seconds (0.0), &nrConsumer::SendPacket, this);
	   // m_sendEvent = Simulator::Schedule ((m_random == 0) ? Seconds(1.0 / m_frequency) : Seconds(m_random->GetValue ()), &nrConsumer::SendPacket, this);
}

std::vector<std::string> tradConsumer::GetCurrentInterest()
{
	std::string prefix = "/";
	std::string str;
	std::vector<std::string> result;
	Ptr<NodeSensor> sensor = this->GetNode()->GetObject<NodeSensor>();
	const std::string& currentLane = sensor->getLane();
	std::vector<std::string>::const_iterator it;
	const std::vector<std::string>& route = sensor->getNavigationRoute();


	//遍历，寻找和当前道路相同的道路，把剩余的道路加入兴趣list中
	for(it=route.begin();it!=route.end();++it)
	{
		//std::cout<<this->GetNode()->GetId()<<" "<<*it <<"\t"<<currentLane.data() <<std::endl;
		if(*it==currentLane)//一直遍历寻找到当前道路，然后把后面的压紧容器返回
			break;
	}
	//int routeSum=0;//2015.9.25  小锟添加，只对未来的10条道路有兴趣。结论：没什么作用，兴趣包数量大，主要是由于转发次数过多造成
	for(;it!=route.end();++it)
	{
		str=prefix+(*it);
		result.push_back(str);
	//	++routeSum;
	}
	return result;
}
*/

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
