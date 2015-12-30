/*
 * ClusterNode.cc
 *
 *  Created on: Dec 29, 2013
 *      Author: cys
 */

#include "ClusterNode.h"

namespace ns3
{
namespace nrc
{

NodeData::NodeData(NodeState state):ch(NULL),cm(NULL),un(NULL),fch(NULL)
{
	switch (state)
	{
	case undecide:
		un = new Undecide;
		break;
	case clusterhead:
		ch = new ClusterHead;
		break;
	case clustermember:
		cm = new ClusterMember;
		break;
	case futureclusterhead:
		fch = new FutureClusterHead;
		break;
	default:
		break;
	}
}
NodeData::~NodeData()
{
	if (ch != NULL)
		delete ch;
	if (cm != NULL)
		delete cm;
	if (un != NULL)
		delete un;
	if (fch != NULL)
		delete fch;
}


} /* namespace nrc */
} /* namespace ns3 */
