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
	/*
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

	void updateNowRoot(string currentLane)
	{
		//遍历root的子节点，找到currentLane所在的节点，删除其他

		InterestTreeNode *tmp=root->child[currentLane];
		if( NULL ==tmp)
		{
			std::cout<<"车辆"<<NodeId<<"的兴趣树不存在子节点："<<currentLane<<endl;
		}

		//更新root节点
		root=tmp;
		//为了节约内存，需要删除其他不是currentLane的兄弟子树

	}
	void deleteTree(InterestTreeNode* deleteNode)
	{
		//如果节点为空，则直接返回
		if(deleteNode == NULL ) return;

		std::map<string , InterestTreeNode*>::ite = deleteNode->child.begin();
		//先删除子节点
		for(;ite!=deleteNode->child.end();ite++)
		{//迭代删除
			deleteTree(ite);
		}
		//在删除当前节点
		free(deleteNode);

	}
*/
private:
	InterestTreeNode *root;
	int NodeId;

};


}/* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_NR_INTERESTTREE_IMPL_H_ */
