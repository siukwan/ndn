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
void NrInterestTreeImpl::insertInterest(uint32_t&id,unsigned int pos,const vector<string>& route,InterestTreeNode* root)
{
	if(pos >= route.size()) return;//已经遍历完毕
	else if(pos == 0 && root->lane =="")
	{//初始化插入
		root->lane = route[0];
		root->NodeId[id]=true;
		//cout<<"(InterestTree)初始化插入"<<root->lane<<" ID"<<id<<endl;
		insertInterest(id,pos+1,route,root);
	}
	else
	{
		//如果没有找到孩子
		if( root->child.find( route[pos] ) ==root->child.end())
			root->child[route[pos]]=new InterestTreeNode(route[pos]);
		root->child[route[pos]]->NodeId[id]=true;
		//cout<<"(InterestTree)  递归插入"<<root->child[route[pos]]->lane<<" ID"<<id<<endl;
		//递归地插入
		insertInterest(id,pos+1,route,root->child[route[pos]]);
	}
}

void NrInterestTreeImpl::MergeInterest(uint32_t&id,unsigned int pos,const vector<string>& oldRoute,string curLane,bool&flag)
{
	//如果车辆当前所在的道路与兴趣树的root不相同
	if(curLane!= root->lane)
	{
		updateNowRoot(curLane);
		flag=true;
	}vector<string> route(oldRoute.size());
	//把%5d等等转换成[]
	for(unsigned int i=0;i<oldRoute.size();++i)
	{
		route[i]=uriConvertToString(oldRoute[i]);
	}
	unsigned int idx=0;
	for(idx=0;idx<route.size();++idx)
	{
		//找出当前的路段
		if(curLane == route[idx])
			break;
	}
	//没有共同的路段
	if(idx == route.size())
		return;

	//增加当前路段的兴趣节点
	root->NodeId[id]=true;
	insertInterest(id,idx+1,route,root);
}

void NrInterestTreeImpl::levelOrder()
{
	cout<<"(InterestTree)层序遍历"<<endl;
	queue<InterestTreeNode*> q;
	q.push(this->root);
	int count1=1;
	int count2=0;
	int level=0;
	while(!q.empty())
	{
		cout<<level<<":";
		for(int i=0;i<count1;i++)
		{
			InterestTreeNode* head=q.front();
			q.pop();
			cout<<head->lane<<"( ";
			//输出相应感兴趣的节点
			for(map<int,bool>::iterator ite=head->NodeId.begin();ite!=head->NodeId.end();ite++)
				cout<<ite->first<<" ";
			cout<<")  ";
			map<string, InterestTreeNode* >::iterator ite = head->child.begin();
			for(;ite!=head->child.end();ite++)
			{
				count2++;
				q.push(ite->second);
			}
		}
		count1=count2;
		count2=0;
		level++;
		cout<<endl;
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
	//删除root的其他孩子节点
	map<string , InterestTreeNode*>::iterator ite = root->child.begin();
	vector<InterestTreeNode*> deleteNodeContainer(0);
	for(;ite!=root->child.end();ite++)
	{
		if(ite->first!=currentLane)
			deleteNodeContainer.push_back(ite->second);
	}
	//更新root节点
	delete root;
	root=tmp;
	cout<<"更新当前节点，路段为："<<root->lane<<endl;
	getchar();
	//为了节约内存，需要删除其他不是currentLane的兄弟子树
	for(unsigned int i=0;i<deleteNodeContainer.size();++i)
	{
		deleteTree(deleteNodeContainer[i]);
	}

}
void NrInterestTreeImpl::deleteTree(InterestTreeNode* deleteNode)
{
	//如果节点为空，则直接返回
	if(deleteNode == NULL ) return;

	map<string , InterestTreeNode*>::iterator ite = deleteNode->child.begin();
	//先删除子节点
	for(;ite!=deleteNode->child.end();ite++)
	{//迭代删除
		deleteTree(ite->second);
	}
	//在删除当前节点
	delete deleteNode;

}


//小锟添加，2016-1-15
string NrInterestTreeImpl::uriConvertToString(string str)
{
	//因为获取兴趣时使用toUri，避免出现类似[]的符号，进行编码转换
	std::string ret="";
	for(uint32_t i=0;i<str.size();i++)
	{
		if(i+2<str.size())
		{
			if(str[i]=='%'&&str[i+1]=='5'&&str[i+2]=='B')
			{
				ret+="[";
				i=i+2;
			}
			else if(str[i]=='%'&&str[i+1]=='5'&&str[i+2]=='D')
			{
				ret+="]";
				i=i+2;
			}
			else
				ret+=str[i];
		}
		else
			ret+=str[i];
	}
	return ret;
}

} /* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */


