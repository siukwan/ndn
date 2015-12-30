/*
 * NodeSensor.cc
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */

#include "NodeSensor.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"

#include "ns3/log.h"
NS_LOG_COMPONENT_DEFINE ("ndn.nrndn.NodeSensor");

namespace ns3
{
namespace ndn
{
namespace nrndn
{
//NS_OBJECT_ENSURE_REGISTERED (NodeSensor);
const std::string NodeSensor::emptyType("UNKNOWN_VEHTYPE");
const std::string NodeSensor::emptyLane("UNKNOWN_LANE");

NodeSensor::NodeSensor():
		m_laneTimer(Timer::CANCEL_ON_DESTROY)
{
	// TODO Auto-generated constructor stub

	// Connect the m_lane's callback to NodeSensor::LaneChangeEvent
	m_lane.ConnectWithoutContext(MakeCallback(&NodeSensor::LaneChangeEvent,this));
	m_laneTimer.SetFunction(&NodeSensor::getLane,this);
	m_laneTimer.SetDelay(Seconds(1));
	m_laneTimer.Cancel();
	m_laneTimer.Schedule();

}

TypeId NodeSensor::GetTypeId()
{
	  static TypeId tid = TypeId ("ns3::ndn::nrndn::NodeSensor")
	    .SetGroupName ("Nrndn")
	    .SetParent<Object> ()
	   // .AddConstructor<NodeSensor> ()
        .AddTraceSource("LaneChange","Lane name of the vehicle change.",
                        MakeTraceSourceAccessor(&NodeSensor::m_laneChangeCallback))
	    ;
	  return tid;
}


NodeSensor::~NodeSensor()
{
	// TODO Auto-generated destructor stub
}

const std::string& NodeSensor::getLane()
{
	m_laneTimer.Cancel();
	m_laneTimer.Schedule();
	return emptyLane;
}

void
NodeSensor::LaneChangeEvent(std::string oldLane, std::string newLane)
{

	NS_LOG_DEBUG("Lane Change Event: oldLane="<<oldLane<<"\tNewLane="<<newLane);
	m_laneChangeCallback(oldLane,newLane);
}


} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
