/*
 * NodeSensorHelper.h
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */

#ifndef NODESENSORHELPER_H_
#define NODESENSORHELPER_H_

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/application.h"

namespace ns3
{
namespace ndn
{
namespace nrndn
{

class NodeSensorHelper
{
public:
	NodeSensorHelper();
	virtual ~NodeSensorHelper();

	void Install(Ptr<Node> node) const;
	void Install(NodeContainer container) const;

	/**
	 * Perform the work of NodeSensorHelper::Install on _all_ nodes which
	 * exist in the simulation.
	 */
	void InstallAll(void);

	void SetSensorModel (std::string type,
	                           std::string n1 = "", const AttributeValue &v1 = EmptyAttributeValue (),
	                           std::string n2 = "", const AttributeValue &v2 = EmptyAttributeValue (),
	                           std::string n3 = "", const AttributeValue &v3 = EmptyAttributeValue (),
	                           std::string n4 = "", const AttributeValue &v4 = EmptyAttributeValue (),
	                           std::string n5 = "", const AttributeValue &v5 = EmptyAttributeValue (),
	                           std::string n6 = "", const AttributeValue &v6 = EmptyAttributeValue (),
	                           std::string n7 = "", const AttributeValue &v7 = EmptyAttributeValue (),
	                           std::string n8 = "", const AttributeValue &v8 = EmptyAttributeValue (),
	                           std::string n9 = "", const AttributeValue &v9 = EmptyAttributeValue ());
private:
	 ObjectFactory m_sensor;
};

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NODESENSORHELPER_H_ */
