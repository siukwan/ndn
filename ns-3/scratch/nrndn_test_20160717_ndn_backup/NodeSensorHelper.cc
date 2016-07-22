/*
 * NodeSensorHelper.cc
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */


#include "NodeSensorHelper.h"
#include "NodeSensor.h"

namespace ns3
{
namespace ndn
{
namespace nrndn
{

//class NodeSensor;

NodeSensorHelper::NodeSensorHelper()
{
	// TODO Auto-generated constructor stub
	//m_sensor.SetTypeId ("ns3::ndn::nrndn::NodeSensor");
}

NodeSensorHelper::~NodeSensorHelper()
{
	// TODO Auto-generated destructor stub
}

void NodeSensorHelper::Install(Ptr<Node> node) const
{
	Ptr<Object> object = node;
	Ptr<NodeSensor> sensor = object->GetObject<NodeSensor>();
	if (sensor == 0)
	{
		sensor = m_sensor.Create()->GetObject<NodeSensor>();
		if (sensor == 0)
		{
			NS_FATAL_ERROR(
					"The requested sensor model is not a sensor model: \""<< m_sensor.GetTypeId ().GetName ()<<"\"");
		}
		object->AggregateObject(sensor);
	}
	return;
}

void NodeSensorHelper::Install(NodeContainer c) const
{
	  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
	    {
	      Install (*i);
	    }
}

void NodeSensorHelper::InstallAll(void)
{
	 Install (NodeContainer::GetGlobal ());
}

void
NodeSensorHelper::SetSensorModel (std::string type,
                                  std::string n1, const AttributeValue &v1,
                                  std::string n2, const AttributeValue &v2,
                                  std::string n3, const AttributeValue &v3,
                                  std::string n4, const AttributeValue &v4,
                                  std::string n5, const AttributeValue &v5,
                                  std::string n6, const AttributeValue &v6,
                                  std::string n7, const AttributeValue &v7,
                                  std::string n8, const AttributeValue &v8,
                                  std::string n9, const AttributeValue &v9)
{
	m_sensor.SetTypeId (type);
	m_sensor.Set (n1, v1);
	m_sensor.Set (n2, v2);
	m_sensor.Set (n3, v3);
	m_sensor.Set (n4, v4);
	m_sensor.Set (n5, v5);
	m_sensor.Set (n6, v6);
	m_sensor.Set (n7, v7);
	m_sensor.Set (n8, v8);
	m_sensor.Set (n9, v9);
}
/*
Ptr<NodeSensor> NodeSensorHelper::GetSumoNodeSensor(
		const Ptr<vanetmobility::VANETmobility>& sumodata, uint32_t id,
		Ptr<MobilityModel> mobility)
{
	Ptr<SumoNodeSensor> result;
	result = Create<SumoNodeSensor>(sumodata,id,mobility);
	return DynamicCast<vanetmobility::VANETmobility>(result);
}
*/
} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
