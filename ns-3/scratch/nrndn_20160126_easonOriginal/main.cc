/*
 * simulation.cc
 *
 *  Created on: Jan 8, 2016
 *      Author: siukwan
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/config-store-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/application.h"

#include "ns3/vanetmobility-helper.h"
#include "ns3/ndnSIM-module.h"
#include "nrProducer.h"
#include "NodeSensorHelper.h"
#include "nrUtils.h"
//#include "ndn-nr-pit-impl.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
NS_LOG_COMPONENT_DEFINE("nrndn.Example");

using namespace ns3;
using namespace std;
using namespace ns3::ndn::nrndn;
//using namespace ns3::ndn;
using namespace ns3::vanetmobility;

struct timeval StartTime;
struct timeval EndTime;
double TimeUse=0;
//获取时间
time_t startTime;
time_t endTime;

class nrndnExample
{
public:
  nrndnExample ();
  ~nrndnExample();
  /// Configure script parameters, \return true on successful configuration
  bool Configure (int argc, char **argv);

  void Run();
  /// Run simulation of nrndn
  void RunNrndnSim ();
  /// Run simulation of distance based forwarding
  void RunDistSim ();
  /// Run simulation of CDS based forwarding
  void RunCDSSim ();
  /// Report results
  void Report ();

  uint32_t getMethod()
  {
	  return method;
  }

private:
  ///\name parameters
  //\{
  map<uint32_t, bool> randomNode;
  int random_seed;
  uint32_t certain_count; //定点数量
  double certain_interval;//定点事件间隔
  int random_accident;
  /// Number of nodes
  uint32_t size;
  /// Simulation time, seconds
  double totalTime;
  /// total time from sumo
  double readTotalTime;
  /// nodes stop moving time, default is totalTime
  double alltoallTime;
  /// Write per-device PCAP traces if true
  bool pcap;
  /// Print routes if true
  bool printRoutes;
  /// look at clock interval
  double clockInterval;
  /// dominator percentage interval
  double dpInterval;
  /// Wifi Phy mode
  string phyMode;
  /// Tell echo applications to log if true
  bool verbose ;
  /// indicate whether use flooding to forward messages
  bool flood;
  /// transmission range
  double transRange;
  ///the switch which can turn on the log on Functions about hello messages
  bool HelloLogEnable;
  ///the number of accident. It will randomly put into the whole simulation
  uint32_t accidentNum;

  uint32_t method;

  string inputDir;
  /// Output data path
  string outputDir;
  /// Programme name
  string name;

  std::ofstream os;

  // Interest Packet Sending Frequency
  double interestFrequency;

  //\}

  ///\name network
  //\{
  NodeContainer nodes;
  NetDeviceContainer devices;
  Ipv4InterfaceContainer interfaces;
  //\}

  ///\name traffic information
  //\{
//  RoadMap roadmap;
//  VehicleLoader vl;
  Ptr<VANETmobility> mobility;
  //\}

  //hitRate: among all the interested nodes, how many are received
  double hitRate;

  //accuracyRate: among all the nodes received, how many are interested
  double accuracyRate;
  double disinterestRate;
  double arrivalRate;
  double averageForwardTimes;
  double averageInterestForwardTimes;
  double averageDelay;
  uint32_t SumForwardTimes;



  bool noFwStop;

  uint32_t TTLMax;
  uint32_t virtualPayloadSize;
private:
  // Initialize
  /// Load traffic data
  void LoadTraffic();
  void CreateNodes ();
  void CreateDevices ();
  void InstallInternetStack ();
  void InstallSensor();
  void InstallNrNdnStack();
  void InstallDistNdnStack();
  void InstallCDSNdnStack();
  void InstallMobility();
  void InstallTestMobility();
  void InstallNrndnApplications ();
  void InstallDistApplications();
  void InstallCDSApplications();
  void InstallTestApplications();

  void InstallTraffics();

  // Utility funcitons
  void Look_at_clock();
  void ForceUpdates (std::vector<Ptr<MobilityModel> > mobilityStack);
  void SetPos(Ptr<MobilityModel> mob);
  void getStatistic();
};

int main (int argc, char **argv)
{

	gettimeofday(&StartTime, NULL);
	time(&startTime);
	nrndnExample test;
	if (!test.Configure(argc, argv))
		NS_FATAL_ERROR("Configuration failed. Aborted.");
	if(test.getMethod() == 3)
	{
		cout<<"编译完成，退出程序"<<endl;
		return 0;
	}
	test.Run();
	test.Report();
	return 0;
}



//-----------------------------------------------------------------------------
//构造函数
nrndnExample::nrndnExample () :
  random_seed(54321),
  certain_count(20), //定点数量
  certain_interval(10),//定点事件间隔
  random_accident(0),//默认不随机
  size (3),
  totalTime (36000),
  readTotalTime(0),
  alltoallTime(-1),
  pcap (false),
  printRoutes (true),
  clockInterval(1),
  dpInterval(1),
  phyMode("DsssRate1Mbps"),
  //phyMode("OfdmRate24Mbps"),
  verbose (false),
  flood(false),
  transRange(300),
  HelloLogEnable(true),
  accidentNum(0),//默认3
  method(0),
  interestFrequency(2),//本来是1秒1次,0.0001是10000秒一次
  hitRate(0),
  accuracyRate(0),
  disinterestRate(0),
  arrivalRate(0),
  averageForwardTimes(0),
  averageInterestForwardTimes(0),
  averageDelay(0),
  SumForwardTimes(0),
  noFwStop(true),
  TTLMax(1),
  virtualPayloadSize(1024)
{
	//os =  std::cout;
	string home         = getenv("HOME");
	inputDir  = home +"/input";
	outputDir = home +"/input";
}

nrndnExample::~nrndnExample()
{
	os.close();
}
bool
nrndnExample::Configure (int argc, char **argv)
{
  // Enable logs by default. Comment this if too noisy
  // LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL);
  //name = strrchr(argv[0], '/');
  time_t now = time(NULL);
  cout<<"NR-NDN simulation begin at "<<ctime(&now);

  SeedManager::SetSeed (12345);
  CommandLine cmd;

  cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
  cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
  cmd.AddValue ("time", "Simulation time, s.", totalTime);
  cmd.AddValue ("clockInterval","look at clock interval.",clockInterval);
  cmd.AddValue ("dpInterval","dominator percentage interval ",dpInterval);
  cmd.AddValue ("phyMode","Wifi Phy mode",phyMode);
  cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);
  cmd.AddValue ("inputDir","The Simulation data path",inputDir);
  cmd.AddValue ("outputDir","The Simulation output path",outputDir);
  cmd.AddValue ("flood","indicate whether to use flooding to forward messages",flood);
  cmd.AddValue ("alltoallTime","Nodes begin to broadcast emergency messages, it is an all-to-all broadcast",alltoallTime);
  cmd.AddValue ("transRange","transmission range",transRange);
  cmd.AddValue("HelloLogEnable","the switch which can turn on the log on Functions about hello messages",HelloLogEnable);
  cmd.AddValue("accidentNum","the number of accident. It will randomly put into the whole simulation(default 3)",accidentNum);
  cmd.AddValue("method","Forward method,0=Nrndn, 1=Dist, 2=CDS",method);
  cmd.AddValue("noFwStop","When the PIT covers the nodes behind, no broadcast stop message",noFwStop);
  cmd.AddValue("TTLMax","This value indicate that when a data is received by disinterested node, the max hop count it should be forwarded",TTLMax);
  cmd.AddValue("interestFreq","Interest Packet Sending Frequency(Hz)",interestFrequency);
  cmd.AddValue("virtualPayloadSize","Virtual payload size for traffic Content packets",virtualPayloadSize);
  cmd.Parse (argc, argv);
  std::cout<<"(main.cc)兴趣包发送频率:"<<interestFrequency<<std::endl;
 // getchar();
  return true;
}

void nrndnExample::Run()
{
	std::cout<<"运行方法是："<<method<<std::endl;
	switch(method)
	{
	case 0:
		RunNrndnSim();
		break;
	case 1:
		RunDistSim();
		break;
	case 2:
		RunCDSSim();
		break;
	default:
		cout<<"Undefine method"<<endl;
		break;
	}
}

void
nrndnExample::RunNrndnSim ()
{
	name = "NR-NDN-Simulation";

	std::cout<<"读取交通数据"<<std::endl;
	LoadTraffic();
	std::cout<<"创造节点"<<std::endl;
	CreateNodes();
	std::cout<<"创造设备"<<std::endl;
	CreateDevices();
	//InstallInternetStack();
	std::cout<<"初始化Mobility"<<std::endl;
	InstallMobility();
	std::cout<<"安装传感器"<<std::endl;
	InstallSensor();
	std::cout<<"初始化NrNdnStack"<<std::endl;
	InstallNrNdnStack();

	//InstallTestMobility();
	std::cout<<"安装Nrndn应用程序"<<std::endl;
	InstallNrndnApplications();
	//InstallTestApplications();
	std::cout<<"安装交通状况"<<std::endl;
	InstallTraffics();


	Simulator::Schedule(Seconds(0.0), &nrndnExample::Look_at_clock, this);

	std::cout << "Starting simulation for " << totalTime << " s ...\n";

	Simulator::Stop(Seconds(totalTime));
	std::cout << "开始运行：\n";
	Simulator::Run();

	Simulator::Destroy();
}


void nrndnExample::RunDistSim()
{
	name = "Dist-Simulation";
	std::cout<<"读取交通数据"<<std::endl;
	LoadTraffic();
	std::cout<<"创造节点"<<std::endl;
	CreateNodes();
	std::cout<<"创造设备"<<std::endl;
	CreateDevices();
	std::cout<<"初始化Mobility"<<std::endl;
	InstallMobility();
	std::cout<<"安装传感器"<<std::endl;
	InstallSensor();
	std::cout<<"初始化DistNdnStack"<<std::endl;
	InstallDistNdnStack();
	std::cout<<"安装Dist应用程序"<<std::endl;
	InstallDistApplications();
	std::cout<<"安装交通状况"<<std::endl;
	InstallTraffics();

	Simulator::Schedule(Seconds(0.0), &nrndnExample::Look_at_clock, this);

	std::cout << "Starting simulation for " << totalTime << " s ...\n";

	Simulator::Stop(Seconds(totalTime));
	Simulator::Run();

	Simulator::Destroy();
}

void nrndnExample::RunCDSSim()
{
	name = "CDS-Simulation";
	LoadTraffic();
	CreateNodes();
	CreateDevices();
	InstallMobility();
	InstallSensor();
	InstallCDSNdnStack();
	InstallCDSApplications();
	InstallTraffics();

	Simulator::Schedule(Seconds(0.0), &nrndnExample::Look_at_clock, this);

	std::cout << "Starting simulation for " << totalTime << " s ...\n";

	Simulator::Stop(Seconds(totalTime));
	Simulator::Run();

	Simulator::Destroy();
}

void
nrndnExample::Report ()
{
	string method_cout="";
	switch(method)
	{
	case 0:
		method_cout="RunNrndnSim()";
		break;
	case 1:
		method_cout="RunDistSim() ";
		break;
	case 2:
		method_cout="RunCDSSim()  ";
		break;
	default:
		method_cout="Undefine method";
		break;
	}
	gettimeofday(&EndTime, NULL);
	TimeUse = 1000000*(EndTime.tv_sec-StartTime.tv_sec)+EndTime.tv_usec-StartTime.tv_usec;
	TimeUse/=1000;
	NS_LOG_UNCOND (method_cout+":report data outputs here");
	time(&endTime);
	tm *timeOut;
	timeOut=localtime(&startTime);
	cout<<"(main.cc)开始时间：";
	cout<<timeOut->tm_year+1900<<"-"<<timeOut->tm_mon+1<<"-"<<timeOut->tm_mday<<" "<<timeOut->tm_hour<<":"<<timeOut->tm_min<<":"<<timeOut->tm_sec<<endl;
	
	if(method == 0)
		os << endl;

	os<<"(main.cc)开始时间：";
	os<<timeOut->tm_year+1900<<"-"<<timeOut->tm_mon+1<<"-"<<timeOut->tm_mday<<" "<<timeOut->tm_hour<<":"<<timeOut->tm_min<<":"<<timeOut->tm_sec<<endl;
	timeOut=localtime(&endTime);
	os<<"(main.cc)结束时间：";
	os<<timeOut->tm_year+1900<<"-"<<timeOut->tm_mon+1<<"-"<<timeOut->tm_mday<<" "<<timeOut->tm_hour<<":"<<timeOut->tm_min<<":"<<timeOut->tm_sec<<endl;
	cout<<"(main.cc)结束时间：";
	cout<<timeOut->tm_year+1900<<"-"<<timeOut->tm_mon+1<<"-"<<timeOut->tm_mday<<" "<<timeOut->tm_hour<<":"<<timeOut->tm_min<<":"<<timeOut->tm_sec<<endl;

	std::cout<<"(main.cc)Report data outputs here\n";
	std::cout<<"(main.cc)运行方法是："<<method_cout<<endl;
	//1. get statistic first
	getStatistic();
	os<<"(main.cc)method:"<<method_cout<<endl;

	string random_accident_str = "定点";
	if(random_accident == 1)
		random_accident_str = "随机";

	os<<"(main.cc)seed:"<<random_seed<<" 节点:";

	map<uint32_t, bool>::iterator map_ite;
	for(map_ite = randomNode.begin(); map_ite != randomNode.end(); map_ite ++)
	{
		os<<map_ite->first << " ";
	}
	os << endl;
	os<<"(main.cc)certain_count定点数量:"<<certain_count<<endl;
	os<<"(main.cc)certain_interval定点间隔:"<<certain_interval<<endl;
	os<<"(main.cc)random_accident:"<<random_accident<<random_accident_str<<endl;
	os<<"(main.cc)accidentNum:"<<accidentNum<<endl;
	os<<"(main.cc)transRange:"<<transRange<<endl;
	os<<"(main.cc)noFwStop:"<<noFwStop<<endl;
	os<<"(main.cc)TTLMax:"<<TTLMax<<endl;
	os<<"(main.cc)interestFreq:"<<interestFrequency<<endl;
	os<<"(main.cc)nodeSum:"<<size<<endl;
	os<<"(main.cc)simulationTime:"<<totalTime<<endl;
	os<<"(main.cc)runningTime:"<<(int)(TimeUse/1000)/60<<"m"<<((int)(TimeUse/1000))%60<<"s"<<endl;



    cout<<"(main.cc)seed:"<<random_seed<<" 节点:";
	for(map_ite = randomNode.begin(); map_ite != randomNode.end(); map_ite ++)
	{
		cout<<map_ite->first << " ";
	}
	cout << endl;
	cout<<"(main.cc)certain_count定点数量:"<<certain_count<<endl;
	cout<<"(main.cc)certain_interval定点间隔:"<<certain_interval<<endl;
	cout<<"(main.cc)random_accident:"<<random_accident<<random_accident_str<<endl;
	cout<<"(main.cc)accidentNum:"<<accidentNum<<endl;
	cout<<"(main.cc)transRange:"<<transRange<<endl;
	cout<<"(main.cc)noFwStop:"<<noFwStop<<endl;
	cout<<"(main.cc)TTLMax:"<<TTLMax<<endl;
	cout<<"(main.cc)interestFreq:"<<interestFrequency<<endl;
	cout<<"(main.cc)nodeSum:"<<size<<endl;
	cout<<"(main.cc)simulationTime:"<<totalTime<<endl;
	cout<<"(main.cc)runningTime:"<<(int)(TimeUse/1000)/60<<"m"<<((int)(TimeUse/1000))%60<<"s"<<endl;


	cout<<"总花费时间为："<<TimeUse<<"ms"<<endl;
	cout<<"总花费时间为："<<TimeUse/1000<<"s"<<endl;
	cout<<"总花费时间为："<<(int)(TimeUse/1000)/60<<"分"<<((int)(TimeUse/1000))%60<<"秒"<<endl;

	//2. output the result
	os<<std::left<<std::setw(11)<<"arrivalR"
			<<std::left<<std::setw(10)<<"accuracyR"
			<<std::left<<std::setw(10)<<"hitR"
			<<std::left<<std::setw(10)<<"avgDelay"
			<<std::left<<std::setw(10)<<"avgFwd"
			<<std::left<<std::setw(11)<<"avgIntFwd"
			<<std::left<<std::setw(10)<<"SumFwd"
			<<std::left<<std::setw(13)<<"IntByteSent"
			<<std::left<<std::setw(11)<<"HelByteSnt"
			<<std::left<<std::setw(11)<<"DatByteSnt"
			<<std::left<<std::setw(11)<<"ByteSnt"
			<<std::left<<std::setw(11)<<"disinterestR"<<endl;
	os<<std::left<<std::setw(11)<<arrivalRate
			<<std::left<<std::setw(10)<<accuracyRate
			<<std::left<<std::setw(10)<<hitRate
			<<std::left<<std::setw(10)<<averageDelay
			<<std::left<<std::setw(10)<<averageForwardTimes
			<<std::left<<std::setw(11)<<averageInterestForwardTimes
			<<std::left<<std::setw(10)<<SumForwardTimes
			<<std::left<<std::setw(13)<<nrUtils::InterestByteSent
			<<std::left<<std::setw(11)<<nrUtils::HelloByteSent
			<<std::left<<std::setw(11)<<nrUtils::DataByteSent
			<<std::left<<std::setw(11)<<nrUtils::ByteSent
			<<std::left<<std::setw(11)<<disinterestRate<<endl;





	std::cout<<std::left<<std::setw(11)<<"arrivalR"
			<<std::left<<std::setw(10)<<"accuracyR"
			<<std::left<<std::setw(10)<<"hitR"
			<<std::left<<std::setw(10)<<"avgDelay"
			<<std::left<<std::setw(10)<<"avgFwd"<<endl;
	std::cout<<std::left<<std::setw(11)<<arrivalRate
			<<std::left<<std::setw(10)<<accuracyRate
			<<std::left<<std::setw(10)<<hitRate
			<<std::left<<std::setw(10)<<averageDelay
			<<std::left<<std::setw(10)<<averageForwardTimes<<endl<<endl;


	std::cout<<std::left<<std::setw(11)<<"avgIntFwd"
			<<std::left<<std::setw(10)<<"SumFwd"
			<<std::left<<std::setw(13)<<"IntByteSent"
			<<std::left<<std::setw(11)<<"HelByteSnt"
			<<std::left<<std::setw(11)<<"DatByteSnt"
			<<std::left<<std::setw(11)<<"ByteSnt"
			<<std::left<<std::setw(11)<<"disinterestR"<<endl;

	std::cout<<std::left<<std::setw(11)<<averageInterestForwardTimes
			<<std::left<<std::setw(10)<<SumForwardTimes
			<<std::left<<std::setw(13)<<nrUtils::InterestByteSent
			<<std::left<<std::setw(11)<<nrUtils::HelloByteSent
			<<std::left<<std::setw(11)<<nrUtils::DataByteSent
			<<std::left<<std::setw(11)<<nrUtils::ByteSent
			<<std::left<<std::setw(11)<<disinterestRate<<endl;


	/*
	os<<arrivalRate <<'\t'
			<<accuracyRate<<'\t'
			<<hitRate<<'\t'
			<<averageDelay<<'\t'
			<<averageForwardTimes<<'\t'
			<<averageInterestForwardTimes<<'\t'
			<<SumForwardTimes<<'\t'
			<<nrUtils::InterestByteSent<<'\t'
			<<nrUtils::HelloByteSent<<'\t'
			<<nrUtils::DataByteSent<<'\t'
			<<nrUtils::ByteSent<<'\t'
			<<disinterestRate<<endl;
			*/
}

void
nrndnExample::LoadTraffic()
{
	cout<<"Method: "<<name<<endl;
	DIR* dir=NULL;
	DIR* subdir=NULL;
	//打开数据源
	if((dir = opendir(inputDir.data()))==NULL)
		NS_FATAL_ERROR("Cannot open input path "<<inputDir.data()<<", Aborted.");
	outputDir += '/' + name;
	if((subdir = opendir(outputDir.data()))==NULL)
	{
		cout<<outputDir.data()<<" not exist, create a new one"<<endl;
		if(mkdir(outputDir.data(),S_IRWXU)==-1)
			NS_FATAL_ERROR("Cannot create dir "<<outputDir.data()<<", Aborted.");
	}
	string netxmlpath   = inputDir + "/input_net.net.xml";
	string routexmlpath = inputDir + "/routes.rou.xml";
	string fcdxmlpath   = inputDir + "/fcdoutput.xml";

	string outfile      = outputDir + "/result.txt";

	std::cout<<"输出文件路径："<<outfile<<std::endl;
	os.open(outfile.data(),ios::out);
	std::cout<<"文件打开成功。"<<std::endl;

	VANETmobilityHelper mobilityHelper;
	mobility=mobilityHelper.GetSumoMObility(netxmlpath,routexmlpath,fcdxmlpath);
	std::cout<<"读取完毕！"<<std::endl;
//获取结点size
	size = mobility->GetNodeSize();
	std::cout<<"节点size："<<size<<std::endl;

	if(accidentNum == 0 || accidentNum == 999)
	{
		if(accidentNum != 999) //999表示非随机
		{
			random_accident = 1;//如果没有输入事件数量，则使用定点发
			cout << "定点发数据" << endl;
		}
		else
			cout << "随机" << endl;
		accidentNum = size * 1;
	}
	std::cout<<"(main.cc)修改accidentNum为size的4倍"<<accidentNum<<std::endl;

}

void
nrndnExample::CreateNodes ()
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

	std::cout << "创建完毕\n";
}

void
nrndnExample::CreateDevices ()
{//设置网卡的，不用修改
	// disable fragmentation for frames below 2200 bytes
	Config::SetDefault("ns3::WifiRemoteStationManager::FragmentationThreshold",
			StringValue("2200"));
	// turn off RTS/CTS for frames below 2200 bytes
	//request to send / clear to send 请求发送/清除发送协议
	Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
			StringValue("2200"));
	// Fix non-unicast data rate to be the same as that of unicast//单播
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

	//wifiPhy.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange",
	//				DoubleValue(transRange));


	wifiPhy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

	//YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
	YansWifiChannelHelper wifiChannel;
	wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");

	wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange",
						DoubleValue(transRange));
	wifiPhy.SetChannel(wifiChannel.Create());

	// Add a non-QoS upper mac, and disable rate control
	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();

	// Set it to adhoc mode
	//wifiMac.SetType("ns3::AdhocWifiMac");

	devices = wifi.Install(wifiPhy, wifiMac, nodes);

    if (pcap)
	{
		wifiPhy.EnablePcapAll(std::string("aodv"));
	}
}

void
nrndnExample::InstallNrNdnStack()
{
	NS_LOG_INFO ("Installing NDN stack");
	ndn::StackHelper ndnHelper;
	// ndnHelper.AddNetDeviceFaceCreateCallback (WifiNetDevice::GetTypeId (), MakeCallback (MyNetDeviceFaceCallback));
	//ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::BestRoute");
	string str("false");
	string noFwStopStr("false");
	if(HelloLogEnable)
		str="true";
	if(noFwStop)
		noFwStopStr="true";
	std::ostringstream TTLMaxStr;
	TTLMaxStr<<TTLMax;
	std::ostringstream pitCleanIntervalStr;
	uint32_t pitCleanInterval = 1.0 / interestFrequency;
	//uint32_t pitCleanInterval = 1.0 / 1 * 3.0;//有什么作用？
		pitCleanIntervalStr<<pitCleanInterval;
	cout<<"pitInterval="<<pitCleanIntervalStr.str()<<endl;
	ndnHelper.SetForwardingStrategy ("ns3::ndn::fw::nrndn::NavigationRouteHeuristic","HelloLogEnable",str,"NoFwStop",noFwStopStr,"TTLMax",TTLMaxStr.str());
	ndnHelper.SetContentStore ("ns3::ndn::cs::Lru", "MaxSize", "1000");
	ndnHelper.SetPit("ns3::ndn::pit::nrndn::NrPitImpl","CleanInterval",pitCleanIntervalStr.str());
	ndnHelper.SetDefaultRoutes (true);
	ndnHelper.Install (nodes);
}

void nrndnExample::InstallDistNdnStack()
{
  NS_LOG_INFO("Installing Dist NDN stack");
  ndn::StackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy (
      "ns3::ndn::fw::nrndn::DistanceBasedForwarding");
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.Install (nodes);
}

void nrndnExample::InstallCDSNdnStack()
{
  NS_LOG_INFO("Installing CDS NDN stack");
  ndn::StackHelper ndnHelper;
  ndnHelper.SetForwardingStrategy (
      "ns3::ndn::fw::nrndn::CDSBasedForwarding");
  ndnHelper.SetDefaultRoutes (true);
  ndnHelper.Install (nodes);
}

void
nrndnExample::InstallInternetStack ()
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
nrndnExample::InstallMobility()
{
	//double maxTime = 0;
	std::cout<<"正在安装mobility.."<<std::endl;
	mobility->Install();
	std::cout<<"正在读取总时间："<<std::endl;
	readTotalTime = mobility->GetReadTotalTime();
	totalTime = readTotalTime < totalTime ? readTotalTime : totalTime;
	std::cout<<"总时间："<<totalTime<<std::endl;
}



void
nrndnExample::InstallNrndnApplications ()
{
	NS_LOG_INFO ("Installing nrndn Applications");
	ndn::AppHelper consumerHelper ("ns3::ndn::nrndn::nrConsumer");
	//Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
	consumerHelper.SetAttribute ("Frequency", DoubleValue (interestFrequency));
	consumerHelper.SetAttribute ("PayloadSize", UintegerValue (virtualPayloadSize));//从virtualPayloadSize改为0,因为兴趣树已经包含在header中
	//If you send the same interest packet for several times, the data producer will
	//response your interest packet respectively. It may be a waste. So we can set Consumer::MaxSeq
	//To limit the times interest packet send. For example,just 1.0
	//consumerHelper.SetAttribute ("MaxSeq"/*"Maximum sequence number to request"*/,IntegerValue(2));
//	consumerHelper.Install (nodes.Get(0));
	consumerHelper.Install(nodes);
	nrUtils::appIndex["ns3::ndn::nrndn::nrConsumer"]=0;
/*
	double start=mobility->GetStartTime(0);
	double stop =mobility->GetStopTime (0);
	nodes.Get(0)->GetApplication(0)->SetAttribute("StartTime",TimeValue (Seconds (start)));
	nodes.Get(0)->GetApplication(0)->SetAttribute("StopTime", TimeValue (Seconds (stop )));
*/
	ndn::AppHelper producerHelper ("ns3::ndn::nrndn::nrProducer");
	//producerHelper.SetPrefix ("/");
	producerHelper.SetAttribute ("PayloadSize", UintegerValue (virtualPayloadSize));
	//producerHelper.Install (nodes.Get (0));
	producerHelper.Install(nodes);
	nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"]=1;

	//Setup start and end time;
///*
	for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
	{
		double start=mobility->GetStartTime((*i)->GetId());
		double stop =mobility->GetStopTime ((*i)->GetId());
		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrConsumer"])->SetAttribute("StartTime",TimeValue (Seconds (start)));
		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrConsumer"])->SetAttribute("StopTime", TimeValue (Seconds (stop )));

		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"])->SetAttribute("StartTime",TimeValue (Seconds (start)));
		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"])->SetAttribute("StopTime", TimeValue (Seconds (stop )));
	}
//*/

	//for test
	//nodes.Get(0)->GetApplication(0)->SetAttribute("StartTime",TimeValue (Seconds (0)));
	//nodes.Get(0)->GetApplication(0)->SetAttribute("StopTime", TimeValue (Seconds (10 )));

}

//这个函数只是1秒执行一次
void nrndnExample::Look_at_clock()
{

	cout<<"\n(main.cc) Time now: "<<Simulator::Now().GetSeconds()<<endl;

	Simulator::Schedule(Seconds(clockInterval),&nrndnExample::Look_at_clock,this);
}


void nrndnExample::ForceUpdates (std::vector<Ptr<MobilityModel> > mobilityStack)
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


void nrndnExample::SetPos(Ptr<MobilityModel> mob)
{
	ns3::Vector pos = mob->GetPosition();
	pos.x+=50;
	mob->SetPosition(pos);
	Simulator::Schedule (Seconds (1.0), &nrndnExample::SetPos, this, mob);
}

void
nrndnExample::InstallTestMobility()
{
//	Ptr<UniformRandomVariable> randomizer = CreateObject<UniformRandomVariable> ();
//	randomizer->SetAttribute ("Min", DoubleValue (10));
//	randomizer->SetAttribute ("Max", DoubleValue (100));

/*	MobilityHelper mobility;
	mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
	                               "X", PointerValue (randomizer),
	                               "Y", PointerValue (randomizer),
	                               "Z", PointerValue (randomizer));

	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

	mobility.Install (nodes);
*/
	  MobilityHelper mobility;
	  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
	                                 "MinX", DoubleValue (0.0),
	                                 "MinY", DoubleValue (0.0),
	                                 "DeltaX", DoubleValue (transRange),
	                                 "DeltaY", DoubleValue (0),
	                                 "GridWidth", UintegerValue (size),
	                                 "LayoutType", StringValue ("RowFirst"));
	  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	  mobility.Install (nodes);
}

void nrndnExample::InstallSensor()
{
	NS_LOG_INFO ("Installing Sensors");
	NodeSensorHelper sensorHelper;
	sensorHelper.SetSensorModel("ns3::ndn::nrndn::SumoNodeSensor",
			"sumodata",PointerValue(mobility));
	sensorHelper.InstallAll();
}

void nrndnExample::InstallTestApplications()
{
	//This test use 3 nodes, Consumer A and B send different interest to Producer C.
	//At time 5s, Data producer C change its data from A's interest to B's interest.
	//So before 5s, C response to A, and after 5s. C response to B
	ndn::AppHelper consumerHelper ("ns3::ndn::ConsumerCbr");
	//Ndn application for sending out Interest packets at a "constant" rate (Poisson process)
	consumerHelper.SetPrefix ("/prefix0");
	consumerHelper.SetAttribute ("Frequency", DoubleValue (1.0));

	//If you send the same interest packet for several times, the data producer will
	//response your interest packet respectively. It may be a waste. So we can set Consumer::MaxSeq
	//To limit the times interest packet send. For example,just 1.0
	consumerHelper.SetAttribute ("MaxSeq"/*"Maximum sequence number to request"*/,IntegerValue(10));
	consumerHelper.Install (nodes.Get(0));
	consumerHelper.SetPrefix ("/prefix1");
	consumerHelper.Install (nodes.Get(1));

	ndn::AppHelper producerHelper ("ns3::ndn::nrndn::nrProducer");
	producerHelper.SetPrefix ("/prefix0");
	producerHelper.SetAttribute ("PayloadSize", StringValue("1200"));
	producerHelper.Install (nodes.Get (2));

}

void nrndnExample::InstallTraffics()
{
	SeedManager::SetSeed(random_seed);
	UniformVariable rnd(0,nodes.GetN());
	std::cout<<"插入事件："<<accidentNum<<endl;
	if(random_accident)
	{
		
		for(uint32_t idx = 0; idx < certain_count; idx ++)
		{
			uint32_t index=rnd.GetValue();
			while(randomNode[index])
			{
				index=rnd.GetValue();
			}
			randomNode[index] = true;
			Ptr<ns3::ndn::nrndn::nrProducer> producer= DynamicCast<ns3::ndn::nrndn::nrProducer>(
					nodes.Get(index)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"]));
			NS_ASSERT(producer);
			producer->addAccident(certain_interval);
		}
		/*
		for(uint32_t i=0;i<accidentNum;++i)
		{
			uint32_t index=rnd.GetValue();
			Ptr<ns3::ndn::nrndn::nrProducer> producer= DynamicCast<ns3::ndn::nrndn::nrProducer>(
					nodes.Get(index)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"]));
			NS_ASSERT(producer);
			producer->addAccident(1);
		}*/
	}
	else
	{
		for(uint32_t index = 0; index < certain_count; index ++)
		{

			randomNode[index] = true;
			Ptr<ns3::ndn::nrndn::nrProducer> producer= DynamicCast<ns3::ndn::nrndn::nrProducer>(
					nodes.Get(index)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"]));
			NS_ASSERT(producer);
			producer->addAccident(certain_interval);
		}
	}
	std::cout<<"插入事件：完毕"<<endl;
	getchar();
}



void nrndnExample::InstallDistApplications()
{
	NS_LOG_INFO ("Installing Dist Applications");
	ndn::AppHelper consumerHelper ("ns3::ndn::nrndn::tradConsumer");
	consumerHelper.SetAttribute ("PayloadSize", UintegerValue (virtualPayloadSize));
	consumerHelper.Install(nodes);
	nrUtils::appIndex["ns3::ndn::nrndn::tradConsumer"]=0;

	ndn::AppHelper producerHelper ("ns3::ndn::nrndn::nrProducer");
	producerHelper.Install(nodes);
	nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"]=1;

	//Setup start and end time;
	for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
	{
		double start=mobility->GetStartTime((*i)->GetId());
		double stop =mobility->GetStopTime ((*i)->GetId());
		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::tradConsumer"])->SetAttribute("StartTime",TimeValue (Seconds (start)));
		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::tradConsumer"])->SetAttribute("StopTime", TimeValue (Seconds (stop )));

		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"])->SetAttribute("StartTime",TimeValue (Seconds (start)));
		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"])->SetAttribute("StopTime", TimeValue (Seconds (stop )));
	}
}

void nrndnExample::InstallCDSApplications()
{
	NS_LOG_INFO ("Installing CDS Applications");
	ndn::AppHelper consumerHelper ("ns3::ndn::nrndn::tradConsumer");
	consumerHelper.SetAttribute ("PayloadSize", UintegerValue (virtualPayloadSize));
	consumerHelper.Install(nodes);
	nrUtils::appIndex["ns3::ndn::nrndn::tradConsumer"]=0;

	ndn::AppHelper producerHelper ("ns3::ndn::nrndn::nrProducer");
	producerHelper.Install(nodes);
	nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"]=1;

	//Setup start and end time;
	for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
	{
		double start=mobility->GetStartTime((*i)->GetId());
		double stop =mobility->GetStopTime ((*i)->GetId());
		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::tradConsumer"])->SetAttribute("StartTime",TimeValue (Seconds (start)));
		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::tradConsumer"])->SetAttribute("StopTime", TimeValue (Seconds (stop )));

		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"])->SetAttribute("StartTime",TimeValue (Seconds (start)));
		(*i)->GetApplication(nrUtils::appIndex["ns3::ndn::nrndn::nrProducer"])->SetAttribute("StopTime", TimeValue (Seconds (stop )));
	}
}



void
nrndnExample::getStatistic()
{
	//1. get average arrival rate
	arrivalRate = nrUtils::GetAverageArrivalRate();

	//2. get average accurate rate
	accuracyRate=nrUtils::GetAverageAccurateRate();

	//3. get average hit rate
	hitRate = nrUtils::GetAverageHitRate();

	//4. get average delay
	averageDelay = nrUtils::GetAverageDelay();

	//5. get average data forward times
	pair<uint32_t,double> AverageDataForwardPair = nrUtils::GetAverageForwardTimes();
	averageForwardTimes = AverageDataForwardPair.second;

	//6. get average interest forward times
	pair<uint32_t,double> AverageInterestForwardPair = nrUtils::GetAverageInterestForwardTimes();
	averageInterestForwardTimes = AverageInterestForwardPair.second;

	disinterestRate=nrUtils::GetAverageDisinterestedRate();

	SumForwardTimes = AverageDataForwardPair.first + AverageInterestForwardPair.first;
}


