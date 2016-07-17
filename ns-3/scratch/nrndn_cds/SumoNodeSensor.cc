/*
 * SumoNodeSensor.cc
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */

#include "SumoNodeSensor.h"

#include "ns3/RouteElement.h"

namespace ns3
{
namespace ndn
{
namespace nrndn
{
using namespace std;
NS_OBJECT_ENSURE_REGISTERED (SumoNodeSensor);

SumoNodeSensor::SumoNodeSensor()
{
	// TODO Auto-generated constructor stub

}

SumoNodeSensor::~SumoNodeSensor()
{
	// TODO Auto-generated destructor stub
}

TypeId SumoNodeSensor::GetTypeId()
{
	  static TypeId tid = TypeId ("ns3::ndn::nrndn::SumoNodeSensor")
	    .SetGroupName ("Nrndn")
	    .SetParent<NodeSensor> ()
	    .AddConstructor<SumoNodeSensor> ()
	    .AddAttribute("sumodata", "The trace data read by sumo files.",
	   	    		PointerValue (),
	   	    		MakePointerAccessor (&SumoNodeSensor::m_sumodata),
	   	    		MakePointerChecker<vanetmobility::sumomobility::SumoMobility> ())
	    ;
	  return tid;
}

double SumoNodeSensor::getX()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getX():Cannot find Trace!!");
	NS_ASSERT_MSG(m_sumodata->GetTrace(m_id,pos).x == pos.x,"Can not find coordinate x");
	return m_sumodata->GetTrace(m_id,pos).x;

}

double SumoNodeSensor::getY()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getY():Cannot find Trace!!");
	NS_ASSERT_MSG(m_sumodata->GetTrace(m_id,pos).y == pos.y,"Can not find coordinate y");
	return m_sumodata->GetTrace(m_id,pos).y;
}

double SumoNodeSensor::getPos()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getPos():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).pos;
}

double SumoNodeSensor::getTime()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getTime():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).time;
}

double SumoNodeSensor::getAngle()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getAngle():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).angle;
}

double SumoNodeSensor::getSlope()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getSlope():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).slope;
}

double SumoNodeSensor::getSpeed()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	NS_ASSERT_MSG(&m_sumodata->GetTrace(m_id,pos)!=NULL,"SumoNodeSensor::getSpeed():Cannot find Trace!!");
	return m_sumodata->GetTrace(m_id,pos).speed;
}

const std::string& SumoNodeSensor::getType()
{
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t m_id = node->GetId();
	if(&m_sumodata->GetTrace(m_id,pos)==NULL)
		return emptyType;
	return m_sumodata->GetTrace(m_id,pos).type;
}

const std::string& SumoNodeSensor::getLane()
{
	NodeSensor::getLane();
	Ptr<MobilityModel> mobility=this->GetObject<MobilityModel>();
	Vector pos = mobility->GetPosition();
	Ptr<Node> node = this->GetObject<Node>();
	uint32_t id = node->GetId();
	if(&(m_sumodata->GetTrace(id,pos))==NULL)
        m_lane.Set(emptyLane);
    else
    	m_lane.Set(m_sumodata->GetTrace(id,pos).lane);
	//std::cout<<"id's current lane "<<m_lane<<std::endl;
	m_sumoLane = m_lane.Get();
    return m_sumoLane;
}

const std::vector<std::string>& SumoNodeSensor::getNavigationRoute()
{
	if(!m_navigationRoute.empty())
		return m_navigationRoute;
	else
	{
		//Set the navigation route according to the m_sumodata at the first time invoked
		Ptr<Node> node		= this->GetObject<Node>();
		uint32_t id			= node->GetId();
		m_navigationRoute	= m_sumodata->getVl().getVehicles()[id].route.edgesID;
	}
	return m_navigationRoute;
}
std::string SumoNodeSensor::uriConvertToString(std::string str)
{
	//因为获取兴趣时使用toUri，避免出现类似[]的符号，进行编码转换
	std::string ret="";
	for(uint32_t i=0;i<str.size();i++)
	{
		if(i+2<str.size())
		{
			if(str[i]=='%'&&str[i+1]=='5'&&str[i+2]=='B')
			{
				ret+="[";
				i=i+2;
			}
			else if(str[i]=='%'&&str[i+1]=='5'&&str[i+2]=='D')
			{
				ret+="]";
				i=i+2;
			}
			else
				ret+=str[i];
		}
		else
			ret+=str[i];
	}
	return ret;
}

std::pair<bool, double> SumoNodeSensor::getDistanceWith(const double& x,const double& y,const std::vector<std::string>& route)
{
	const string& localLane = getLane();
	const double& localPos  = getPos();

	vector<string>::const_iterator localLaneIterator;
	vector<string>::const_iterator remoteLaneIterator;

	std::pair<std::string, double> remoteInfo =
			convertCoordinateToLanePos(x,y);

	const string& remoteLane = remoteInfo.first;
	const double& remotePos  = remoteInfo.second;

	localLaneIterator  = std::find (route.begin(), route.end(), localLane);
	remoteLaneIterator = std::find (route.begin(), route.end(), remoteLane);

	if(remoteLaneIterator==route.end()||
			localLaneIterator==route.end())
	{
		Vector 	localPos = GetObject<MobilityModel>()->GetPosition();
		localPos.z=0;//Just in case
		Vector remotePos(x,y,0);
		return std::pair<bool, double>(false,CalculateDistance(localPos,remotePos));
	}

	uint32_t localIndex = distance(route.begin(),localLaneIterator);
	uint32_t remoteIndex= distance(route.begin(),remoteLaneIterator);

	if(localIndex==remoteIndex)
		return pair<bool, double>(true, remotePos-localPos);

	double distance,beginLen, middleLen;
	distance = beginLen = middleLen = 0;
	const map<string,vanetmobility::sumomobility::Edge>& edges = m_sumodata->getRoadmap().getEdges();
	std::map<std::string,vanetmobility::sumomobility::Edge>::const_iterator eit;
	if(localIndex>remoteIndex)
	{
		//From behind
		eit = edges.find(uriConvertToString(route.at(remoteIndex)));
		NS_ASSERT_MSG(eit!=edges.end(),"No edge info for "<<remoteLane);
		//=======.=======|=========|========|===============.=============
		//       remote(3)	(4)			(5)				local(6)
		//       |-------|------------------|---------------|
		//       beginLen     middleLen             localPos
		beginLen = eit->second.lane.length - remotePos ;
		for(uint32_t i = remoteIndex + 1; i<localIndex;++i)
		{
			eit = edges.find(uriConvertToString(route.at(i)));
			NS_ASSERT_MSG(eit!=edges.end(),"No edge info for "<<uriConvertToString(route.at(i)));
			middleLen+=eit->second.lane.length;
		}
		distance = -(beginLen+middleLen+localPos);
	}
	else
	{
		//From front
		eit = edges.find(uriConvertToString(route.at(localIndex)));
		NS_ASSERT_MSG(eit!=edges.end(),"No edge info for "<<remoteLane);
		//=======.=======|=========|========|===============.=============
		//       local(3)	(4)			(5)				remote(6)
		//       |-------|------------------|---------------|
		//       beginLen     middleLen             remotePos
		beginLen = eit->second.lane.length - localPos ;
		for(uint32_t i = localIndex + 1; i<remoteIndex;++i)
		{
			eit = edges.find(uriConvertToString(route.at(i)));
			NS_ASSERT_MSG(eit!=edges.end(),"No edge info for "<<uriConvertToString(route.at(i)));
			middleLen+=eit->second.lane.length;
		}
		distance = beginLen+middleLen+remotePos;

	}
	return std::pair<bool, double>(true,distance);

}

std::pair<std::string, double> SumoNodeSensor::convertCoordinateToLanePos(
		const double& x, const double& y)
{

	const vanetmobility::sumomobility::SumoMobility::CoordinateToLaneType& cvTable=
			m_sumodata->getCoordinateToLane();
	vanetmobility::sumomobility::SumoMobility::CoordinateToLaneType::const_iterator it;
	it=cvTable.find(Vector2D(x,y));
	NS_ASSERT_MSG(it!=cvTable.end(),"cannot find the trace of ("<<x<<","<<y<<")");
	return it->second;
}

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */
