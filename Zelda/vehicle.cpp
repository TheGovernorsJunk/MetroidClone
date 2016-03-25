#include "vehicle.h"

namespace te
{
	Vehicle::Vehicle()
		: mSteering(*this)
	{}

	const SteeringBehaviors& Vehicle::getSteering() const
	{
		return mSteering;
	}

	SteeringBehaviors& Vehicle::getSteering()
	{
		return mSteering;
	}
}
