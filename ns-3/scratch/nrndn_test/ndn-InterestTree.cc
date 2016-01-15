/*
 * ndn-InterestTree.cc
 *
 *  Created on: Jan 13, 2016
 *  Author       : siukwan
 */

#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("ndn.pit.InterestTree");

#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"
#include "ndn-InterestTree.h"

namespace ns3
{
namespace ndn
{
namespace pit
{
namespace nrndn
{
TypeId
NrInterestTreeImpl::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::pit::nrndn::NrInterestTreeImpl")
    .SetGroupName ("Ndn")
    .SetParent<Object> ()
    .AddConstructor< NrInterestTreeImpl > ()
  /*  .AddAttribute ("CleanInterval", "cleaning interval of the timeout incoming faces of PIT entry",
   			                    TimeValue (Seconds (10)),
   			                    MakeTimeAccessor (&NrPitImpl::m_cleanInterval),
   			                    MakeTimeChecker ())*/
    ;

  return tid;
}

NrInterestTreeImpl::NrInterestTreeImpl()
{
	root = new InterestTreeNode();
	NodeId=-1;
}
NrInterestTreeImpl::~NrInterestTreeImpl ()
{

}
void NrInterestTreeImpl::insertInterest(uint32_t&id,unsigned int pos,const std::vector<std::string>& route,InterestTreeNode* root)
{
	if(pos >= route.size()) return;//已经遍历完毕
	else if(pos == 0 && root->lane =="")
	{//初始化插入
		root->lane = route[0];
		root->NodeId[id]=true;
		cout<<"(InterestTree)初始化插入"<<root->lane<<" ID"<<id<<endl;
		insertInterest(id,pos+1,route,root);
	}
	else
	{
		//如果没有找到孩子
		if( root->child.find( route[pos] ) ==root->child.end())
			root->child[route[pos]]=new InterestTreeNode(route[pos]);
		root->child[route[pos]]->NodeId[id]=true;
		cout<<"(InterestTree)  递归插入"<<root->child[route[pos]]->lane<<" ID"<<id<<endl;
		//递归地插入
		insertInterest(id,pos+1,route,root->child[route[pos]]);
	}
}

void NrInterestTreeImpl::updateNowRoot(string currentLane)
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
void NrInterestTreeImpl::deleteTree(InterestTreeNode* deleteNode)
{
	//如果节点为空，则直接返回
	if(deleteNode == NULL ) return;

	std::map<string , InterestTreeNode*>::iterator ite = deleteNode->child.begin();
	//先删除子节点
	for(;ite!=deleteNode->child.end();ite++)
	{//迭代删除
		deleteTree(ite->second);
	}
	//在删除当前节点
	delete deleteNode;

}

} /* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */


