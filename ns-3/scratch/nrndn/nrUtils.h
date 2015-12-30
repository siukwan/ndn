/*
 * nrUtils.h
 *
 *  Created on: Dec 29, 2014
 *      Author: chen
 */

#ifndef NRUTILS_H_
#define NRUTILS_H_

#include "ns3/core-module.h"
#include "ns3/ndn-wire.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <utility>

namespace ns3
{
namespace ndn
{
namespace nrndn
{

/**
 *
 * The class implements the universal Method for nodes to use the navigation route
 * And some global variables
 *
 */
template <class T>
double GetAverage(std::vector<T>& v)
{
	if(v.empty())
		return 0;
	typename std::vector<T>::iterator rit;
	double average = 0;
	double sum = 0;
	for(rit=v.begin();rit!=v.end();++rit)
		sum += *rit;
	average = sum / v.size();
	return average;
}

struct MsgAttribute
{
	MsgAttribute():
		NodeSize(0),
		InterestedNodeSize(0),
		InterestedNodeReceiveCounter(0),
		DisinterestedNodeReceiveCounter(0){}
	uint32_t NodeSize;
	uint32_t InterestedNodeSize;
	uint32_t InterestedNodeReceiveCounter;
	uint32_t DisinterestedNodeReceiveCounter;
};

class nrUtils
{//常用工具包
public:
	nrUtils();
	~nrUtils();

	typedef std::unordered_map<std::string, uint32_t> AppIndexType;
	typedef std::unordered_map<uint32_t, std::unordered_map<uint32_t,std::vector<double> > > TransmissionDelayMap;
	typedef std::unordered_map<uint32_t, std::unordered_map<uint32_t,uint32_t > > ForwardCounterMap;
	typedef std::unordered_map<uint32_t, std::unordered_map<uint32_t,MsgAttribute> >    MessageArrivalMap;

	//1. Arrival Condition
	static MessageArrivalMap msgArrivalCounter;
	static std::pair<uint32_t, uint32_t> GetNodeSizeAndInterestNodeSize(uint32_t id,uint32_t signature, const std::string& lane);
	static void SetNodeSize(uint32_t id, uint32_t signature,uint32_t nodesize);
	static void SetInterestedNodeSize(uint32_t id,uint32_t signature,uint32_t InterestedNodeSize);
	static void IncreaseInterestedNodeCounter(uint32_t id,uint32_t signature);
	static void IncreaseDisinterestedNodeCounter(uint32_t id,uint32_t signature);

	//2. forward times
	static ForwardCounterMap forwardCounter;
	static void IncreaseForwardCounter(uint32_t id,uint32_t signature);
	static ForwardCounterMap interestForwardCounter;
	static void IncreaseInterestForwardCounter(uint32_t id,uint32_t nonce);

	//3. Delay Record
	static TransmissionDelayMap TransmissionDelayRecord;  //\brief TransmissionDelayRecord[nodeid][signature]=Delay time;
	static void InsertTransmissionDelayItem(uint32_t id,uint32_t signature,double delay);
	static double GetAverageTransmissionDelay();

	static double GetAverageArrivalRate();
	static double GetAverageAccurateRate();
	static double GetAverageHitRate();

	/*
	 * @brief get the average forward times
	 * \return (ForwardTimesSum,averageForwardTimes)
	 * */
	static std::pair<uint32_t,double> GetAverageForwardTimes();

	/*
	 * @brief get the average interest packet forward times
	 * \return (InterestForwardTimesSum,averageInterestForwardTimes)
	 * */
	static std::pair<uint32_t,double> GetAverageInterestForwardTimes();

	static double GetAverageDelay();

	//4 . appIndex
	static AppIndexType appIndex;

	static uint32_t ByteSent;
	static uint32_t DataByteSent;
	static uint32_t InterestByteSent;
	static uint32_t HelloByteSent;
	static void AggrateDataPacketSize(Ptr<const Data> data);
	static void AggrateInterestPacketSize(Ptr<const Interest> interest);
};

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NRUTILS_H_ */
