/*
 * SumoNodeSensor.h
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */

#ifndef SUMONODESENSOR_H_
#define SUMONODESENSOR_H_

#include "NodeSensor.h"
#include "ns3/SumoMobility.h"

#include <unordered_map>

namespace ns3
{
namespace ndn
{
namespace nrndn
{

class SumoNodeSensor: public NodeSensor
{
public:
	 static TypeId GetTypeId ();

	SumoNodeSensor();
	virtual ~SumoNodeSensor();

	virtual double getX();
	virtual double getY();
			double getPos();
	virtual double getTime();
	virtual double getAngle();
	virtual double getSlope();
	virtual double getSpeed();
	virtual const std::string& getType() ;
	virtual const std::string& getLane();
	virtual const std::vector<std::string>& getNavigationRoute();

	virtual std::pair<bool, double> getDistanceWith(const double& x,const double& y,const std::vector<std::string>& route);

private:
	/*
	 * @brief convert the coordinate (x,y) to the lane and offset pair
	 * \return (lane,offset)
	 * */
	std::pair<std::string,double> convertCoordinateToLanePos(const double& x,const double& y);


private:
	Ptr<vanetmobility::sumomobility::SumoMobility> m_sumodata;
	std::vector<std::string> m_navigationRoute;
	std::string m_sumoLane;


};

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* SUMONODESENSOR_H_ */
