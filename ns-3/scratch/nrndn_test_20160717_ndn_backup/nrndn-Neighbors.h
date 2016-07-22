/*
 * nrndn-Neighbors.h
 *
 *  Created on: Jan 29, 2015
 *      Author: chen
 */

#ifndef NRNDN_NEIGHBORS_H_
#define NRNDN_NEIGHBORS_H_

#include "ns3/simulator.h"
#include "ns3/timer.h"
#include "ns3/callback.h"

#include <unordered_map>
#include <vector>
#include <string>


namespace ns3 {
namespace ndn {
namespace fw {
namespace nrndn {

class Neighbors {
public:
	Neighbors(Time delay);
	virtual ~Neighbors();

	struct Neighbor
	{
		double m_x;
		double m_y;
	    Time m_expireTime;
	    Neighbor (const double& x, const double& y,const Time& t) :
	    	m_x (x),m_y(y),m_expireTime (t)
	    {
	    }
	};
	  /// Get 1hop Neighbor list
	  const std::unordered_map<uint32_t, Neighbor>& getNb() const { return m_nb;	}

	  /// Return expire time for neighbor node with identifier id, if exists, else return 0.
	  Time GetExpireTime (uint32_t id);
	  /// Check that node with identifier id  is neighbor
	  bool IsNeighbor (uint32_t id);
	  /// Update expire time for entry with identifier id, if it exists, else add new entry
	  void Update(const uint32_t& id, const double& x,const double& y,const Time& expire);
	  /// Remove all expired entries
	  void Purge ();
	  /// Schedule m_ntimer.
	  void ScheduleTimer ();
	  /// Remove all entries
	  void Clear () { m_nb.clear (); }
	  /// Cancle m_ntimer.
	  void CancelTimer();

	  ///\name Handle link failure callback
	  //\{
	  void SetCallback (Callback<void, uint32_t> cb) { m_handleLinkFailure = cb; }
	  Callback<void, uint32_t> GetCallback () const { return m_handleLinkFailure; }
	  //\}
	private:
	  /// link failure callback
	  Callback<void, uint32_t> m_handleLinkFailure;
	  /// Timer for neighbor's list. Schedule Purge().
	  Timer m_ntimer;
	  /// hashset of entries, use node id as keys
	  std::unordered_map<uint32_t,Neighbor> m_nb;
};

} /* namespace nrndn */
} /* namespace fw */
} /* namespace ndn */
} /* namespace ns3 */
#endif /* NRNDN_NEIGHBORS_H_ */
