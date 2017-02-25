/*
 * ndn-nr-pit-impl.cc
 *
 *  Created on: Jan 20, 2015
 *      Author: chen
 */

#include "ndn-nr-pit-impl.h"
#include "ndn-pit-entry-nrimpl.h"
//#include "NodeSensor.h"


#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE ("ndn.pit.NrPitImpl");

#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace ndn
{
namespace pit
{
namespace nrndn
{


NS_OBJECT_ENSURE_REGISTERED (NrPitImpl);

TypeId
NrPitImpl::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::ndn::pit::nrndn::NrPitImpl")
    .SetGroupName ("Ndn")
    .SetParent<Pit> ()
    .AddConstructor< NrPitImpl > ()
    .AddAttribute ("CleanInterval", "cleaning interval of the timeout incoming faces of PIT entry",
   			                    TimeValue (Seconds (10)),
   			                    MakeTimeAccessor (&NrPitImpl::m_cleanInterval),
   			                    MakeTimeChecker ())
    ;

  return tid;
}

NrPitImpl::NrPitImpl ():
		m_cleanInterval(Seconds(10.0))
{
	m_nrtree = ns3::Create<pit::nrndn::NrInterestTreeImpl> ();
}

NrPitImpl::~NrPitImpl ()
{
}


void
NrPitImpl::NotifyNewAggregate ()
{
	if (m_fib == 0)
	{
		m_fib = GetObject<Fib>();
	}
	if (m_forwardingStrategy == 0)
	{
		m_forwardingStrategy = GetObject<ForwardingStrategy>();
	}

	if (m_sensor == 0)
	{
		m_sensor = GetObject<ndn::nrndn::NodeSensor>();
		// Setup Lane change action
		if (m_sensor != NULL)
		{
			m_sensor->TraceConnectWithoutContext("LaneChange",
					MakeCallback(&NrPitImpl::laneChange, this));

			//PIT需要m_sensor，所以在m_sensor初始化后，马上初始化PIT表
			//NrPitEntry needs m_sensor. Initialize immediately after m_sensor is aggregated
			//std::cout<<"(ndn-nr-pit-impl)初始化PIT"<<std::endl;
			//getchar();
			InitializeNrPitEntry();
		}
	}

  Pit::NotifyNewAggregate ();
}
//获取车辆当前所在的路段
std::string NrPitImpl::getCurrentLane()
{
	std::vector<Ptr<Entry> >::iterator pit=m_pitContainer.begin();
	Ptr<Entry> entry = *pit;
	//获取兴趣
	Name::const_iterator head=entry->GetInterest()->GetName().begin();
	//当前路段为：uriConvertToString(head->toUri())
	return uriConvertToString(head->toUri());
}
//更新PIT表
bool NrPitImpl::UpdatePit(const std::vector<std::string>& route,const uint32_t& id)
{
	std::ostringstream os;
	std::vector<Ptr<Entry> >::iterator pit=m_pitContainer.begin();
	Ptr<Entry> entry = *pit;
	//获取兴趣
	Name::const_iterator head=entry->GetInterest()->GetName().begin();
	//Can name::Component use "=="?
	std::vector<std::string>::const_iterator it=
			std::find(route.begin(),route.end(),head->toUri());

	//当前路段为：uriConvertToString(head->toUri())

	//std::cout<<"当前路段"<<uriConvertToString(head->toUri())<<std::endl;


	//找不到
	if(it==route.end())
		return false;
	//找到，把后面的添加进去
	for(;pit!=m_pitContainer.end()&&it!=route.end();++pit,++it)
	{
		const name::Component &pitName=(*pit)->GetInterest()->GetName().get(0);
		if(pitName.toUri() == *it)
		{
			Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
			pitEntry->AddIncomingNeighbors(id);
			os<<(*pit)->GetInterest()->GetName().toUri()<<" add Neighbor "<<id<<' ';
			//std::cout<<uriConvertToString((*pit)->GetInterest()->GetName().toUri())<<" ";
		}
		else
			break;

	}
	cout<<"(pit-impl.cc)添加后"<<id<<endl;
	showPit();
	getchar();
	//NS_LOG_UNCOND("update pit:"<<os.str());
	NS_LOG_DEBUG("update pit:"<<os.str());
	return true;
}

void NrPitImpl::showPit()
{
	cout<<"(pit-impl.cc)显示pit内容："<<endl;
	for(uint32_t i=0;i<m_pitContainer.size();++i)
	{
		Ptr<EntryNrImpl> pitEntry_siu = DynamicCast<EntryNrImpl>(m_pitContainer[i]);
		pitEntry_siu->listPitEntry();
	}

}

//小锟添加，通过兴趣树更新PIT表
bool NrPitImpl::UpdatePitByInterestTree(Ptr<pit::nrndn::NrInterestTreeImpl>&receivetree,const uint32_t& id)
{

	for(std::vector<Ptr<Entry> >::iterator pit=m_pitContainer.begin();pit!=m_pitContainer.end();++pit)
	{
		//删除指定id的邻居
		Ptr<EntryNrImpl> pitEntry_siu = DynamicCast<EntryNrImpl>(*pit);
		pitEntry_siu->CleanPITNeighbors(id);
	}
	
	//cout<<"(pit-impl.cc)删除后"<<id<<endl;
	//showPit();
	//getchar();
	
	std::ostringstream os;
	//进行广度优先搜索，把兴趣树的所有节点都放到set里面
	set<string> tree_set;
	if(receivetree->root == NULL)
		return false;
	queue<InterestTreeNode*> q;
	q.push(receivetree->root);
	int count1=1;
	int count2=0;
	int level=0;
	while(!q.empty())
	{
		for(int i=0;i<count1;i++)
		{
			InterestTreeNode* head=q.front();
			q.pop();
			tree_set.insert(head->lane);
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
	}

	for(std::vector<Ptr<Entry> >::iterator pit=m_pitContainer.begin();pit!=m_pitContainer.end();++pit)
	{
		const name::Component &pitName=(*pit)->GetInterest()->GetName().get(0);
		//cout<<receivetree->prefix+uriConvertToString(pitName.toUri())<<" ";
		if(tree_set.find(receivetree->prefix+uriConvertToString(pitName.toUri()))!=tree_set.end())
		{
			//cout<<"\n(找到)"<<receivetree->prefix+uriConvertToString(pitName.toUri())<<endl;
			Ptr<EntryNrImpl> pitEntry = DynamicCast<EntryNrImpl>(*pit);
			pitEntry->AddIncomingNeighbors(id);
			os<<(*pit)->GetInterest()->GetName().toUri()<<" add Neighbor "<<id<<' ';
			//std::cout<<uriConvertToString((*pit)->GetInterest()->GetName().toUri())<<" ";
		}
	}
	
	/*if( id == 15)
	{
		cout<<"\n(pit-impl.cc)添加后"<<id<<endl;
		showPit();
		getchar();
	}*/
	//NS_LOG_UNCOND("update pit:"<<os.str());
	
	NS_LOG_DEBUG("update pit:"<<os.str());
	return true;
}


//小锟添加，通过兴趣树更新PIT表
bool NrPitImpl::UpdatePitByInterestTree2(Ptr<pit::nrndn::NrInterestTreeImpl>&receivetree)
{
	
	/*
	cout<<"(pit-impl.cc)更新前"<<endl;
	showPit();
	*/
	m_pitContainer.clear();
	
	std::ostringstream os;
	//进行广度优先搜索，把兴趣树的所有节点都放到set里面
	set<string> tree_set;
	if(receivetree->root == NULL)
		return false;
	queue<InterestTreeNode*> q;
	q.push(receivetree->root);
	int count1=1;
	int count2=0;
	int level=0;
	while(!q.empty())
	{
		for(int i=0;i<count1;i++)
		{
			InterestTreeNode* head=q.front();
			q.pop();
			tree_set.insert(head->lane);
			
			Ptr<Name> name = ns3::Create<Name>(head->lane);
			Ptr<Interest> interest=ns3::Create<Interest> ();
			//cout<< "interest->SetName" <<endl;
			interest->SetName				(name);
			interest->SetInterestLifetime	(Time::Max());//never expire
			//cout<< "interest->SetName ok" <<endl;
			//Create a fake FIB entry(if not ,L3Protocol::RemoveFace will have problem when using pitEntry->GetFibEntry)
			Ptr<fib::Entry> fibEntry=ns3::Create<fib::Entry>(Ptr<Fib>(0),Ptr<Name>(0));
			Ptr<Entry> entry = ns3::Create<EntryNrImpl>(*this,interest,fibEntry,m_cleanInterval) ;
			m_pitContainer.push_back(entry);
			
			//cout<< "m_pitContainer.push_back" <<endl;
			Ptr<EntryNrImpl> pitEntry_siu = DynamicCast<EntryNrImpl>(m_pitContainer.back());
			
			//cout<< "pitEntry_siu" <<endl;
			for(auto ite = head->NodeId.begin(); ite != head->NodeId.end(); ite++)
			{
				pitEntry_siu->AddIncomingNeighbors(ite->first);
			}
			
			//cout<< "AddIncomingNeighbors" <<endl;
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
	}
/*
	cout<<"(pit-impl.cc)更新后后后"<<endl;
	showPit();
	getchar();
*/	
	/*if( id == 15)
	{
		cout<<"\n(pit-impl.cc)添加后"<<id<<endl;
		showPit();
		getchar();
	}*/
	//NS_LOG_UNCOND("update pit:"<<os.str());
	
	NS_LOG_DEBUG("update pit:"<<os.str());
	return true;
}

void
NrPitImpl::DoDispose ()
{
	m_forwardingStrategy = 0;
	//m_fib = 0;
  
	Pit::DoDispose ();
 }
  

Ptr<Entry>
NrPitImpl::Lookup (const Data &header)
{
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Lookup (const Data &header) should not be invoked");
	return 0;
 }
  
Ptr<Entry>
NrPitImpl::Lookup (const Interest &header)
{
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Lookup (const Interest &header) should not be invoked");
	return 0;
 }
  

Ptr<Entry>
NrPitImpl::Find (const Name &prefix)
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Find (const Name &prefix) should not be invoked");
	 NS_LOG_INFO ("Finding prefix"<<prefix.toUri());
	 std::vector<Ptr<Entry> >::iterator it;
	 if(m_pitContainer.size() == 0)
	 	return 0;
	 //NS_ASSERT_MSG(m_pitContainer.size()!=0,"Empty pit container. No initialization?");
	 for(it=m_pitContainer.begin();it!=m_pitContainer.end();++it)
	 {
		 //如果找到兴趣，就返回入口
		 if((*it)->GetPrefix()==prefix)
			 return *it;
	 }
	return 0;
}
  

Ptr<Entry>
NrPitImpl::Create (Ptr<const Interest> header)
 {

	NS_LOG_DEBUG (header->GetName ());
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Create (Ptr<const Interest> header) "
			"should not be invoked, use "
			"NrPitImpl::CreateNrPitEntry instead");
	return 0;
}



//PIT表初始化
bool
NrPitImpl::InitializeNrPitEntry()
{
	NS_LOG_FUNCTION (this);
	//获取所有的导航路线
	const std::vector<std::string>& route =	m_sensor->getNavigationRoute();
	std::vector<std::string>::const_iterator rit;
	//每个路段配置一个兴趣
	for(rit=route.begin();rit!=route.end();++rit)
	{
		Ptr<Name> name = ns3::Create<Name>('/'+*rit);
		Ptr<Interest> interest=ns3::Create<Interest> ();
		interest->SetName				(name);
		interest->SetInterestLifetime	(Time::Max());//never expire

		//Create a fake FIB entry(if not ,L3Protocol::RemoveFace will have problem when using pitEntry->GetFibEntry)
		Ptr<fib::Entry> fibEntry=ns3::Create<fib::Entry>(Ptr<Fib>(0),Ptr<Name>(0));

		Ptr<Entry> entry = ns3::Create<EntryNrImpl>(*this,interest,fibEntry,m_cleanInterval) ;
		m_pitContainer.push_back(entry);
		NS_LOG_DEBUG("Initialize pit:Push_back"<<name->toUri());
	}
	return true;
}
  

void
NrPitImpl::MarkErased (Ptr<Entry> item)
{
	NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::MarkErased (Ptr<Entry> item) should not be invoked");
	return;
}
  
  
void
NrPitImpl::Print (std::ostream& os) const
{

}

uint32_t
NrPitImpl::GetSize () const
{
	return m_pitContainer.size ();
}
  
Ptr<Entry>
NrPitImpl::Begin ()
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Begin () should not be invoked");

	if(m_pitContainer.begin() == m_pitContainer.end())
		return End();
	else
		return *(m_pitContainer.begin());
}

Ptr<Entry>
NrPitImpl::End ()
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::End () should not be invoked");
	return 0;
}
  
Ptr<Entry>
NrPitImpl::Next (Ptr<Entry> from)
{
	//NS_ASSERT_MSG(false,"In NrPitImpl,NrPitImpl::Next () should not be invoked");
	if (from == 0) return 0;

	std::vector<Ptr<Entry> >::iterator it;
	it = find(m_pitContainer.begin(),m_pitContainer.end(),from);
	if(it==m_pitContainer.end())
		return End();
	else
	{
		++it;
		if(it==m_pitContainer.end())
			return End();
		else
			return *it;
	}
}

//小锟添加，2015-8-23
std::string NrPitImpl::uriConvertToString(std::string str)
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
			else if(str[i]=='%'&&str[i+1]=='2'&&str[i+2]=='3')
			{
				ret+="#";
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
void NrPitImpl::laneChange(std::string oldLane, std::string newLane)
{
	//cout << "ndn-nr-pit-impl.cc:" << "lanechange" <<endl;
	int needtoUpdateroot=0;
	if (oldLane.empty()
			|| (ndn::nrndn::NodeSensor::emptyLane == oldLane
					&& ndn::nrndn::NodeSensor::emptyLane != newLane))
		return;
	NS_LOG_INFO ("Deleting old lane pit entry of "<<oldLane);
	//cout << "ndn-nr-pit-impl.cc:" << "Deleting old lane pit entry of "<<oldLane <<endl;

	std::vector<Ptr<Entry> >::iterator it;
	it =m_pitContainer.begin();

	if(it == m_pitContainer.end())
	{
		//已经为空，不用删除
		cout << "pit container is NULL" << endl;
		return ;
	}

	//cout << (*it)->GetInterest()->GetName().get(0).toUri() << endl;
	bool IsOldLaneAtPitBegin =(  uriConvertToString((*it)->GetInterest()->GetName().get(0).toUri())==(oldLane));
	//cout << "ndn-nr-pit-impl.cc:" << "IsOldLaneAtPitBegin" <<endl;

	if(!IsOldLaneAtPitBegin)
	{
		//std::cout<< "ndn-nr-pit-impl.cc:"<<"旧路段不在头部:"<<"oldLane:"<<(oldLane)<<" newLane:"<<uriConvertToString((*it)->GetInterest()->GetName().get(0).toUri())<<std::endl;


		//遍历整个Pit
		std::vector<Ptr<Entry> >::iterator itTraversal;
		itTraversal =m_pitContainer.begin();
		bool findOldLane=false;
		//std::cout<<"寻找oldLane中...\n";
		for(;itTraversal!=m_pitContainer.end();itTraversal++)
		{//遍历整个PIT表，寻找oldLane是否在表中
			if( uriConvertToString((*itTraversal)->GetInterest()->GetName().get(0).toUri()) == (oldLane) )
			{//如果找到则直接跳出
				findOldLane=true;
				break;
			}
		}
		if(findOldLane)
		{
			//cout << "ndn-nr-pit-impl.cc:" << "findOldLane" <<endl;
			//就路段不在头部，但是在后面
			needtoUpdateroot=1;
			it =m_pitContainer.begin();
			int a=0;
			while(  uriConvertToString((*it)->GetInterest()->GetName().get(0).toUri())!=(oldLane)
					&&it!=m_pitContainer.end())
			{
				//std::cout<<a<<"遍历删除中："<<uriConvertToString( (*it)->GetInterest()->GetName().get(0).toUri())<<" OLd:"<<(oldLane)<<std::endl;
				a++;
				DynamicCast<EntryNrImpl>(*it)->RemoveAllTimeoutEvent();
				m_pitContainer.erase(it);
				it =m_pitContainer.begin();
			}
			if(it<=m_pitContainer.end())
			{
				//std::cout<<"最后遍历删除中："<<uriConvertToString( (*it)->GetInterest()->GetName().get(0).toUri())<<" OLd:"<<(oldLane)<<std::endl;
				//1. Befor erase it, cancel all the counting Timer fore the neighbor to expire
				DynamicCast<EntryNrImpl>(*it)->RemoveAllTimeoutEvent();
				//2. erase it
				m_pitContainer.erase(it);
				//std::cout<<"删除完毕\n";
			}
			else
			{
				//std::cout<<"删除完毕：迭代器为空\n";
			}

		}
		else
		{
			//std::cout<<"没找到...\n";
		}
	}
	else
	{//旧路段在pit头部才进行删除

		//cout << "ndn-nr-pit-impl.cc:" << "旧路段在pit头部才进行删除" <<endl;
			//报错？
		//NS_ASSERT_MSG(IsOldLaneAtPitBegin,"The old lane should at the beginning of the pitContainer. Please Check~");
		//1. Befor erase it, cancel all the counting Timer fore the neighbor to expire
		DynamicCast<EntryNrImpl>(*it)->RemoveAllTimeoutEvent();

		//就路段在头部，但是在后面
		needtoUpdateroot=2;
		//2. erase it
		m_pitContainer.erase(it);
		//std::cout<<"erase OK!"<<std::endl;
		return;
	}
	if(needtoUpdateroot)
	{
		//cout<<"\n(ndn-nr-pit-impl)needtoUpdateroot新路段："<<newLane<<"  "<<needtoUpdateroot<<endl;
   	m_nrtree->levelOrder();
		m_nrtree->updateNowRoot(m_nrtree->prefix+newLane);
		m_nrtree->levelOrder();
		//getchar();
	}
}

void NrPitImpl::DoInitialize(void)
{
	Pit::DoInitialize();
}

} /* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */


