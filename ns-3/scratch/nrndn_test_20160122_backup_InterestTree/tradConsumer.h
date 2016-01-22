/*
 * tradConsumer.h
 *
 *  Created on: Mar 2, 2015
 *      Author: chen
 */

#ifndef TRADCONSUMER_H_
#define TRADCONSUMER_H_

#include "ns3/ndn-app.h"
#include "ns3/ndn-name.h"

namespace ns3
{
namespace ndn
{
namespace nrndn
{

/*
 * @brief	implement of traditional consumer, it will act like a normal Wifi
 * 			receiver, will not express any interest, just receive data
 * */
class tradConsumer: public App
{
public:

	static TypeId GetTypeId();
	tradConsumer();
	virtual ~tradConsumer();

protected:
	// inherited from App base class. Originally they were private
	virtual void
	StartApplication();    ///< @brief Called at time specified by Start

	virtual void
	StopApplication();     ///< @brief Called at time specified by Stop

	virtual void
	OnData(Ptr<const Data> contentObject);

	 bool IsInterestData(const Name& name);
private:
	typedef App super;
	uint32_t m_virtualPayloadSize;
};

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* TRADCONSUMER_H_ */
