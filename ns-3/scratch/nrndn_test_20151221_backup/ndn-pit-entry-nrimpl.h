/*
 * ndn-pit-entry-nrimpl.h
 *
 *  Created on: Jan 21, 2015
 *      Author: chenyishun
 */


#ifndef NDN_PIT_ENTRY_NRIMPL_H_
#define NDN_PIT_ENTRY_NRIMPL_H_

#include "ns3/ndn-pit-entry.h"
#include "ns3/ndn-pit-entry-incoming-face.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace ns3 {
namespace ndn {

class Pit;

namespace pit {
namespace nrndn{
	
	/**
	 * @ingroup ndn-pit
	 * @brief PIT entry implementation with additional pointers to the underlying container
	 *           It is completely unrelated to the class PIT::Entry
	 */

class EntryNrImpl : public Entry
{
public:
	typedef Entry super;

	EntryNrImpl(Pit &container, Ptr<const Interest> header,Ptr<fib::Entry> fibEntry,Time cleanInterval);
	virtual ~EntryNrImpl();
	
	/**
	 * @brief Remove all the timeout event in the timeout event list
	 *
	 */
	void RemoveAllTimeoutEvent();

	/**
	 * @brief Add `id` to the list of incoming neighbor list(m_incomingnbs)
	 *
	 * @param id Neighbor id to add to the list of incoming neigbhor list
	 * @returns iterator to the added neighbor id
	 */
	std::unordered_set< uint32_t >::iterator
	AddIncomingNeighbors(uint32_t id);

	const std::unordered_set<uint32_t>& getIncomingnbs() const
	{
		return m_incomingnbs;
	}

private:
	void AddNeighborTimeoutEvent(uint32_t id);

	//when the time expire, the incoming neighbor id will be removed automatically
	void CleanExpiredIncomingNeighbors(uint32_t id);
private:
	std::unordered_map< uint32_t,EventId> m_nbTimeoutEvent;///< @brief it is a hashmap that record the timeout event of each neighbor id
	std::unordered_set< uint32_t > 		  m_incomingnbs;///< @brief container for incoming neighbors
	std::string m_interest_name;	
	Time m_infaceTimeout;

};



} // namespace nrndn
} // namespace pit
} // namespace ndn
} // namespace ns3


#endif /* NDN_PIT_ENTRY_NRIMPL_H_ */
