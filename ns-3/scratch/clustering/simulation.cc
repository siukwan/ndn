/*
 * simulation.cc
 *
 *  Created on: Dec 25, 2013
 *      Author: cys
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/application.h"
#include "ns3/vanetmobility-helper.h"

#include "NeighborDetector.h"
#include "NodeSensorHelper.h"
#include "NodeSensor.h"


#include <iostream>
#include <vector>
#include <string>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

NS_LOG_COMPONENT_DEFINE("ALM_Clustering");

using namespace ns3;
using namespace std;
using namespace nrc;


class nrcExample
{
public:
  nrcExample ();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);
  /// Run simulation
  void Run ();
  /// Report results
  void Report (std::ostream & os);

private:
  ///\name parameters
  //\{
  /// Number of nodes
  uint32_t size;
  /// Distance between nodes, meters
  double step;
  /// Simulation time, seconds
  double totalTime;
  /// total time from sumo
  double readTotalTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;
  /// look at clock interval
  double clockInterval;
  /// Wifi Phy mode
  string phyMode;
  /// Tell echo applications to log if true
  bool verbose ;
  /// indicate whether use flooding to forward messages
  bool flood;
  /// How many emergency messages
  uint32_t emsgNum;
  /// The Simulation data path
  string inputDir;
  /// Output data path
  string outputDir;
  /// Programme name
  string name;
  //\}

  ///\name network
  //\{
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
  //\}

  Ptr<ns3::vanetmobility::VANETmobility> mobility;

  ofstream statistics_result;
  string statistics_result_path;
private:
  // Initialize
  /// Load traffic data
  void LoadTraffic();
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallSensor();
  void InstallMobility();
  void InstallApplications ();
  void InstallTestApplications();

  // Utility funcitons
  void Look_at_clock();
  void ForceUpdates (std::vector<Ptr<MobilityModel> > mobilityStack);
  void SetPos(Ptr<MobilityModel> mob);
  void PrintStatistic();
};

int main (int argc, char **argv)
{
	//srand(time(NULL));
	//SeedManager::SetSeed(rand());
	//RngSeedManager::SetSeed (1);
	cout << "ALM Begin!" << endl;
	nrcExample test;
	if (!test.Configure(argc, argv))
		NS_FATAL_ERROR("Configuration failed. Aborted.");

	test.Run();
	test.Report(std::cout);

	return 0;
}



//-----------------------------------------------------------------------------
nrcExample::nrcExample () :
  size (10),
  step (100),
  totalTime (36000),
  readTotalTime(0),
  pcap (false),
  printRoutes (true),
  clockInterval(10),
  phyMode("DsssRate1Mbps"),
  verbose (false),
  flood(false),
  emsgNum(2)
{
	string home         = getenv("HOME");
	inputDir  = home +"/tcl/use";
	outputDir = home +"/tcl/use";
}

bool
nrcExample::Configure (int argc, char **argv)
{
  // Enable AODV logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);
  name = strrchr(argv[0], '/');
  cout<<"Programme name "<<name<<endl;

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("clockInterval","look at clock interval.",clockInterval);
  cmd.AddValue ("phyMode","Wifi Phy mode",phyMode);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("flood","indicate whether to use flooding to forward messages",flood);
  cmd.AddValue ("emsgNum","How many emergency messages",emsgNum);
  cmd.AddValue ("inputDir","The Simulation data path",inputDir);
  cmd.AddValue ("outputDir","The Simulation output path",outputDir);

  cmd.Parse (argc, argv);
  return true;
}

void
nrcExample::Run ()
{
	LoadTraffic();
	CreateNodes();
	CreateDevices();
	InstallMobility();
	InstallSensor();
	InstallInternetStack();
	InstallApplications();
	// InstallTestApplications();

	Simulator::Schedule(Seconds(0.0), &nrcExample::Look_at_clock, this);
	totalTime = readTotalTime < totalTime ? readTotalTime : totalTime;
	std::cout << "Starting simulation for " << totalTime << " s ...\n";
	Simulator::Stop(Seconds(totalTime));
	Simulator::Run();

	PrintStatistic();

	Simulator::Destroy();
	NeighborDetector::closefile();
}

void nrcExample::InstallSensor()
{
	NS_LOG_INFO ("Installing Sensors");
	NodeSensorHelper sensorHelper;
	sensorHelper.SetSensorModel("ns3::ndn::nrndn::SumoNodeSensor",
			"sumodata",PointerValue(mobility));
	sensorHelper.InstallAll();
}

void
nrcExample::PrintStatistic()
{
	NeighborDetector::initializeCounter();
	 statistics_result.open(statistics_result_path.data(),ios::out);
	  for(uint32_t i=0;i<size;i++)
	  {
		  Application*  app=GetPointer(nodes.Get(i)->GetApplication(0));

	      ((NeighborDetector*)app)->statistics(statistics_result);
	      ((NeighborDetector*)app)->msgCounter();
	  }
	  statistics_result.close();
	  ofstream out;
	  std::ostringstream os;
	  os<<outputDir<<'/'<<flood<<".txt";
	  out.open(os.str().data());

	  NeighborDetector::printMsgRecvCounter(out);
	  out.close();
}

void
nrcExample::Report (std::ostream &os)
{
	os<<"Stop"<<endl;
}


void
nrcExample::LoadTraffic()
{
	DIR* dir=NULL;
	DIR* subdir=NULL;
	if((dir = opendir(inputDir.data()))==NULL)
		NS_FATAL_ERROR("Cannot open input path "<<inputDir.data()<<", Aborted.");
	outputDir += name;
	if((subdir = opendir(outputDir.data()))==NULL)
	{
		cout<<outputDir.data()<<" not exist, create a new one"<<endl;
		if(mkdir(outputDir.data(),S_IRWXU)==-1)
			NS_FATAL_ERROR("Cannot create dir "<<outputDir.data()<<", Aborted.");
	}
	string netxmlpath   = inputDir + "/input_net.net.xml";
	string routexmlpath = inputDir + "/routes.rou.xml";
	string fcdxmlpath   = inputDir + "/fcdoutput.xml";

	statistics_result_path = outputDir + "/result.txt";
	string outfile      = outputDir + "/trace.txt";

	ns3::vanetmobility::VANETmobilityHelper mobilityHelper;
	mobility=mobilityHelper.GetSumoMObility(netxmlpath,routexmlpath,fcdxmlpath);
	size = mobility->GetNodeSize(); //Vehicle number
	NeighborDetector::open(outfile.data());
	NeighborDetector::m_flood = flood;
}

void
nrcExample::CreateNodes ()
{
	std::cout << "Creating " << (unsigned) size << " vehicle nodes.\n";
	nodes.Create(size);
	// Name nodes
	for (uint32_t i = 0; i < size; ++i)
	{
		std::ostringstream os;
		os << "vehicle-" << i;
		Names::Add(os.str(), nodes.Get(i));
	}
}

void
nrcExample::CreateDevices ()
{
	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold",
			StringValue("2200"));
	// turn off RTS/CTS for frames below 2200 bytes
	Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
			StringValue("2200"));
	// Fix non-unicast data rate to be the same as that of unicast
	Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
			StringValue(phyMode));

	WifiHelper wifi = WifiHelper::Default ();
	if (verbose)
	{
		wifi.EnableLogComponents();  // Turn on all Wifi logging
	}
	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
	wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode",
				StringValue(phyMode), "ControlMode", StringValue(phyMode));

	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
	// ns-3 supports RadioTap and Prism tracing extensions for 802.11b
	wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
	YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
	wifiPhy.SetChannel(wifiChannel.Create());

	// Add a non-QoS upper mac, and disable rate control
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();

	// Set it to adhoc mode
	wifiMac.SetType("ns3::AdhocWifiMac");
	devices = wifi.Install(wifiPhy, wifiMac, nodes);

    if (pcap)
	{
		wifiPhy.EnablePcapAll(std::string("aodv"));
	}
}

void
nrcExample::InstallInternetStack ()
{
//  AodvHelper aodv;
  InternetStackHelper stack;
 // stack.SetRoutingHelper(aodv);
  stack.Install (nodes);
  Ipv4AddressHelper address;
  NS_LOG_INFO ("Assign IP Addresses.");
  address.SetBase ("10.0.0.0", "255.255.0.0");
  interfaces = address.Assign (devices);
}

void
nrcExample::InstallMobility()
{
	mobility->Install();
	readTotalTime = mobility->GetReadTotalTime();
	totalTime = readTotalTime < totalTime ? readTotalTime : totalTime;
}

void
nrcExample::InstallApplications ()
{
	//Setup Emergency Messages:
	map<uint32_t,Time> emsg;
	Ptr<UniformRandomVariable> rnd = CreateObject<UniformRandomVariable>();
	for(uint32_t i=0;i<emsgNum;i++)
	{
		uint32_t id = rnd->GetInteger(0,size-1);
		uint32_t start = mobility->GetStartTime(id);
		uint32_t end   = mobility->GetStopTime(id);
		emsg[id] = Seconds(rnd->GetInteger(start+2,end-2)+0.2);
		cout<<"ID = "<< id <<" emsgtime = "<<emsg[id].GetSeconds()<<endl;
	}

	for(uint32_t i=0;i<size;i++)
	{
		Ptr<NeighborDetector>  app = Create<NeighborDetector>();
		app->SetStartTime(Seconds(mobility->GetStartTime(i)));
		app->SetStopTime (Seconds(mobility->GetStopTime(i)));
		if (emsg.count(i))
		{
			app->ScheduleEmergencyMessage(emsg[i]);
		}
		nodes.Get(i)->AddApplication(app);
	}

}

void nrcExample::InstallTestApplications()
{
	// Create static grid
	MobilityHelper mobility;
	mobility.SetPositionAllocator ("ns3::GridPositionAllocator","MinX", DoubleValue (0.0),"MinY", DoubleValue (0.0),"DeltaX", DoubleValue (step), "DeltaY", DoubleValue (0),
	                                "GridWidth", UintegerValue (size),"LayoutType", StringValue ("RowFirst"));
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(nodes);

	ApplicationContainer p;
	for(uint32_t i=0;i<size;i++)
	{
		Ptr<NeighborDetector>  app = Create<NeighborDetector>();
		nodes.Get(i)->AddApplication(app);
		p.Add(app);
	}

	p.Start (Seconds (0));
    p.Stop (Seconds (totalTime) - Seconds (0.001));

    // move node away
    Ptr<Node> node = nodes.Get (0);
    Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
	Simulator::Schedule (Seconds (1.0), &nrcExample::SetPos, this, mob);
}
void nrcExample::Look_at_clock()
{
	cout<<"Time now: "<<Simulator::Now().GetSeconds()<<endl;
	Simulator::Schedule(Seconds(clockInterval),&nrcExample::Look_at_clock,this);
}



void nrcExample::ForceUpdates (std::vector<Ptr<MobilityModel> > mobilityStack)
{
	for(uint32_t i=0;i<mobilityStack.size();i++)
	{
		Ptr<WaypointMobilityModel> mob = mobilityStack[i]->GetObject<WaypointMobilityModel>();
		Waypoint waypoint = mob->GetNextWaypoint();
		Ptr<MobilityModel> model = nodes.Get(i)->GetObject<MobilityModel>();
		if (model == 0)
		{
			model = mobilityStack[i];

		}
		model->SetPosition(waypoint.position);
	}

}

void nrcExample::SetPos(Ptr<MobilityModel> mob)
{
	ns3::Vector pos = mob->GetPosition();
	pos.x+=50;
	mob->SetPosition(pos);
	Simulator::Schedule (Seconds (1.0), &nrcExample::SetPos, this, mob);
}
