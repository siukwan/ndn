/*
 * SumoMobility.cc
 *
 *  Created on: Dec 31, 2014
 *      Author: chen
 */


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/application.h"
#include "ns3/SumoMobility.h"

namespace ns3
{
namespace vanetmobility
{
namespace sumomobility
{
NS_OBJECT_ENSURE_REGISTERED (SumoMobility);

using namespace std;


SumoMobility::SumoMobility(std::string netxmlpath,std::string routexmlpath,std::string fcdxmlpath):
		netxmlpath(netxmlpath),routexmlpath(routexmlpath),fcdxmlpath(fcdxmlpath),readTotalTime(0)
{
	// TODO Auto-generated constructor stub
	LoadTraffic();
	InitializeCoordinateToLane();
}

SumoMobility::~SumoMobility()
{
	// TODO Auto-generated destructor stub
}

TypeId SumoMobility::GetTypeId()
{
	  static TypeId tid = TypeId ("ns3::vanetmobility::sumomobility::SumoMobility")
	    .SetParent<Object> ()
	  ;
	  return tid;
}

void SumoMobility::LoadTraffic()
{
	roadmap.LoadNetXMLFile(netxmlpath.data());
	vl.LoadRouteXML(routexmlpath.data());
	vl.LoadFCDOutputXML(fcdxmlpath.data());
}

double SumoMobility::GetStartTime(uint32_t id)
{
	return vl.getVehicles()[id].trace.front().time;

}

double SumoMobility::GetStopTime(uint32_t id)
{
	return vl.getVehicles()[id].trace.back().time;
}

void SumoMobility::Install()
{

	MobilityHelper mobility;
	Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

	int siu_i=0;
	//Initialize the position of each vehicle, although it may appear later, it is placed once the simulation starts
	for(vector<Vehicle>::const_iterator vehicle =vl.getVehicles().begin();vehicle!=vl.getVehicles().end();vehicle++)
	{
		std::cout<<"初始化车辆位置："<<siu_i<<"   "<<std::endl;
		double pos_x =(*vehicle).trace.front().x;
		double pos_y =(*vehicle).trace.front().y;
		positionAlloc->Add(Vector(pos_x,pos_y,0.0));
		siu_i++;
	}
	mobility.SetPositionAllocator (positionAlloc);
	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (NodeContainer::GetGlobal());

	///setup way point mobility
	bool lazyNotify = true;
	ObjectFactory mobilityFactory;
	std::deque<Waypoint> waypoints;
	mobilityFactory.SetTypeId("ns3::WaypointMobilityModel");
	mobilityFactory.Set("LazyNotify", BooleanValue(lazyNotify));
	std::vector<Ptr<MobilityModel> > mobilityStack;

	double maxTime = 0;
	// Populate the vector of mobility models, each model for a vehicle
	for (vector<Vehicle>::const_iterator vehicle =vl.getVehicles().begin();vehicle!=vl.getVehicles().end();vehicle++)
	{
		double end_time=0.0;
		// Create a new mobility model.
		Ptr<MobilityModel> model = mobilityFactory.Create()->GetObject<MobilityModel>();
		// Add this mobility model to the stack.
		Ptr<WaypointMobilityModel> waypointmodel = model->GetObject<WaypointMobilityModel>();
		//Add a initial position(0,0,10000)
		waypointmodel->AddWaypoint(Waypoint(Seconds((*((*vehicle).trace).begin()).time-1.0),Vector(10000.0,10000.0,10000.0)));
		// Add the trace into the way point model
		for(vector<Trace>::const_iterator t=(*vehicle).trace.begin();t!=(*vehicle).trace.end();t++)
		{
			// Create waypoints
			end_time = (*t).time;
			if(end_time>maxTime)maxTime=end_time;
			Waypoint wp(Seconds(end_time),Vector((*t).x,(*t).y,0.0));
			waypointmodel->AddWaypoint(wp);
		}
		//Add a final position(0,0,-10000)
		waypointmodel->AddWaypoint(Waypoint(Seconds(end_time+1.0),Vector(-10000.0,-10000.0,-10000.0)));
		mobilityStack.push_back(model);

		//call the initialize method while at time the vehicle depart
		Time start_model_time = Seconds((*(*vehicle).trace.begin()).time);
		Simulator::Schedule(start_model_time, &Object::Initialize, model);
	}

	cout<<"Max time in fcdoutput.xml is "<<maxTime<<endl;
	readTotalTime = maxTime+1;
	// Schedule updates at non-waypoint times to make sure lazy notifications don't happen
	for (double updateTime = 0.5; updateTime <= ((double)maxTime  + 1.5);
			updateTime += 1.0)
	{
		Simulator::Schedule(Seconds(updateTime), &SumoMobility::ForceUpdates, this,mobilityStack);
	}

}

void SumoMobility::ForceUpdates(std::vector<Ptr<MobilityModel> > mobilityStack)
{

	for(uint32_t i=0;i<mobilityStack.size();i++)
	{
		Ptr<WaypointMobilityModel> mob = mobilityStack[i]->GetObject<WaypointMobilityModel>();
		Waypoint waypoint = mob->GetNextWaypoint();
		Ptr<MobilityModel> model = NodeList::GetNode(i)->GetObject<MobilityModel>();
		if (model == 0)
		{
			model = mobilityStack[i];

		}
		model->SetPosition(waypoint.position);
	}

}

const sumomobility::Trace& SumoMobility::GetTrace(uint32_t& Vehicle_ID,Vector& pos) const
{
	vector<Trace>::const_iterator result;
	for (vector<Trace>::const_iterator trace = vl.getVehicles()[Vehicle_ID].trace.begin();
			trace != vl.getVehicles()[Vehicle_ID].trace.end(); trace++)
	{
		if ((*trace) == pos)
			result=trace;
	}
	if(result==vl.getVehicles()[Vehicle_ID].trace.end())
		return *(Trace*)NULL;
	return *result;
}

void SumoMobility::InitializeCoordinateToLane()
{
	const vector<Vehicle>& vehicles=vl.getVehicles();
	vector<Vehicle>::const_iterator vit;
	vector<Trace>::const_iterator trace;
	for(vit=vehicles.begin();vit!=vehicles.end();++vit)
	{
		for (trace =vit->trace.begin();	trace != vit->trace.end(); ++trace)
		{
			Vector2D v(trace->x,trace->y);
			m_CoordinateToLane[v]=std::pair<std::string,double>(trace->lane,trace->pos);
		}
	}
}

} /* namespace sumomobility */
} /* namespace vanetmobility */
} /* namespace ns3 */


