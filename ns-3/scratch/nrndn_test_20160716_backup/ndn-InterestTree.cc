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
void NrInterestTreeImpl::insertInterest(uint32_t&id,unsigned int pos,const vector<string>& route,InterestTreeNode* root,bool&changeFlag)
{
	if(pos >= route.size()) return;//已经遍历完毕
	else if(pos == 0 && root->lane =="")
	{//初始化插入
		root->lane = prefix+route[pos];
		root->NodeId[id]=true;
		changeFlag=true;
		//cout<<"(InterestTree)初始化插入"<<root->lane<<" ID"<<id<<endl;
		insertInterest(id,pos+1,route,root,changeFlag);
	}
	else
	{
		string nowRoute= prefix+route[pos];
		//如果没有找到孩子
		if( root->child.find( nowRoute ) ==root->child.end())
		{
			root->child[nowRoute]=new InterestTreeNode(nowRoute);
			changeFlag=true;
		}
		root->child[nowRoute]->NodeId[id]=true;
		//cout<<"(InterestTree)  递归插入"<<root->child[route[pos]]->lane<<" ID"<<id<<endl;
		//递归地插入
		insertInterest(id,pos+1,route,root->child[nowRoute],changeFlag);
	}
}
//合并兴趣与路线
bool NrInterestTreeImpl::MergeInterest(uint32_t&id,const vector<string>& oldRoute,string curLane,bool&flag)
{
	curLane=prefix+curLane;
	//如果车辆当前所在的道路与兴趣树的root不相同
	//cout<< "interestTree.cc: 如果车辆当前所在的道路与兴趣树的root不相同" << endl;
	if(root == NULL)
	{
		cout<< "root为空" << endl;
		root = new InterestTreeNode();
		root->lane = curLane;
		//return false;
	}
	//cout <<"interestTree.cc:"<< root->lane << endl;
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
	//cout<< "把%5d等等转换成[]" << endl;
	unsigned int idx=0;
	for(idx=0;idx<route.size();++idx)
	{
		//找出当前的路段
		if(curLane == prefix+route[idx])
			break;
	}
	bool changeFlag=false;
	//没有共同的路段
	//cout<< "没有共同的路段" << endl;
	if(idx == route.size())
		return changeFlag;

	//cout<< "增加当前路段的兴趣节点" << id << endl;
	if(root == NULL)
	{
		cout<< "root为空" << endl;
		root = new InterestTreeNode();
		root->lane = curLane;
		cout <<"interestTree.cc:"<< curLane <<endl;
		//return false;
	}
	//增加当前路段的兴趣节点
	root->NodeId[id]=true;
	//cout<< "root->NodeId[id]=true" << endl;
	insertInterest(id,idx+1,route,root,changeFlag);
	//cout<< "changeFlag" << endl;
	return changeFlag;
}

void NrInterestTreeImpl::levelOrder()
{
	cout<<"(InterestTree)层序遍历"<<endl;
	if(this->root == NULL)
	{
		cout<<"(InterestTree)根结点为空"<<endl;
		return ;
	}
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
	if(currentLane == prefix+"UNKNOWN_LANE")
	{//sensor获取回来的道路未知
		cout<<"(ndn-InterestTree)currentLane UNKNOWN_LANE"<<endl;
		return;//没有找到适当的道路
	}
	cout<<"(ndn-InterestTree)更新所在的路段："<<currentLane<<endl;
	root = levelOrderDelete(currentLane);
	if( root != NULL)
	{
		cout<<"(ndn-InterestTree)更新当前节点，路段为："<<root->lane<<endl;
		//getchar();
	}
}

//层序删除，以找出在第三层或以上的路径，即当前路径不在root的孩子节点，在其更深层次的孩子节点中
InterestTreeNode* NrInterestTreeImpl::levelOrderDelete(string curLane)
{//updateNowRoot中调用，在之前已经添加了prefix
	//删除root的其他孩子节点
	if(curLane == prefix+"UNKNOWN_LANE")
	{//sensor获取回来的道路未知
		cout<<"(ndn-InterestTree)currentLane UNKNOWN_LANE"<<endl;
		return root;//没有找到适当的道路
	}
	if(root == NULL)
	{
		cout<<"(ndn-InterestTree.cc)根结点为空，无法删除"<<endl;
		return root;
	}
	//cout<<curLane<<endl;
	InterestTreeNode* result = NULL;
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
				result = head;
				found = true;
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
string NrInterestTreeImpl::serialize_withId()
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
			vector<vector<char>> nodeIdChar(0);
			//把head对应的感兴趣节点记录到os中
			for(map<int, bool>::iterator ite=head->NodeId.begin();ite!=head->NodeId.end();ite++)
			{
				if(ite->second)//避免了出现不为true的情况，即查询时不小心插入了
				{
					char high = ite->first/256-128;
					char low = ite->first%256-128;
					vector<char> tmpChar={high,low};
					nodeIdChar.push_back(tmpChar);
					//os<<"^"<<ite->first;
				}
			}
			uint32_t nodeSize=nodeIdChar.size();
			char high = nodeSize/256-128;
			char low = nodeSize%256-128;
			os<<"^"<<high<<low;
			for(uint32_t j=0;j<nodeSize;++j)
				os<<nodeIdChar[j][0]<<nodeIdChar[j][1];

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


//序列化，不带NodeId信息 路段^自己编号父亲编号 最大上限为256
string NrInterestTreeImpl::serialize_noId()
{
	if(root==NULL) return "";
	ostringstream os;
	int prefixSize=prefix.size();
	//节点+父亲节点编号
	queue<pair<InterestTreeNode*,unsigned char>> q;
	int count1=1;
	int count2=0;
	unsigned char tmpId=2;
	q.push({root,tmpId});
	os<<root->lane.substr(prefixSize)<<"^"<<tmpId<<(unsigned char)(1);

	while(!q.empty())
	{
		for(int i=0;i<count1;++i)
		{
			InterestTreeNode* head=q.front().first;
			unsigned char parentId=q.front().second;
			q.pop();
			//遍历孩子节点，压紧队列和序列化
			for(map<string,InterestTreeNode*>::iterator ite=head->child.begin();ite!=head->child.end();ite++)
			{
				tmpId++;
				os<<ite->first.substr(prefixSize)<<"^"<<tmpId<<parentId;
				q.push({ite->second,tmpId});//孩子节点+父亲节点编号
				count2++;
			}
		}
		count1=count2;
		count2=0;
	}
	return os.str();
}
//反序列化,不带NodeId
InterestTreeNode* NrInterestTreeImpl::deserialize_noId(string serializeTree)
{
	if(serializeTree=="") return NULL;

	map<int ,InterestTreeNode*> tree_map;
	tree_map[1]=new InterestTreeNode("parent");
	string laneName="";
	for(unsigned int i=0;i<serializeTree.size();)
	{
		if(serializeTree[i]=='^')
		{
			uint32_t childId=serializeTree[i+1];
			uint32_t parentId=serializeTree[i+2];
			tree_map[childId]= new InterestTreeNode(prefix+laneName);//建立新的儿子节点
			tree_map[parentId]->child[laneName]=tree_map[childId];//把儿子节点挂载到父亲节点上
			i+=3;
			//cout<<childId<<" "<<parentId<<" "<<laneName<<endl;
			laneName="";
		}
		else
			laneName+=serializeTree[i++];
	}
	root=tree_map[1]->child.begin()->second;
	return root;
}

void NrInterestTreeImpl::tree2Routes(vector<vector<string>>& routes,vector<string> tmpRoutes,InterestTreeNode* node)
{
	if(node==NULL) return;
	int prefixSize=prefix.size();
	tmpRoutes.push_back(node->lane.substr(prefixSize));
	if(node->child.size()==0&&tmpRoutes.size()!=0)
		routes.push_back(tmpRoutes);
	else
	{
		for(map<string,InterestTreeNode*>::iterator ite=node->child.begin();ite!=node->child.end();ite++)
		{
			//tmpRoutes.push_back(ite->first.substr(prefix.size()));
			tree2Routes(routes,tmpRoutes,ite->second);
			//tmpRoutes.pop_back();
		}
	}
	tmpRoutes.pop_back();
}
//反序列化
InterestTreeNode* NrInterestTreeImpl::deserialize_withId(string serializeTree)
{
	if(serializeTree=="") return NULL;
	serializeTree+="#";//避免最后一层还原不出来
	InterestTreeNode* result;
	vector<InterestTreeNode*> treeVector(0);
	int level=0;
	int flag = 0;
	int laneFlag=1;
	int levelFlag=2;
	int NodeIdFlag=3;
	string laneName="";
	map<int ,InterestTreeNode*> preLevel;
	map<int ,InterestTreeNode*> thisLevel;
	for(unsigned int i=0;i<serializeTree.size();)
	{
		if(serializeTree[i]=='#')
		{//两种情况，刚开始和非刚开始


			flag = levelFlag;
			if(i!=0)
			{//上次的编号要加入到上一层中，并且新开一层，初始化所有参数
				level++;
				preLevel=thisLevel;
				thisLevel.clear();//清楚当前层
			}
			++i;
		}
		else if(serializeTree[i]=='$')
		{//接下来开始记录路段名字

			flag = laneFlag;
			laneName="";
			++i;
		}
		else if(serializeTree[i]=='^')
		{//接下来记录NodeId
			treeVector.push_back(new InterestTreeNode(laneName));
			if(level == 0 && treeVector.size() == 1)
					result = treeVector.back();
			//计算size
			uint32_t nodeSize=((int)serializeTree[i+1]+128)*256+(int)serializeTree[i+2]+128;
			i+=3;
			for(uint32_t j=0;j<nodeSize;++j)
			{
				int nodeValue = ((int)serializeTree[i]+128)*256+(int)serializeTree[i+1]+128;
				i+=2;
				treeVector.back()->NodeId[nodeValue]=true;
				thisLevel[nodeValue]=treeVector.back();//key:NodeId,value:InterestTreeNode
				if(level!=0 && preLevel[nodeValue]->child.find(laneName)==preLevel[nodeValue]->child.end())
					preLevel[nodeValue]->child[laneName]=treeVector.back();//挂载到父节点上
			}

			flag = NodeIdFlag;
		}
		else
		{
			if(flag == levelFlag)
			{}
			else if(flag == laneFlag)
				laneName+=serializeTree[i];

			++i;
		}

	}
	return result;
}

void NrInterestTreeImpl::convert2Routes(vector<vector<string>>&routes,vector<uint32_t>&node_vec)
{
	//遍历根部的所有节点，每个节点都有一个到根的路线

	node_vec=vector<uint32_t> (0);
	for(map<int,bool>::iterator ite = root->NodeId.begin();ite!=root->NodeId.end();ite++)
	{
		if(ite->second)
			node_vec.push_back(ite->first);
	}
	routes=vector<vector<string>> (node_vec.size(),vector<string>(0));

	for(uint32_t i=0;i<node_vec.size();++i)
	{
		routes[i]=getSingleRoute(node_vec[i]);
	}
}

vector<string>  NrInterestTreeImpl::getSingleRoute(uint32_t node)
{
	vector<string> tmp(0);
	getSingleRoute_dfs(tmp,node,root);
	return tmp;
}

void NrInterestTreeImpl::getSingleRoute_dfs(vector<string>& result,uint32_t&node,InterestTreeNode*  treeNode)
{
	//找到node所在的路线
	if(treeNode->NodeId.find(node)!=treeNode->NodeId.end())
	{
		//需要把前缀prefix去掉
		result.push_back(treeNode->lane.substr(prefix.size()));
		//遍历孩子节点
		for(map<string, InterestTreeNode* >::iterator ite=treeNode->child.begin();ite!=treeNode->child.end();ite++)
		{
			if(ite->second)
			{
				//找到了直接返回就可以
				if(ite->second->NodeId.find(node)!=ite->second->NodeId.end())
				{
					getSingleRoute_dfs(result,node,ite->second);
					return ;
				}
			}
		}
	}
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


