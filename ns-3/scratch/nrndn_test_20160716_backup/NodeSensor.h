/*
 * NodeSensor.h
 *
 *  Created on: Jan 4, 2015
 *      Author: chen
 */

#ifndef NODESENSOR_H_
#define NODESENSOR_H_


#include "ns3/object.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/timer.h"

namespace ns3
{
namespace ndn
{
namespace nrndn
{

class NodeSensor:public Object
{
public:
	  static TypeId GetTypeId ();


	NodeSensor();
	virtual ~NodeSensor();

	/*
	 * @brief get the X coordinate
	 * \return the X coordinate of the node
	 * */
	virtual double getX()		=0;

	/*
 	 * @brief get the Y coordinate
 	 * \return the Y coordinate of the node
 	 *
	 * */
	virtual double getY()		=0;

	/*
	 * @brief get the simulation time
	 * \return the simulation time
	 * */
	virtual double getTime()	=0;

	/*
	 * @brief get the offset of lane of the node currently at.Temporarily useless
	 * \return just return 0
	 *
	 **/
	//virtual double getPos();

	/*
	 * @brief get the direction angle of the node.Temporarily useless
	 * \return just return 0
	 *
	 **/
	virtual double getAngle()	=0;

	/*
	 * @brief get the lane slope of the node currently at. Temporarily useless
	 * \return just return 0
	 *
	 **/
	virtual double getSlope()	=0;

	/*
	 * @brief get the speed of the node.
	 * \return speed of the node in unit m/s
	 *
	 **/
	virtual double getSpeed()	=0;

	/*
	 * @brief get the type of the node. eg.TAXI,BUS,ect.
	 * \return the type name of the node
	 *
	 **/
	virtual const std::string& getType()=0;

	/*
	 * @brief get the lane name of the node currently at.
	 * \return the default name: NodeSensor::emptyLane
	 *
	 **/
	virtual const std::string& getLane();

	/*
	 * @brief get the navigation route of the node.
	 * \return list of navigation route, from the start lane to the terminal lane
	 *
	 **/
	virtual const std::vector<std::string>& getNavigationRoute()=0;

	/**
	 * 	\brief	1. Firstly, determine whether a given location is on the given navigation route
	 * 			   Secondly, calculate the DISTANCE between the local node position and remote position,
	 * 			   along with the given navigation route
	 *
	 * 				1.1 If it is on the route, return the pair (true, DISTANCE).
	 * 				Here, the DISTANCE means the distance with the given remote position.
	 * 				please notice it is not straight line distance,
	 * 				lane distance instead
	 * 				For example:
	 * 									||
	 * 								/	Node A
	 * 						5m	/		||3m
	 * 						/			||
	 * 				=====.===============
	 * 			    	 ^Node B  |
	 * 			     			  4m
	 * 				Node A and Node B has straight line distance of 5m
	 * 				Node A and Node B has lane distance of 7m
	 * 				Here I mean it should return 7m
	 *
	 *			    1.2 If either LOCAL node or the GIVEN position is not on the given route, just return (false, STRAIGHT_LINE_DISTANCE).
	 *
	 *
	 *			2.  If the given location is in front of the node,
	 *			    (The notion of front is according to the direction of given navigation route)
	 *					a POSITIVE(+) value should be returned.
	 *				otherwise,
	 *					a NEGATIVE(-) value should be returned.
	 *
	 *	@param	x		x coordinate of given location
	 *	@param	y		y coordinate of given location
	 *	@param route	the given navigation route
	 *	\return	(true,distance)
	 *					 if it is in the front of the node, and along with the distance
	 *					 POSITIVE(+) distance for front positions
	 *					 NEGATIVE(-) distance for back  positions
	 *			(false,straight_line_distance)
	 *					otherwise. Notice the straight line distance is always positive(+)
	 */
	virtual std::pair<bool, double> getDistanceWith(const double& x,const double& y,const std::vector<std::string>& route)=0;

	const static std::string emptyType;
	const static std::string emptyLane;

protected:
    TracedValue<std::string> m_lane;

    /**
     *  @brief	m_lane is a traceable value, however, it can only forwarding calls to a single function
     *  		In order to forward to more functions, m_lane's callback connnects to m_laneChangeCallback
     *  		throught member function NodeSensor::LaneChangeEvent
     * */
    TracedCallback<std::string,std::string> m_laneChangeCallback;

private:
	/**
	 *  @brief	m_lane is a traceable value, however, it can only forwarding calls to a single function
	 *  		In order to forward to more functions, m_lane's callback connnects to m_laneChangeCallback
	 *  		throught member function NodeSensor::LaneChangeEvent
	 * */
    void LaneChangeEvent(std::string oldLane, std::string newLane);

    // Reflash the lane name every 1s
    Timer m_laneTimer;
};

} /* namespace nrndn */
} /* namespace ndn */
} /* namespace ns3 */

#endif /* NODESENSOR_H_ */
