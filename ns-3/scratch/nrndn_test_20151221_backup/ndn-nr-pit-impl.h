/*
 * ndn-nr-pit-impl.h
 *
 *  Created on: Jan 20, 2015
 *      Author: chen
 */

#ifndef NDN_NR_PIT_IMPL_H_
#define NDN_NR_PIT_IMPL_H_

#include "ns3/ndn-pit.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#include "ns3/ndn-interest.h"
#include "ns3/ndn-data.h"
#include "ns3/ndn-forwarding-strategy.h"
#include "ns3/ndn-name.h"

#include "NodeSensor.h"

#include <vector>


namespace ns3
{
namespace ndn
{
namespace pit
{
namespace nrndn
{

/**
 * @ingroup ndn-pit
 * @brief Class implementing Pending Interests Table,
 * 		  with navigation route customize
 */
class NrPitImpl	: public Pit				
{
public:
  /**
   * \brief Interface ID
   *
   * \return interface ID
   */
  static TypeId GetTypeId ();

  /**
   * \brief PIT constructor
   */
  NrPitImpl();

  /**
   * \brief Destructor
   */
  virtual ~NrPitImpl();

  // inherited from Pit

  //abandon
  virtual Ptr<Entry>
  Lookup (const Data &header);

  //abandon
  virtual Ptr<Entry>
  Lookup (const Interest &header);

  //This prefix is different from the format of interest's name
  virtual Ptr<Entry>
  Find (const Name &prefix);

  //abandon
  virtual Ptr<Entry>
  Create (Ptr<const Interest> header);

  //replace NrPitImpl::Create
  bool
  InitializeNrPitEntry ();

  //abandon
  virtual void
  MarkErased (Ptr<Entry> entry);

  virtual void
  Print (std::ostream &os) const;

  virtual uint32_t
  GetSize () const;

  virtual Ptr<Entry>
  Begin ();

  virtual Ptr<Entry>
  End ();

  virtual Ptr<Entry>
  Next (Ptr<Entry>);

  /**
   * This function update the pit using Interest packet
   * not simply add the name into the pit
   * multi entry of pit will be operated
   */
  bool UpdatePit(const std::vector<std::string>& route,const uint32_t& id);


  void laneChange(std::string oldLane, std::string newLane);

  //小锟添加，2015-8-23
  std::string uriConvertToString(std::string str);
protected:
  // inherited from Object class
  virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object
  virtual void DoDispose (); ///< @brief Do cleanup
  virtual void DoInitialize(void);



private:
  Time m_cleanInterval;
  Ptr<ForwardingStrategy>		m_forwardingStrategy;
  std::vector<Ptr<Entry> >		m_pitContainer;
  Ptr<ndn::nrndn::NodeSensor>	m_sensor;

  friend class EntryNrImpl;


  Ptr<Fib> m_fib; ///< \brief Link to FIB table(Useless, Just for compatibility)
};


}/* namespace nrndn */
} /* namespace pit */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NDN_NR_PIT_IMPL_H_ */
