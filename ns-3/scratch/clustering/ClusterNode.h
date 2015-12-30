/*
 * ClusterNode.h
 *
 *  Created on: Jul 8, 2013
 *      Author: cys
 */

#ifndef CLUSTERNODE_H_
#define CLUSTERNODE_H_

#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/callback.h"
#include "nrcHeader.h"
#include <set>

namespace ns3
{
namespace nrc
{
class ClusterNode
{
public:
	ClusterNode();
	virtual ~ClusterNode();

	virtual void run()=0;

	virtual void RecvRequest(Ptr<Packet> p, Ipv4Address receiver,Ipv4Address src)=0;

	const Callback<void, NodeState>& getHandleStateChange() const
	{
		return m_handleStateChange;
	}

	void setHandleStateChange(
			const Callback<void, NodeState>& handleStateChange)
	{
		m_handleStateChange = handleStateChange;
	}

protected:
	  /// state change callback
	  Callback<void, NodeState> m_handleStateChange;
};

struct ClusterHead
{
	ClusterHead():m_cmlistemptytime(Simulator::Now()){}
	std::set<Ipv4Address> m_cluster_memberlist;
	std::multimap<std::string,Ipv4Address>  m_leave_pos_cm;
	std::map<std::string,Ipv4Address>  m_fch_list;
	Time m_cmlistemptytime;
//	map<Ipv4Address,Ipv4Address> m_mhpair;
	std::map<Ipv4Address,std::map<double, Ipv4Address> > m_nchlist;
};

struct ClusterMember
{
	ClusterMember():ClusterGateWay(false){}
	Ipv4Address ch;
	bool ClusterGateWay;
};

struct Undecide
{
//Undecide();
//~Undecide();
	Ipv4Address tojoin;

	// if a UN delete a ch from neighor_ch_list,
	// it tells that the ch is not suitable for the node,
	// so it add the ch into black list so
	std::set<Ipv4Address>  m_ch_blacklist;

};

struct FutureClusterHead
{
	std::vector<Ipv4Address> m_cluster_memberlist;
	std::set<Ipv4Address> m_ch_blacklist;
	Ipv4Address tojoin;
//	map<Ipv4Address,Ipv4Address> m_mhpair;
};

struct NodeData
{
	NodeData():ch(NULL),cm(NULL),un(NULL),fch(NULL){}
	NodeData(NodeState state);

	~NodeData();

	ClusterHead   *ch;
	ClusterMember *cm;
	Undecide      *un;
	FutureClusterHead *fch;
};

struct AddTimePair
{
	Ipv4Address add;
	double time;
};

} /* namespace nrc */
} /* namespace ns3 */

#endif /* CLUSTERNODE_H_ */
