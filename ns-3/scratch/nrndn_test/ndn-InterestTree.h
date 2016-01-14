/*
 * ndn-InterestTree.h
 *
 *  Created on: Jan 13, 2016
 *  Author       : siukwan
 */

#ifndef NDN_NR_INTERESTTREE_IMPL_H_
#define NDN_NR_INTERESTTREE_IMPL_H_

#include "ns3/ndn-pit.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-name.h"

#include "NodeSensor.h"

#include <vector>


namespace ns3
{
namespace ndn
{
namespace pit
{
namespace nrndn
{

/**
 * 兴趣树结构
 */
class NrInterestTreeImpl
{
public:
	//树节点
	struct InterestTreeNode{
		std::map<std::string, InterestTreeNode* > child;//孩子节点
		map<int,bool> NodeId;//感兴趣的节点
		string lane;//当前节点的路段
		//两个简单的构造函数
		InterestTreeNode(std::string x) :lane(x),NodeId(-1){};
		InterestTreeNode() :lane(""),NodeId(-1){};
	};

	void insertInterest(int&id,int pos,const std::vector<std::string>& route,InterestTreeNode* root)
	{
		if(pos == route.size()) return;//已经遍历完毕
		else
		{
			//如果没有找到孩子
			if( root->child.find( route[pos] ) ==root->child.end())
				root->child[route[pos]]=new InterestTreeNode(route[pos]);
			root->child[route[pos]]->NodeId[pos]=true;

			pos++;
			//递归地插入
			insertInterest(id,pos,routeroot->child[route[pos]]);
		}
	}


};


}/* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_NR_INTERESTTREE_IMPL_H_ */
