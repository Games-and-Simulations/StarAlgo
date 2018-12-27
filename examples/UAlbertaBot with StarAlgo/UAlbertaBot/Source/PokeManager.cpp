#include "PokeManager.h"
#include "UnitUtil.h"

using namespace UAlbertaBot;

PokeManager::PokeManager()
{
}

static bool isAntiAir(BWAPI::Unit target)
{
	return target->getType().airWeapon() != BWAPI::WeaponTypes::None ? true : false;
}

static bool isThreaty(BWAPI::Unit rangedUnit, BWAPI::Unit target)
{
	return rangedUnit->getType().isFlyer() ? isAntiAir(target) : target->getType().groundWeapon() != BWAPI::WeaponTypes::None;
}

void PokeManager::executeMicro(const BWAPI::Unitset & targets)
{
	const BWAPI::Unitset & rangedUnits = getUnits();
	BWAPI::Unitset rangedUnitTargets;
	std::copy_if(targets.begin(), targets.end(),
		std::inserter(rangedUnitTargets, rangedUnitTargets.end()),
		[](BWAPI::Unit u){ return u->isVisible(); });

	if (unitToDie == nullptr || unitToDie->getHitPoints() <= 0) // find dangerous unit
	{
		for (auto & u : rangedUnitTargets)
		{
			if (isAntiAir(u))
			{
				unitToDie = u;
				break;
			}
		}
	}
	isAttacking = (unitToDie == nullptr || 
		(unitToDie->getHitPoints() > 0 ||
		(getUnits().size() && unitNearEnemy(*getUnits().begin())))); // TODO run on closest to enemy unit
	//(BWAPI::Broodwar->enemy()->getUnits().size() == 0))); // 

	for (auto & rangedUnit : rangedUnits)
	{
		if (isAttacking)
		{
			if (!rangedUnitTargets.empty())
			{
				BWAPI::Unit target = getTarget(rangedUnit, rangedUnitTargets);
				/*if (target && Config::Debug::DrawUnitTargetInfo)
				{
				BWAPI::Broodwar->drawLineMap(rangedUnit->getPosition(), rangedUnit->getTargetPosition(), BWAPI::Colors::Purple);
				}*/
				Micro::MutaDanceTarget(rangedUnit, target);
			}
			else // no targets
			{
				// if we're not near the order position
				if (rangedUnit->getDistance(order.getPosition()) > 100)
				{
					// move to it
					Micro::SmartAttackMove(rangedUnit, order.getPosition());
				}
			}
		}
		else
		{
			BWAPI::Unit target = getTarget(rangedUnit, rangedUnitTargets);
			if (target)
				rangedUnit->move(Micro::GetKiteVector(rangedUnit, target)); // target must not be empty
		}
	}
}

int PokeManager::getAttackPriority(BWAPI::Unit rangedUnit, BWAPI::Unit target)
{
	BWAPI::UnitType rangedType = rangedUnit->getType();
	BWAPI::UnitType targetType = target->getType();


	if (rangedUnit->getType() == BWAPI::UnitTypes::Zerg_Scourge)
	{
		if (target->getType() == BWAPI::UnitTypes::Protoss_Carrier)
		{

			return 100;
		}

		if (target->getType() == BWAPI::UnitTypes::Protoss_Corsair)
		{
			return 90;
		}
	}

	bool isThreat = isThreaty(rangedUnit, target);

	if (target->getType().isWorker())
	{
		isThreat = false;
	}

	if (target->getType() == BWAPI::UnitTypes::Zerg_Larva || target->getType() == BWAPI::UnitTypes::Zerg_Egg)
	{
		return 0;
	}

	if (rangedUnit->isFlying() && target->getType() == BWAPI::UnitTypes::Protoss_Carrier)
	{
		return 101;
	}

	// if the target is building something near our base something is fishy
	BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
	if (target->getType().isWorker() && (target->isConstructing() || target->isRepairing()) && target->getDistance(ourBasePosition) < 1200)
	{
		return 100;
	}

	if (target->getType().isBuilding() && (target->isCompleted() || target->isBeingConstructed()) && target->getDistance(ourBasePosition) < 1200)
	{
		return 90;
	}

	// highest priority is something that can attack us or aid in combat
	if (targetType == BWAPI::UnitTypes::Terran_Bunker || isThreat)
	{
		return 11;
	}
	// next priority is worker
	else if (targetType.isWorker())
	{
		if (rangedUnit->getType() == BWAPI::UnitTypes::Terran_Vulture)
		{
			return 11;
		}

		return 11;
	}
	// next is special buildings
	else if (targetType == BWAPI::UnitTypes::Zerg_Spawning_Pool)
	{
		return 5;
	}
	// next is special buildings
	else if (targetType == BWAPI::UnitTypes::Protoss_Pylon)
	{
		return 5;
	}
	// next is buildings that cost gas
	else if (targetType.gasPrice() > 0)
	{
		return 4;
	}
	else if (targetType.mineralPrice() > 0)
	{
		return 3;
	}
	// then everything else
	else
	{
		return 1;
	}
}

BWAPI::Unit PokeManager::getTarget(BWAPI::Unit rangedUnit, const BWAPI::Unitset & targets)
{
	int highPriority = 0;
	double closestDist = std::numeric_limits<double>::infinity();
	BWAPI::Unit closestTarget = nullptr;

	for (const auto & target : targets)
	{
		double distance = rangedUnit->getDistance(target);
		double LTD = UnitUtil::CalculateLTD(target, rangedUnit);
		int priority = getAttackPriority(rangedUnit, target);
		bool targetIsThreat = LTD > 0;

		if (!closestTarget || (priority > highPriority) || (priority == highPriority && distance < closestDist))
		{
			closestDist = distance;
			highPriority = priority;
			closestTarget = target;
		}
	}

	return closestTarget;
}