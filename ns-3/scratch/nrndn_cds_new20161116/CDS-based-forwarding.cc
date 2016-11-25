/*
 * CDS-based-forwarding.cc
 *
 *  Created on: 2015��3��4��
 *      Author: chen
 */

#include "CDS-based-forwarding.h"
#include "nrHeader.h"
#include "nrUtils.h"

#include "ns3/core-module.h"
#include "ns3/ptr.h"
#include "ns3/ndn-interest.h"
#include "ns3/log.h"
#include "ns3/object.h"
#include "ns3/node.h"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.h"
#include "ns3/ndn-face.h"
#include "ns3/ndn-data.h"

#include <algorithm>    // std::find

NS_LOG_COMPONENT_DEFINE ("ndn.fw.CDSBasedForwarding");

namespace ns3
{
namespace ndn
{
namespace fw
{
namespace nrndn
{
NS_OBJECT_ENSURE_REGISTERED (CDSBasedForwarding);

TypeId CDSBasedForwarding::GetTypeId(void)
{
	  static TypeId tid = TypeId ("ns3::ndn::fw::nrndn::CDSBasedForwarding")
	    .SetGroupName ("Ndn")
	    .SetParent<ForwardingStrategy> ()
	    .AddConstructor<CDSBasedForwarding>()
	    .AddAttribute ("HelloInterval", "HELLO messages emission interval.",
	            TimeValue (Seconds (0.5)),
	            MakeTimeAccessor (&CDSBasedForwarding::HelloInterval),
	            MakeTimeChecker ())
	     .AddAttribute ("AllowedHelloLoss", "Number of hello messages which may be loss for valid link.",
	            UintegerValue (2),
	            MakeUintegerAccessor (&CDSBasedForwarding::AllowedHelloLoss),
	            MakeUintegerChecker<uint32_t> ())
          .AddAttribute ("UniformRv", "Access to the underlying UniformRandomVariable",
      	    	StringValue ("ns3::UniformRandomVariable"),
      	    	MakePointerAccessor (&CDSBasedForwarding::m_uniformRandomVariable),
				MakePointerChecker<UniformRandomVariable> ())
          .AddAttribute ("HelloLogEnable", "the switch which can turn on the log on Functions about hello messages",
        		BooleanValue (true),
				MakeBooleanAccessor (&CDSBasedForwarding::m_HelloLogEnable),
				MakeBooleanChecker())
		   ;
	  return tid;
}

CDSBasedForwarding::CDSBasedForwarding():
			HelloInterval (Seconds (1)),
			AllowedHelloLoss (2),
			m_htimer (Timer::CANCEL_ON_DESTROY),
			m_CacheSize(100000),// Cache size can not change. Because if you change the size, the m_interestNonceSeen and m_dataNonceSeen also need to change. It is really unnecessary
			m_dataSignatureSeen(m_CacheSize),
			m_interestNonceSeen(5000),
			m_nb (HelloInterval),
			m_running(false),
			m_runningCounter(0),
			m_HelloLogEnable(true),
			m_state(dominatee),
			m_iClearCount(0)
{	

	// TODO Auto-generated constructor stub
	m_htimer.SetFunction (&CDSBasedForwarding::HelloTimerExpire, this);
	m_nb.SetCallback (MakeCallback (&CDSBasedForwarding::FindBreaksLinkToNextHop, this));
}

CDSBasedForwarding::~CDSBasedForwarding()
{
	// TODO Auto-generated destructor stub
}

void CDSBasedForwarding::Start()
{
	NS_LOG_FUNCTION (this);
	if(!m_runningCounter)
	{
		m_running = true;
		m_offset = MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100));
		m_htimer.Schedule(m_offset);
		m_nb.ScheduleTimer();
	}
	//CreateInterestPacket();
	m_runningCounter++;
}


void CDSBasedForwarding::Stop()
{
	NS_LOG_FUNCTION (this);
	if(m_runningCounter)
		m_runningCounter--;
	else
		return;

	if(m_runningCounter)
		return;
	m_running = false;
	m_htimer.Cancel();
	m_nb.CancelTimer();
}


Ptr<Packet> CDSBasedForwarding::GetNrPayload(HeaderHelper::Type type, Ptr<const Packet> srcPayload,uint32_t forwardId, const Name& dataName/* = *((Name*)NULL)*/)
{
	NS_LOG_INFO("Get nr payload, type:"<<type);
	Ptr<Packet> nrPayload = Create<Packet>(*srcPayload);
	std::vector<uint32_t> priorityList;
	string m_nrtree_str="";
	switch (type)
	{
	case HeaderHelper::INTEREST_NDNSIM:
		{
			//priorityList = GetPriorityList();
			//兴趣包才设置兴趣树的序列化，加入到header
			//m_nrtree_str = m_nrtree->serialize_noId();
			break;
		}
	default:
		{
			NS_ASSERT_MSG(false, "unrecognize packet type");
			break;
		}
	}

		const double& x = m_sensor->getX();
		const double& y = m_sensor->getY();
		ndn::nrndn::nrHeader nrheader(m_node->GetId(), x, y, priorityList);
		//设置信息,设置兴趣树
		//nrheader.setTree(m_nrtree_str);
		nrheader.setForwardId(forwardId);
		nrPayload->AddHeader(nrheader);
	return nrPayload;
}


void  CDSBasedForwarding::OnInterest_application(Ptr<Interest> interest)
{
	//consumer产生兴趣包，在路由层进行转发
	NS_LOG_DEBUG("Get interest packet from APPLICATION");
	// This is the source interest from the upper node application (eg, nrConsumer) of itself
	// 1.Set the payload
	interest->SetPayload(GetNrPayload(HeaderHelper::INTEREST_NDNSIM,interest->GetPayload(),999999999));

	interest->SetScope(99999);	// The flag indicate it is hello message
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	uint32_t nodeId;
	//uint32_t seq;
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader( nrheader);
	// 2. record the Interest Packet
	//m_interestNonceSeen.Put(interest->GetNonce(),true);
	//m_myInterest[interest->GetNonce()]=Simulator::Now().GetSeconds();
	// 3. Then forward the interest packet directly
	Simulator::Schedule(MilliSeconds(m_uniformRandomVariable->GetInteger(0,100)),
						&CDSBasedForwarding::SendInterestPacket,this,interest);	
}




void CDSBasedForwarding::OnInterest(Ptr<Face> face, Ptr<Interest> interest)
{
	if(!m_running) return;
	/*if(INTEREST_MESSAGE == interest->GetScope())
	{
		cout<<m_node->GetId()<<"发送兴趣包"<<Simulator::Now().GetSeconds()<<endl;
		getchar();
		OnInterest_application( interest);
		return;
	}
	else */if(HELLO_MESSAGE==interest->GetScope())
	{
		ProcessHello(interest);
	}
	if( m_state != dominator )
	{//不是支配者，直接返回
		return;
	}
	
	
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	Ptr<Packet> ReceivedPacket = nrPayload->Copy();
	
	ndn::nrndn::nrHeader nrheader;
	ndn::nrndn::nrHeader mprheader;
	ReceivedPacket->RemoveHeader(nrheader);
	//ReceivedPacket->RemoveHeader(mprheader);

	uint32_t uRecId= nrheader.getSourceId();
	
	//判断是否一跳邻居
	
	vector<uint32_t> OneHopNeighborList;
	const NeighborList& nblist=m_nb.getNb();
	NeighborList::const_iterator nbit;
	for(nbit=nblist.begin();nbit!=nblist.end();++nbit)
		OneHopNeighborList.push_back(nbit->first);
	
	bool isOneHopNb = false;
	for(uint32_t i = 0; i < OneHopNeighborList.size(); ++i)
	{
		if(uRecId == OneHopNeighborList[i])
		{
			isOneHopNb = true;
			break;
		}
	}
	if(!isOneHopNb)
		return;
	
	const vector<string> remoteRoute= ExtractRouteFromName(interest->GetName());
	//cout<<"forwarding.cc "<<m_node->GetId() << "收到"<<uRecId<<endl;
	for(uint32_t i = 0; i < remoteRoute.size(); ++i)
	{
		m_mapInterestLane[remoteRoute[i]] = true;
		//cout<<remoteRoute[i]<<" ";
	}
	
	/*
	for(auto ite = m_mapInterestLane.begin(); ite != m_mapInterestLane.end(); ++ite)
	{
		cout<<ite->first<<" ";
	}
	cout<<endl;
	
	*/
	//getchar();
	
	/*
	else
		NS_ASSERT_MSG(false,
				"Not suppose to receive Interest Packet, except for HELLO message");
	*/
	return;
}

void CDSBasedForwarding::OnData(Ptr<Face> face, Ptr<Data> data)
{
	if(!m_running) return;
	NS_LOG_FUNCTION(this);
	//If the data packet has already been sent, do not proceed the packet
	if (m_dataSignatureSeen.Get(data->GetSignature()))
	{
		NS_LOG_DEBUG(
				"The Data packet has already been sent, do not proceed the packet of "<<data->GetSignature());
		return;
	}

	if ((Face::APPLICATION & face->GetFlags())
			|| dominator == m_state)
	{
		if(Face::APPLICATION & face->GetFlags())
		{
			// This is the source data from the upper node application (eg, nrProducer) of itself
			NS_LOG_DEBUG("Get data packet from APPLICATION");

			Ptr<Packet> payload = Create<Packet>(*data->GetPayload());
			ndn::nrndn::nrHeader nrheader;
			nrheader.setSourceId(m_node->GetId());
			payload->AddHeader(nrheader);
			data->SetPayload(payload);
		}
		
		//检查map中是否存在，对这个是否感兴趣
			
		vector<string> sDataName = ExtractRouteFromName(data->GetName());
		string sOriginalName = sDataName[0];
		
		cout<<"数据包中的路段"<<sOriginalName<<endl;
		
		for(auto ite = m_mapInterestLane.begin(); ite != m_mapInterestLane.end(); ++ite)
		{
			cout<<ite->first<<" ";
		}
		cout<<endl;
		
		//感兴趣才转发
		if( m_mapInterestLane[sOriginalName] )
		{
				cout<<"对这个路感兴趣"<<endl;
		//getchar();
		
		// Forward the data packet directly
		Simulator::Schedule(
				MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100)),
				&CDSBasedForwarding::ForwardDataPacket, this, data);
		}
	}

	NotifyUpperLayer(data);
}

void CDSBasedForwarding::AddFace(Ptr<Face> face)
{
	NS_LOG_FUNCTION(this);
	if (Face::APPLICATION == face->GetFlags())
	{
		NS_LOG_DEBUG(
				"Node "<<m_node->GetId()<<" add application face "<<face->GetId());
		m_inFaceList.push_back(face);
	}
	else
	{
		NS_LOG_DEBUG(
				"Node "<<m_node->GetId()<<" add NOT application face "<<face->GetId());
		m_outFaceList.push_back(face);
	}
}

void CDSBasedForwarding::RemoveFace(Ptr<Face> face)
{
	NS_LOG_FUNCTION(this);
	if(Face::APPLICATION==face->GetFlags())
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" remove application face "<<face->GetId());
		m_inFaceList.erase(find(m_inFaceList.begin(),m_inFaceList.end(),face));
	}
	else
	{
		NS_LOG_DEBUG("Node "<<m_node->GetId()<<" remove NOT application face "<<face->GetId());
		m_outFaceList.erase(find(m_outFaceList.begin(),m_outFaceList.end(),face));
	}
}

bool CDSBasedForwarding::DoPropagateInterest(
		Ptr<Face> inFace, Ptr<const Interest> interest,
		Ptr<pit::Entry> pitEntry)
{
	NS_ASSERT_MSG(false,"Not suppose to use DoPropagateInterest");
	return true;
}

void CDSBasedForwarding::NotifyNewAggregate()
{

	if (m_sensor == 0)
	{
		m_sensor = GetObject<ndn::nrndn::NodeSensor>();
	}

	if (m_node == 0)
	{
		m_node = GetObject<Node>();
	}

	super::NotifyNewAggregate();
}

//Ptr<Packet> CDSBasedForwarding::GetNrPayload()
//{
//}

void
CDSBasedForwarding::ProcessHello(Ptr<Interest> interest)
{
	if(!m_running) return;
	Ptr<const Packet> nrPayload	= interest->GetPayload();
	Ptr<Packet> ReceivedPacket = nrPayload->Copy();
	ndn::nrndn::nrHeader nrheader;
	ndn::nrndn::nrHeader mprheader;
	ReceivedPacket->RemoveHeader(nrheader);
	ReceivedPacket->RemoveHeader(mprheader);

	NS_ASSERT(mprheader.getSourceId() == 0);

	NS_LOG_INFO( "Received HELLO packet from "<<nrheader.getSourceId());
	NS_LOG_INFO("Its neighbor list size:" << nrheader.getPriorityList().size());
	NS_LOG_INFO("MPR list size:" << mprheader.getPriorityList().size());

	//update neighbor list
	m_nb.Update(nrheader.getSourceId(),nrheader.getX(),nrheader.getY(),Time (AllowedHelloLoss * HelloInterval));
	m_2hopNb[nrheader.getSourceId()]	=  nrheader.getPriorityList();
	m_nbMPRList[nrheader.getSourceId()] = mprheader.getPriorityList();
}

bool CDSBasedForwarding::isDuplicatedData(uint32_t id, uint32_t signature)
{
	NS_LOG_FUNCTION (this);
	return m_dataSignatureSeen.Get(signature);
}


void CDSBasedForwarding::DropPacket()
{
	NS_LOG_DEBUG ("Drop Packet");
}

void
CDSBasedForwarding::HelloTimerExpire ()
{
	if(!m_running) return;
	if (m_HelloLogEnable)
		NS_LOG_FUNCTION(this);
	SendHello();

	m_htimer.Cancel();
	Time base(HelloInterval - m_offset);
	m_offset = MilliSeconds(m_uniformRandomVariable->GetInteger(0, 100));
	m_htimer.Schedule(base + m_offset);
}

void
CDSBasedForwarding::FindBreaksLinkToNextHop(uint32_t BreakLinkNodeId)
{
	 NS_LOG_FUNCTION (this);
}

void
CDSBasedForwarding::SendHello()
{
	if(!m_running) return;
	if (m_HelloLogEnable)
		NS_LOG_FUNCTION(this);

	CalculateMPR();
	SCDSConstructionByID();

	const double& x		= m_sensor->getX();
	const double& y		= m_sensor->getY();
	const string& LaneName=m_sensor->getLane();




	m_iClearCount++;
	if(m_iClearCount >= 3)
	{
		m_iClearCount = 0;
		if (m_mapInterestLane.size() != 0)
		{
			m_mapInterestLane.clear();
			//cout<<"forwarding.cc 3秒清空一次"<<endl;
			//添加自身感兴趣的道路
			const string& LaneName=m_sensor->getLane();
			const vector<string>& route  = m_sensor->getNavigationRoute();
			uint32_t uSearchIdx = 0;
			for(uSearchIdx = 0; uSearchIdx < route.size(); ++uSearchIdx )
			{
				if ( LaneName == route[uSearchIdx])
					break;
			}
			
			m_mapInterestLane[LaneName] = true;
			
			for(; uSearchIdx < route.size(); ++uSearchIdx)
			{
				m_mapInterestLane[route[uSearchIdx]] = true;
			}
		}
	}
	
	const vector<string>& route  = m_sensor->getNavigationRoute();
	uint32_t uSearchIdx = 0;
	for(uSearchIdx = 0; uSearchIdx < route.size(); ++uSearchIdx )
	{
		if ( LaneName == route[uSearchIdx])
			break;
	}
	
	string sName = "/" + LaneName;



	//1.setup name
	Ptr<Name> name = ns3::Create<Name>(sName); //'/'+LaneName);

	//2. setup payload
	vector<uint32_t> OneHopNeighborList;
	const NeighborList& nblist=m_nb.getNb();
	NeighborList::const_iterator nbit;
	for(nbit=nblist.begin();nbit!=nblist.end();++nbit)
		OneHopNeighborList.push_back(nbit->first);

	Ptr<Packet> newPayload	= Create<Packet> ();
	ndn::nrndn::nrHeader mprHeader;
	mprHeader.setPriorityList(m_mpr);
	newPayload->AddHeader(mprHeader);

	ndn::nrndn::nrHeader nrheader;
	nrheader.setX(x);
	nrheader.setY(y);
	nrheader.setSourceId(m_node->GetId());
	nrheader.setPriorityList(OneHopNeighborList);
	newPayload->AddHeader(nrheader);

	NS_LOG_INFO ("Send neighbor list size:"<< OneHopNeighborList.size());

	//3. setup interest packet
	Ptr<Interest> interest	= Create<Interest> (newPayload);
	interest->SetScope(HELLO_MESSAGE);	// The flag indicate it is hello message
	interest->SetName(name); //interest name is lane;


	//4. send the hello message
	SendInterestPacket(interest);

}

vector<string> CDSBasedForwarding::ExtractRouteFromName(const Name& name)
{
	// Name is in reverse order, so reverse it again
	// eg. if the navigation route is R1-R2-R3, the name is /R3/R2/R1
	vector<string> result;
	Name::const_reverse_iterator it;
	for(it=name.rbegin();it!=name.rend();++it)
		result.push_back(it->toUri());
	return result;
}

void CDSBasedForwarding::ForwardDataPacket(Ptr<Data> src)
{
	if(!m_running) return;
	NS_LOG_DEBUG ("Forward a data packet");
	// 1. record the Data Packet(only record the forwarded packet)
	m_dataSignatureSeen.Put(src->GetSignature(),true);
	
	// 2. Send the data Packet. Already wait, so no schedule
	Ptr<Data> data = src;
	SendDataPacket(data);

	Ptr<const Packet> nrPayload	= data->GetPayload();
	ndn::nrndn::nrHeader nrheader;
	nrPayload->PeekHeader( nrheader);
	uint32_t nodeId = nrheader.getSourceId();
	ndn::nrndn::nrUtils::IncreaseForwardCounter(nodeId,data->GetSignature());
}


void CDSBasedForwarding::SendDataPacket(Ptr<Data> data)
{
	if(!m_running) return;
	//NS_ASSERT_MSG(false,"CDSBasedForwarding::SendDataPacket");
	vector<Ptr<Face> >::iterator fit;
	for (fit = m_outFaceList.begin(); fit != m_outFaceList.end(); ++fit)
	{
		(*fit)->SendData(data);
		ndn::nrndn::nrUtils::AggrateDataPacketSize(data);
	}
}

std::unordered_set<uint32_t> CDSBasedForwarding::converVectorList(
		const std::vector<uint32_t>& list)
{
	std::unordered_set<uint32_t> result;
	std::vector<uint32_t>::const_iterator it;
	for(it=list.begin();it!=list.end();++it)
		result.insert(*it);
	return result;
}



void CDSBasedForwarding::NotifyUpperLayer(Ptr<Data> data)
{
	if(!m_running) return;
	// 1. record the Data Packet received
	m_dataSignatureSeen.Put(data->GetSignature(),true);

	// 2. notify upper layer
	vector<Ptr<Face> >::iterator fit;
	for (fit = m_inFaceList.begin(); fit != m_inFaceList.end(); ++fit)
	{
		//App::OnData() will be executed,
		//including nrProducer::OnData.
		//But none of its business, just ignore
		(*fit)->SendData(data);
	}
}

void CDSBasedForwarding::SendInterestPacket(Ptr<Interest> interest)
{
	if(!m_running) return;
	if(m_HelloLogEnable)
		NS_LOG_FUNCTION (this);

	//    if the node has multiple out Netdevice face, send the interest package to them all
	//    makde sure this is a NetDeviceFace!!!!!!!!!!!1
	vector<Ptr<Face> >::iterator fit;
	for(fit=m_outFaceList.begin();fit!=m_outFaceList.end();++fit)
	{
		(*fit)->SendInterest(interest);
		ndn::nrndn::nrUtils::AggrateInterestPacketSize(interest);
	}
}

void CDSBasedForwarding::CreateInterestPacket()
{
	
	if(!m_running) return;
	m_iClearCount++;
	if(m_iClearCount >= 3)
	{
		m_iClearCount = 0;
		if (m_mapInterestLane.size() != 0)
		{
			m_mapInterestLane.clear();
			//cout<<"forwarding.cc 3秒清空一次"<<endl;
			//添加自身感兴趣的道路
			const string& LaneName=m_sensor->getLane();
			const vector<string>& route  = m_sensor->getNavigationRoute();
			uint32_t uSearchIdx = 0;
			for(uSearchIdx = 0; uSearchIdx < route.size(); ++uSearchIdx )
			{
				if ( LaneName == route[uSearchIdx])
					break;
			}
			
			m_mapInterestLane[LaneName] = true;
			
			for(; uSearchIdx < route.size(); ++uSearchIdx)
			{
				m_mapInterestLane[route[uSearchIdx]] = true;
			}
		}
	}
	
	const double& x		= m_sensor->getX();
	const double& y		= m_sensor->getY();
	const string& LaneName=m_sensor->getLane();
	const vector<string>& route  = m_sensor->getNavigationRoute();
	uint32_t uSearchIdx = 0;
	for(uSearchIdx = 0; uSearchIdx < route.size(); ++uSearchIdx )
	{
		if ( LaneName == route[uSearchIdx])
			break;
	}
	
	string sName = "/" + LaneName;
	
	for(; uSearchIdx < route.size(); ++uSearchIdx)
	{
		sName += "/" + route[uSearchIdx];
	}
	
	//1.setup name
	Ptr<Name> name = ns3::Create<Name>(sName);

	//2. setup payload
	vector<uint32_t> OneHopNeighborList;
	const NeighborList& nblist=m_nb.getNb();
	NeighborList::const_iterator nbit;
	for(nbit=nblist.begin();nbit!=nblist.end();++nbit)
		OneHopNeighborList.push_back(nbit->first);
	//cout<<"OneHopNeighborList size:"<<OneHopNeighborList.size()<<endl;
	//cout<<"m_mpr size"<<m_mpr.size()<<endl;
	Ptr<Packet> newPayload	= Create<Packet> ();
	
	ndn::nrndn::nrHeader mprHeader;
	//mprHeader.setPriorityList(m_mpr);
	//newPayload->AddHeader(mprHeader);


	ndn::nrndn::nrHeader nrheader;
	nrheader.setX(x);
	nrheader.setY(y);
	nrheader.setSourceId(m_node->GetId());
	//nrheader.setPriorityList(OneHopNeighborList);
	newPayload->AddHeader(nrheader);
	
	//生成兴趣包
	Ptr<Interest> pInterest	= Create<Interest> (newPayload);
	pInterest->SetScope(INTEREST_MESSAGE);	// The flag indicate it is hello message
	pInterest->SetName(name); //interest name is lane;
	
	
	//cout<<m_node->GetId()<<"生成并发送兴趣包"<<Simulator::Now().GetSeconds()<<endl;
	//getchar();
	//发送兴趣包
	SendInterestPacket(pInterest);
	
	float fDelay = 0 ;//m_uniformRandomVariable->GetInteger(0,100) * 1.0 / 1000.0;
	
	//1秒后继续调用这个函数//
	Simulator::Schedule (Seconds (1.0 + fDelay), &CDSBasedForwarding::CreateInterestPacket, this);
}

void CDSBasedForwarding::CalculateMPR()
{
	if(!m_running) return;
	if (m_HelloLogEnable)
		NS_LOG_FUNCTION(this);

	m_mpr.clear();

	//multimap<double,vector<Neighbor>::const_iterator> sl;

	multimap<uint32_t,uint32_t> sl;

	set<uint32_t> nb2hop;//Store 2 hop neighbor
	set<uint32_t> nb1hop;//Store 1 hop neighbor
	set<uint32_t> tempnb2hop;
	set<uint32_t> tempuncovered;


	 const NeighborList& nb = m_nb.getNb();
	 NeighborList::const_iterator it;

	//1. Get the 1-hop neighborlist
	for(it = nb.begin();it!=nb.end();++it)
	{
		nb1hop.insert(it->first);
		NS_LOG_INFO("Insert "<<it->first<<" into nb1hop");
	}

	//Calculate the stable time for each 1-hop neighbor
	for(it = nb.begin();it!=nb.end();++it)
	{
		tempnb2hop.clear();
		tempuncovered.clear();
		std::vector<uint32_t>::const_iterator it2;
		NS_ASSERT(m_2hopNb.count(it->first));
		//NS_LOG_INFO(it->first<< "'s 2 hop neighbor list size "<<m_2hopNb[it->first].size());
		for (it2 = m_2hopNb[it->first].begin();
				it2 != m_2hopNb[it->first].end(); ++it2)
		{
			NS_LOG_INFO("Insert "<<*it2<<" into tempnb2hop");
			tempnb2hop.insert(*it2);
			nb2hop.insert(*it2);
		}
		tempnb2hop.erase(m_node->GetId());
		set_difference(tempnb2hop.begin(), tempnb2hop.end(), nb1hop.begin(),
				nb1hop.end(), inserter(tempuncovered, tempuncovered.begin()));

		uint32_t size = tempuncovered.size();

		if(size == 0)
			NS_LOG_INFO("Insert a ZERO 2-hop neighbor covered neighbor");

		sl.insert(pair<uint32_t,uint32_t>(size,it->first));
	}

	//Remove ip address itself
	nb2hop.erase(m_node->GetId());

	//Remove neighbor in 2hop neighbor list which is already 1hop neighbor
	set<uint32_t> uncovered;
	set_difference(nb2hop.begin(), nb2hop.end(), nb1hop.begin(), nb1hop.end(), inserter(uncovered,uncovered.begin()));

	if(uncovered.size() == 0)
		NS_LOG_INFO("Uncovered 2 hop neighbor is ZERO");

	//Include the nodes into the mpr list to cover 2-hop neighbors
	multimap<uint32_t,uint32_t>::reverse_iterator rit;
	for(rit = sl.rbegin();rit!=sl.rend();++rit)
	{
		if(!uncovered.empty())
		{
			vector<uint32_t> addr2hop =  m_2hopNb[rit->second];
			set<uint32_t> result;
			sort(addr2hop.begin(),addr2hop.end());

			set_difference(uncovered.begin(), uncovered.end(), addr2hop.begin(),
					addr2hop.end(), inserter(result, result.begin()));

			if(uncovered.size() != result.size())
			{
				m_mpr.push_back(rit->second);
			}
			uncovered = result;
		}
		else
		{
			break;
		}
	}

	if (m_HelloLogEnable)
	{
		std::vector<uint32_t>::iterator vit;
		std::ostringstream os;
		os << "MPR list is:\t";
		for (vit = m_mpr.begin(); vit != m_mpr.end(); ++vit)
			os << " " << *vit;
		NS_LOG_DEBUG(os.str());
	}
	return;
}

void CDSBasedForwarding::SCDSConstructionByID()
{
	if(!m_running) return;
	if (m_HelloLogEnable)
		NS_LOG_FUNCTION(this);

	const NeighborList& nb = m_nb.getNb();
	NeighborList::const_iterator cit;
	//NeighborList::const_iterator minidit;

	uint32_t minid=std::numeric_limits<uint32_t>::max();

	for(cit = nb.begin(); cit!=nb.end();++cit)
	{
		if(cit->first < minid)
		{
			minid = cit->first;
		}
	}

	uint32_t local = m_node->GetId();

	if (local < minid) {
		m_state = dominator;
		NS_LOG_DEBUG("Leader:Change status to dominator for LowestID");
	}
	else
	{
		NS_ASSERT(m_nbMPRList.count(minid));
		vector<uint32_t>::const_iterator mprit =
				find(m_nbMPRList[minid].begin(),m_nbMPRList[minid].end(),local);

		if(mprit==m_nbMPRList[minid].end())
		{
			m_state = dominatee;
			NS_LOG_INFO("Ordinary:Change status to dominatee");
		}
		else
		{
			m_state = dominator;
			NS_LOG_DEBUG("Leader:Change status to dominator for MPR of LID");
		}
	}
}

} /* namespace nrndn */
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */
