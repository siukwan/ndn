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
#include <queue>
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
class InterestTreeNode{
public:
	map<string, InterestTreeNode* > child;//孩子节点
	map<int,bool> NodeId;//感兴趣的节点
	string lane;//当前节点的路段
	//两个简单的构造函数
	InterestTreeNode(string x) :lane(x){};
	InterestTreeNode() :lane(""){};
};

class NrInterestTreeImpl:public Object
{
public:
	static TypeId GetTypeId ();

	         NrInterestTreeImpl();
	virtual ~NrInterestTreeImpl();
	void    insertInterest(uint32_t&id,unsigned int pos,const std::vector<string>& route,InterestTreeNode* root);
	void    updateNowRoot(string currentLane);
	void    deleteTree(InterestTreeNode* deleteNode);
	void    levelOrder();
	void    MergeInterest(uint32_t&id,unsigned int pos,const vector<string>& oldRoute,string curLane,bool&flag);
	string  uriConvertToString(std::string str);
	string  serialize();
	InterestTreeNode* deserialize(string serializeTree);
	InterestTreeNode* levelOrderDelete(string curLane);

	uint32_t NodeId;
	string prefix;//前缀，一般为"/"
	InterestTreeNode *root;
	Ptr<ndn::nrndn::NodeSensor>	m_sensor;

};


}/* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_NR_INTERESTTREE_IMPL_H_ */
