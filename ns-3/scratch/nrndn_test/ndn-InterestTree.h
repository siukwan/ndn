/*
 * ndn-InterestTree.h
 *
 *  Created on: Jan 13, 2016
 *  Author       : siukwan
 */

#ifndef NDN_NR_INTERESTTREE_IMPL_H_
#define NDN_NR_INTERESTTREE_IMPL_H_

#include "ns3/object.h"
#include "ns3/ptr.h"
#include "ns3/ndn-pit.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-name.h"

#include "NodeSensor.h"

#include <vector>
#include <map>
#include <string>

namespace ns3
{
namespace ndn
{
namespace pit
{
namespace nrndn
{
using namespace std;
/**
 * 兴趣树结构
 */
//树节点
struct InterestTreeNode{
	std::map<std::string, InterestTreeNode* > child;//孩子节点
	std::map<int,bool> NodeId;//感兴趣的节点
	std::string lane;//当前节点的路段
	//两个简单的构造函数
	InterestTreeNode(std::string x) :lane(x){};
	InterestTreeNode() :lane(""){};
};

class NrInterestTreeImpl:public Object
{
public:
	static TypeId GetTypeId ();

	NrInterestTreeImpl();
	virtual ~NrInterestTreeImpl();
	void insertInterest(uint32_t&id,unsigned int pos,const std::vector<std::string>& route,InterestTreeNode* root);
	void updateNowRoot(string currentLane);
	void deleteTree(InterestTreeNode* deleteNode);

	uint32_t NodeId;
	InterestTreeNode *root;

};


}/* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_NR_INTERESTTREE_IMPL_H_ */
