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
	prefix="/";
}
NrInterestTreeImpl::~NrInterestTreeImpl ()
{

}
void NrInterestTreeImpl::insertInterest(uint32_t&id,unsigned int pos,const vector<string>& route,InterestTreeNode* root)
{
	if(pos >= route.size()) return;//已经遍历完毕
	else if(pos == 0 && root->lane =="")
	{//初始化插入
		root->lane = prefix+route[pos];
		root->NodeId[id]=true;
		//cout<<"(InterestTree)初始化插入"<<root->lane<<" ID"<<id<<endl;
		insertInterest(id,pos+1,route,root);
	}
	else
	{
		string nowRoute= prefix+route[pos];
		//如果没有找到孩子
		if( root->child.find( nowRoute ) ==root->child.end())
			root->child[nowRoute]=new InterestTreeNode(nowRoute);
		root->child[nowRoute]->NodeId[id]=true;
		//cout<<"(InterestTree)  递归插入"<<root->child[route[pos]]->lane<<" ID"<<id<<endl;
		//递归地插入
		insertInterest(id,pos+1,route,root->child[nowRoute]);
	}
}
//合并兴趣与路线
void NrInterestTreeImpl::MergeInterest(uint32_t&id,unsigned int pos,const vector<string>& oldRoute,string curLane,bool&flag)
{
	curLane=prefix+curLane;
	//如果车辆当前所在的道路与兴趣树的root不相同
	if(curLane!= root->lane)
	{
		updateNowRoot(curLane);
		flag=true;
	}
	vector<string> route(oldRoute.size());
	//把%5d等等转换成[]
	for(unsigned int i=0;i<oldRoute.size();++i)
	{
		route[i]=uriConvertToString(oldRoute[i]);
	}
	unsigned int idx=0;
	for(idx=0;idx<route.size();++idx)
	{
		//找出当前的路段
		if(curLane == prefix+route[idx])
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
	//getchar();
}

void NrInterestTreeImpl::updateNowRoot(string currentLane)
{//在MergeInterest中调用，curLane已经增加了prefix
	//删除root的其他孩子节点
	if(currentLane == "UNKNOWN_LANE")
	{//sensor获取回来的道路未知
		cout<<"(ndn-InterestTree)currentLane UNKNOWN_LANE"<<endl;
		getchar();
		return;//没有找到适当的道路
	}
	cout<<"(ndn-InterestTree)更新所在的路段："<<currentLane<<endl;
	root = levelOrderDelete(currentLane);
	cout<<"(ndn-InterestTree)更新当前节点，路段为："<<root->lane<<endl;
	//getchar();
}

//层序删除，以找出在第三层或以上的路径，即当前路径不在root的孩子节点，在其更深层次的孩子节点中
InterestTreeNode* NrInterestTreeImpl::levelOrderDelete(string curLane)
{//updateNowRoot中调用，在之前已经添加了prefix
	InterestTreeNode*result=NULL;
	queue<InterestTreeNode*> q;
	int count1=1;
	int count2=0;
	q.push(root);
	while(!q.empty())
	{
		bool found=false;
		for(int i=0;i<count1;++i)
		{
			InterestTreeNode* head=q.front();
			q.pop();
			if(head->lane == curLane)
			{
				result=head;
				found =true;
				continue;
			}
			else
			{
				map<string, InterestTreeNode* >::iterator ite = head->child.begin();
				for(;ite!=head->child.end();ite++)
				{
					if(ite->second==NULL) continue;
					count2++;
					q.push(ite->second);
				}
				delete head;//删除该结点
			}
		}
		//找到就跳出
		if(found) break;

		count1=count2;
		count2=0;
	}

	//把队列中的节点递归删除
	while(!q.empty())
	{
		InterestTreeNode* head=q.front();
		q.pop();
		deleteTree(head);
	}
	return result;
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

//序列化#层数$路段1^id1^id2^id3$路段2^id4^id5
string NrInterestTreeImpl::serialize()
{
	ostringstream os;
	int level=0;
	queue<InterestTreeNode*> q;
	int count1=1;
	int count2=0;
	q.push(root);

	while(!q.empty())
	{
		//记录当前的层数
		os<<"#"<<level;

		for(int i=0;i<count1;++i)
		{
			InterestTreeNode* head=q.front();
			q.pop();
			os<<"$"<<head->lane;
			//把head对应的感兴趣节点记录到os中
			for(map<int, bool>::iterator ite=head->NodeId.begin();ite!=head->NodeId.end();ite++)
			{
				if(ite->second)//避免了出现不为true的情况，即查询时不小心插入了
				{
					os<<"^"<<ite->first;
				}
			}
			for(map<string,InterestTreeNode*>::iterator ite=head->child.begin();ite!=head->child.end();ite++)
			{
				q.push(ite->second);
				count2++;
			}
		}
		level++;
		count1=count2;
		count2=0;
	}
	return os.str();
}
//反序列化
InterestTreeNode* NrInterestTreeImpl::deserialize(string serializeTree)
{
	serializeTree+="#";//避免最后一层还原不出来
	InterestTreeNode* result;
	vector<InterestTreeNode*> treeVector(0);
	int level=0;
	int flag = 0;
	int laneFlag=1;
	int levelFlag=2;
	int NodeIdFlag=3;
	string laneName="";
	int InterestedNodeId=0;
	map<int ,InterestTreeNode*> preLevel;
	map<int ,InterestTreeNode*> thisLevel;
	for(unsigned int i=0;i<serializeTree.size();++i)
	{
		if(serializeTree[i]=='#')
		{//两种情况，刚开始和非刚开始

			if(flag == NodeIdFlag)
			{//知道NodeId值，可以挂在到父节点上
				treeVector.back()->NodeId[InterestedNodeId]=true;
				thisLevel[InterestedNodeId]=treeVector.back();//key:NodeId,value:InterestTreeNode
				if(level!=0 && preLevel[InterestedNodeId]->child.find(laneName)==preLevel[InterestedNodeId]->child.end())
					preLevel[InterestedNodeId]->child[laneName]=treeVector.back();//挂载到父节点上
				InterestedNodeId=0;
			}

			flag = levelFlag;
			if(i!=0)
			{//上次的编号要加入到上一层中，并且新开一层，初始化所有参数
				level++;
				preLevel=thisLevel;
				thisLevel.clear();//清楚当前层
			}
		}
		else if(serializeTree[i]=='$')
		{//接下来开始记录路段名字
			if(flag == NodeIdFlag)
			{//知道NodeId值，可以挂在到父节点上
				treeVector.back()->NodeId[InterestedNodeId]=true;
				thisLevel[InterestedNodeId]=treeVector.back();//key:NodeId,value:InterestTreeNode
				if(level!=0 && preLevel[InterestedNodeId]->child.find(laneName)==preLevel[InterestedNodeId]->child.end())
					preLevel[InterestedNodeId]->child[laneName]=treeVector.back();//挂载到父节点上
				InterestedNodeId=0;
			}
			flag = laneFlag;
			laneName="";
			InterestedNodeId=0;
		}
		else if(serializeTree[i]=='^')
		{//接下来记录NodeId
			if(flag == laneFlag)
			{//新建节点，因为只知道路段名，不知道NodeId的值，所以无法挂载到父节点上
				treeVector.push_back(new InterestTreeNode(laneName));
				if(level == 0 && treeVector.size() == 1)
					result = treeVector.back();
				InterestedNodeId=0;
			}
			else
			{//知道NodeId值，可以挂在到父节点上
				treeVector.back()->NodeId[InterestedNodeId]=true;
				thisLevel[InterestedNodeId]=treeVector.back();//key:NodeId,value:InterestTreeNode
				if(level!=0 && preLevel[InterestedNodeId]->child.find(laneName)==preLevel[InterestedNodeId]->child.end())
					preLevel[InterestedNodeId]->child[laneName]=treeVector.back();//挂载到父节点上
				InterestedNodeId=0;
			}
			flag = NodeIdFlag;
		}
		else
		{
			if(flag == levelFlag)
			{}
			else if(flag == laneFlag)
				laneName+=serializeTree[i];
			else if(flag == NodeIdFlag)
				InterestedNodeId= InterestedNodeId*10+serializeTree[i]-'0';
		}

	}
	return result;
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


