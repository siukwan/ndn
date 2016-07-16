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

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
