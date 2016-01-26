/*
 * nrUtils.cpp
 *
 *  Created on: Dec 29, 2014
 *      Author: chen
 */

#include "nrUtils.h"
#include "nrProducer.h"

#include "ns3/network-module.h"

namespace ns3
{
namespace ndn
{
namespace nrndn
{

using namespace std;

nrUtils::MessageArrivalMap nrUtils::msgArrivalCounter;
nrUtils::ForwardCounterMap nrUtils::forwardCounter;
nrUtils::ForwardCounterMap nrUtils::interestForwardCounter;
nrUtils::TransmissionDelayMap nrUtils::TransmissionDelayRecord;
nrUtils::AppIndexType nrUtils::appIndex;
uint32_t nrUtils::ByteSent=0;
uint32_t nrUtils::DataByteSent=0;
uint32_t nrUtils::InterestByteSent=0;
uint32_t nrUtils::HelloByteSent=0;


nrUtils::nrUtils()
{
	// TODO Auto-generated constructor stub

}

nrUtils::~nrUtils()
{
	// TODO Auto-generated destructor stub
}

std::pair<uint32_t, uint32_t> nrUtils::GetNodeSizeAndInterestNodeSize(
		uint32_t id, uint32_t signature, const std::string& lane)
{
	uint32_t nodeSize = 0;
	uint32_t interestSize=0;
	NodeContainer c =NodeContainer::GetGlobal();
	NodeContainer::Iterator it;
	for(it=c.Begin();it!=c.End();++it)
	{
		Ptr<Application> app=(*it)->GetApplication(appIndex["ns3::ndn::nrndn::nrProducer"]);
		Ptr<nrndn::nrProducer> producer = DynamicCast<nrndn::nrProducer>(app);
		NS_ASSERT(producer);
		if(producer->IsActive())
			++nodeSize;
		if(producer->IsInterestLane(lane))
			++interestSize;
	}
	return std::pair<uint32_t, uint32_t>(nodeSize,interestSize);
}
void nrUtils::SetNodeSize(uint32_t id, uint32_t signature,uint32_t nodesize)
{
	msgArrivalCounter[id][signature].NodeSize=nodesize;
}

void nrUtils::SetInterestedNodeSize(uint32_t id,
		uint32_t signature, uint32_t InterestedNodeSize)
{
	msgArrivalCounter[id][signature].InterestedNodeSize=InterestedNodeSize;
}

void nrUtils::IncreaseInterestedNodeCounter(uint32_t id,
		uint32_t signature)
{
	msgArrivalCounter[id][signature].InterestedNodeReceiveCounter++;
}

void nrUtils::IncreaseDisinterestedNodeCounter(uint32_t id,
		uint32_t signature)
{
	msgArrivalCounter[id][signature].DisinterestedNodeReceiveCounter++;
}

void nrUtils::IncreaseForwardCounter(uint32_t id,
		uint32_t signature)
{
	forwardCounter[id][signature]++;
}

void nrUtils::IncreaseInterestForwardCounter(uint32_t id, uint32_t nonce)
{
	interestForwardCounter[id][nonce]++;
}

void nrUtils::InsertTransmissionDelayItem(uint32_t id,
		uint32_t signature, double delay)
{
	TransmissionDelayRecord[id][signature].push_back(delay);
}



double nrUtils::GetAverageArrivalRate()
{
	MessageArrivalMap::iterator it1;
	std::unordered_map<uint32_t,MsgAttribute>::iterator it2;
	vector<double>::iterator rit;
	vector<double> result;

	for (it1 = msgArrivalCounter.begin(); it1 != msgArrivalCounter.end(); ++it1)
	{
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{

			const MsgAttribute& msgAttr = it2->second;
			double size = msgAttr.NodeSize;
			double receiveNodesNum = msgAttr.InterestedNodeReceiveCounter + msgAttr.DisinterestedNodeReceiveCounter;
			if(size != 0)
			{
				double arrivalRate = receiveNodesNum / size ;
				result.push_back(arrivalRate);
			}

		}
	}

	return GetAverage(result);
}

double nrUtils::GetAverageAccurateRate()
{
	MessageArrivalMap::iterator it1;
	std::unordered_map<uint32_t,MsgAttribute>::iterator it2;
	vector<double> result;

	for (it1 = msgArrivalCounter.begin(); it1 != msgArrivalCounter.end(); ++it1)
	{
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			const MsgAttribute& msgAttr = it2->second;
			double interestedNodeNum = msgAttr.InterestedNodeReceiveCounter;
			double receiveNodesNum = msgAttr.InterestedNodeReceiveCounter + msgAttr.DisinterestedNodeReceiveCounter;
			if(receiveNodesNum != 0)
			{
				double accurateRate = interestedNodeNum / receiveNodesNum ;
				result.push_back(accurateRate);
			}

		//	cout<<"From msgArrivalCounter, ID="<<it1->first<<" Signature="<<it2->first<<
		//			" interest node="<< msgAttr.InterestedNodeReceiveCounter<<
		//            " disinterest node="<<msgAttr.DisinterestedNodeReceiveCounter<<endl;

		}
	}

	return GetAverage(result);
}


double nrUtils::GetAverageDisinterestedRate()
{//不感兴趣的接收到兴趣包的车辆数目占所有收到兴趣包的车的比例
	MessageArrivalMap::iterator it1;
	std::unordered_map<uint32_t,MsgAttribute>::iterator it2;
	vector<double> result;

	for (it1 = msgArrivalCounter.begin(); it1 != msgArrivalCounter.end(); ++it1)
	{
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			const MsgAttribute& msgAttr = it2->second;
			double disinterestedNodeNum = msgAttr.DisinterestedNodeReceiveCounter;
			double receiveNodesNum = msgAttr.InterestedNodeReceiveCounter + msgAttr.DisinterestedNodeReceiveCounter;
			if(receiveNodesNum != 0)
			{
				double accurateRate = disinterestedNodeNum / receiveNodesNum ;
				result.push_back(accurateRate);
			}

		//	cout<<"From msgArrivalCounter, ID="<<it1->first<<" Signature="<<it2->first<<
		//			" interest node="<< msgAttr.InterestedNodeReceiveCounter<<
		//            " disinterest node="<<msgAttr.DisinterestedNodeReceiveCounter<<endl;

		}
	}

	return GetAverage(result);
}

double nrUtils::GetAverageHitRate()
{
	MessageArrivalMap::iterator it1;
	std::unordered_map<uint32_t,MsgAttribute>::iterator it2;
	vector<double> result;

	for (it1 = msgArrivalCounter.begin(); it1 != msgArrivalCounter.end(); ++it1)
	{
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{

			const MsgAttribute& msgAttr = it2->second;
			double interestedNodeNum = msgAttr.InterestedNodeReceiveCounter;
			double interestedNodeSum = msgAttr.InterestedNodeSize;
			if(interestedNodeSum!=0)
			{
				double hitRate = interestedNodeNum / interestedNodeSum;
				result.push_back(hitRate);
			}

		}
	}

	return GetAverage(result);
}

pair<uint32_t,double> nrUtils::GetAverageForwardTimes()
{
	ForwardCounterMap::iterator it1;
	std::unordered_map<uint32_t,uint32_t >::iterator it2;
	uint32_t forwardTimes=0;
	double messageNum=0;

	for (it1 = forwardCounter.begin(); it1 != forwardCounter.end(); ++it1)
	{
		messageNum += it1->second.size();
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			forwardTimes += it2->second;
		}
	}

	if(messageNum == 0)
		return make_pair(forwardTimes,0);

	double average = forwardTimes / messageNum;

	return make_pair(forwardTimes,average);
}

pair<uint32_t,double> nrUtils::GetAverageInterestForwardTimes()
{
	ForwardCounterMap::iterator it1;
	std::unordered_map<uint32_t,uint32_t >::iterator it2;
	uint32_t forwardTimes=0;
	double messageNum=0;

	for (it1 = interestForwardCounter.begin(); it1 != interestForwardCounter.end(); ++it1)
	{
		messageNum += it1->second.size();
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			forwardTimes += it2->second;
		}
	}

	if(messageNum == 0)
		return make_pair(forwardTimes,0);

	double average = forwardTimes / messageNum;

	return make_pair(forwardTimes,average);
}


double nrUtils::GetAverageDelay()
{
	TransmissionDelayMap::iterator it1;
	std::unordered_map<uint32_t,std::vector<double> >::iterator it2;
	vector<double> result;

	for (it1 = TransmissionDelayRecord.begin(); it1 != TransmissionDelayRecord.end(); ++it1)
	{
		for (it2 = it1->second.begin(); it2 != it1->second.end(); ++it2)
		{
			double averageDelayOfOneMsg=GetAverage(it2->second);
			result.push_back(averageDelayOfOneMsg);
		}
	}

	return GetAverage(result);
}

double nrUtils::GetAverageTransmissionDelay()
{
	TransmissionDelayMap::iterator it1;
	unordered_map<uint32_t,vector<double> >::iterator it2;
	vector<double>::iterator it3;
	double SumSize=0;
	double SumDelay=0;
	for(it1=TransmissionDelayRecord.begin();it1!=TransmissionDelayRecord.end();++it1)
	{
		for(it2=it1->second.begin();it2!=it1->second.end();++it2)
		{
			SumSize+=it2->second.size();
			for(it3=it2->second.begin();it3!=it2->second.end();++it3)
				SumDelay+=(*it3);
		}
	}

	NS_ASSERT(SumSize != 0);

	return SumDelay/SumSize;
}

void nrUtils::AggrateDataPacketSize(Ptr<const Data> data)
{
	Ptr<Packet> packet = Wire::FromData (data);
	uint32_t size = packet->GetSize();
	ByteSent += size;
	DataByteSent+= size;
}

void nrUtils::AggrateInterestPacketSize(Ptr<const Interest> interest)
{
	Ptr<Packet> packet = Wire::FromInterest (interest);
	uint32_t size = packet->GetSize();
	ByteSent += size;
	if(2==interest->GetScope())//Hello message
		HelloByteSent += size;
	else
		InterestByteSent += size;
}

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
