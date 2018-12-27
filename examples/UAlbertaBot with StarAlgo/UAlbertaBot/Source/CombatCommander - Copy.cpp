#include "CombatCommander.h"
#include "UnitUtil.h"

// Staralgo
#include "Squad.h"
#include "../../StarAlgo/Source/UnitInfoStatic.h"
#include "../../StarAlgo/Source/CombatSimDecreased.h"
#include "../../StarAlgo/Source/MCTSCD.h"
#include "../../StarAlgo/Source/StarAlgoExtra.h"
#include "../../StarAlgo/Source/GameState.h"
#include "../../StarAlgo/Source/EvaluationFunction.h"
#include "../../StarAlgo/Source/Negamax.h"
#include "../../StarAlgo/Source/RegionManager.h"


using namespace UAlbertaBot;

const size_t IdlePriority = 0;
const size_t AttackPriority = 1;
const size_t BaseDefensePriority = 2;
const size_t ScoutDefensePriority = 3;
const size_t DropPriority = 4;

CombatCommander::CombatCommander() 
    : _initialized(false)
{

}

// Staralgo
static CombatSimulator* combatSim; // TODO shared_ptr?
static MCTSCD* searchAlg; // TODO better name
static UnitInfoStatic* unitStatic;
static RegionManager* regionManager;

void CombatCommander::initializeSquads()
{
    SquadOrder idleOrder(SquadOrderTypes::Idle, BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation()), 100, "Chill Out");
	_squadData.addSquad("Idle", Squad("Idle", idleOrder, IdlePriority));

    // the main attack squad that will pressure the enemy's closest base location
    SquadOrder one(SquadOrderTypes::Attack, getMainAttackLocation(), 800, "MC");
	_squadData.addSquad("One", Squad("One", one, AttackPriority));

    // the main attack squad that will pressure the enemy's closest base location
    SquadOrder two(SquadOrderTypes::Attack, getMainAttackLocation(), 800, "MC");
	_squadData.addSquad("Two", Squad("Two", two, AttackPriority));

    // the main attack squad that will pressure the enemy's closest base location
    SquadOrder three(SquadOrderTypes::Attack, getMainAttackLocation(), 800, "MC");
	_squadData.addSquad("Three", Squad("Three", three, AttackPriority));

    // the main attack squad that will pressure the enemy's closest base location
    SquadOrder four(SquadOrderTypes::Attack, getMainAttackLocation(), 800, "MC");
	_squadData.addSquad("Four", Squad("Four", four, AttackPriority));

    // the main attack squad that will pressure the enemy's closest base location
    SquadOrder five(SquadOrderTypes::Attack, getMainAttackLocation(), 800, "MC");
	_squadData.addSquad("Five", Squad("Five", five, AttackPriority));

    BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());

    // the scout defense squad will handle chasing the enemy worker scout
    SquadOrder enemyScoutDefense(SquadOrderTypes::Defend, ourBasePosition, 900, "Get the scout");
    _squadData.addSquad("ScoutDefense", Squad("ScoutDefense", enemyScoutDefense, ScoutDefensePriority));

    // add a drop squad if we are using a drop strategy
    if (Config::Strategy::StrategyName == "Protoss_Drop")
    {
        SquadOrder zealotDrop(SquadOrderTypes::Drop, ourBasePosition, 900, "Wait for transport");
        _squadData.addSquad("Drop", Squad("Drop", zealotDrop, DropPriority));
    }

    _initialized = true;

    // Staralgo
    unitStatic = new UnitInfoStatic;
    // Compute max DPS and HP of each unit
    // set combat simulator
    // 	CombatSimulator* combatSim = new CombatSimLanchester(&unitStatic->DPF);
    // 	CombatSimulator* combatSim = new CombatSimSustained(&unitStatic->DPF);
    combatSim = new CombatSimDecreased(&unitStatic->typeDPF, &unitStatic->DPF);
    EvaluationFunction ef;
    int depth = 10;//LoadConfigInt("MCTSCD", "depth", 1);
    int iterations = 100;//LoadConfigInt("MCTSCD", "iterations");
    int maxSimTime = 288;//LoadConfigInt("MCTSCD", "max_simulation_time");
    regionManager = new RegionManager;
    searchAlg = new MCTSCD(depth, iterations, maxSimTime, &ef, regionManager);
}

bool CombatCommander::isSquadUpdateFrame()
{
	return BWAPI::Broodwar->getFrameCount() % 10 == 0;
}

void CombatCommander::update(const BWAPI::Unitset & combatUnits)
{
    if (!Config::Modules::UsingCombatCommander)
    {
        return;
    }

    if (!_initialized)
    {
        initializeSquads();
    }

    _combatUnits = combatUnits;


	if (isSquadUpdateFrame())
	{
        updateIdleSquad();
        updateDropSquads();
        updateScoutDefenseSquad();
		updateDefenseSquads();
        int frames = BWAPI::Broodwar->getFrameCount();
        if (frames % 200 == 0 && frames > 5000)
            myUpdateSquads();
            //updateAttackSquads();
	}

	_squadData.update();
}

void assignIfCan(SquadData& sd, BWAPI::Unit unit, Squad & squad) {
    if (sd.canAssignUnitToSquad(unit, squad)) {
        sd.assignUnitToSquad(unit, squad);
    }
}

void CombatCommander::myUpdateSquads() {
    std::vector<Squad*> squads;
    squads.push_back(&_squadData.getSquad("One"));
    squads.push_back(&_squadData.getSquad("Two"));
    squads.push_back(&_squadData.getSquad("Three"));
    squads.push_back(&_squadData.getSquad("Four"));
    squads.push_back(&_squadData.getSquad("Five"));

    std::map<BWAPI::Region, std::vector<BWAPI::Unit>> unitRegions;
    for (auto & unit : _combatUnits) {
        unitRegions[unit->getRegion()].push_back(unit);
    }

    int i = 0;
    for (auto it = unitRegions.begin(); it != unitRegions.end(); ++it, ++i) {
        for (auto& unit : it->second) {
            assignIfCan(_squadData, unit, *squads[i%5]);
        }
    }

    GameState gs(combatSim, regionManager);
    gs.addAllEnemyUnits();
    //gs.addSelfBuildings();

    //unsigned groupId = searchAlg->addSquadToGameState(gs, _combatUnits); // MB split?
    unsigned groupId1 = searchAlg->addSquadToGameState(gs, squads[1]->getUnits());
    unsigned groupId2 = searchAlg->addSquadToGameState(gs, squads[1]->getUnits());
    unsigned groupId3 = searchAlg->addSquadToGameState(gs, squads[2]->getUnits());
    unsigned groupId4 = searchAlg->addSquadToGameState(gs, squads[3]->getUnits());
    unsigned groupId5 = searchAlg->addSquadToGameState(gs, squads[4]->getUnits());


    playerActions_t bestActions = searchAlg->start(gs);
    BWAPI::Broodwar->drawTextScreen(200, 50, "\x08 MCTSCD");
    BWAPI::Broodwar->drawTextScreen(200, 60, "\x08 bestActions.size(): %d", bestActions.size());
    //BWAPI::Broodwar->drawTextScreen(200, 70, "\x08 groupIds: %d %d %d %d %d", groupId1, groupId2, groupId3, groupId4, groupId5);
    for (int i = 0; i < bestActions.size(); ++i) // remove
        BWAPI::Broodwar->drawTextScreen(200, 80 + i * 10, "\x08 bestActions: %d, %d, %d", bestActions[i].action.orderID, bestActions[i].action.targetRegion, bestActions[i].pos);
    //std::map<SquadAgent*, BWAPI::Position> bestOrders;
    //BWAPI::Position targetPosition;
 
    for (const auto& groupAction : bestActions) {
        BWAPI::Unitset squadSet = searchAlg->_idToSquad[groupAction.pos];
        for (auto& squad : squadSet) {
            targetPosition = gs.getCenterRegionId(groupAction.action.targetRegion);
            bestOrders[squad] = targetPosition;
            mainAttackOrder = SquadOrder(SquadOrderTypes::Attack, targetPosition, 800, "MCTSCD move");
        }
    }
 

    for (int i = 0; (i < bestActions.size()) && (i < 4); ++i) { // NOT NECESSARY BEST
        SquadOrder order(SquadOrderTypes::Attack, gs._regman->getCenterRegionId(bestActions[i].action.targetRegion), 800, "MC");
        for (auto& unit : _combatUnits) {
            BWAPI::Position pos = unit->getRegion()->getCenter();
            BWAPI::Position pos2 = gs._regman->_regionFromID[gs._army.friendly[bestActions[i].pos]->regionId]->getCenter();
            int id = unit->getType().getID();
            int id2 = gs._army.friendly[bestActions[i].pos]->unitTypeId;
            if (pos == pos2 && // TODO debug
                id == id2) {
                squads[i]->addUnit(unit);
            }
        }
        squads[i]->setSquadOrder(order);
    }
    SquadOrder five;
    for (int i = 4; i < bestActions.size(); ++i) {
        five = SquadOrder(SquadOrderTypes::Attack, gs._regman->getCenterRegionId(bestActions[i].action.targetRegion), 800, "MC");
        for (auto& unit : _combatUnits) {
            if (unit->getRegion()->getID() == gs._army.friendly[bestActions[i].pos]->regionId && // TODO debug
                unit->getType() == gs._army.friendly[bestActions[i].pos]->unitTypeId) {
                squads[4]->addUnit(unit); // rest to last
            }
        }
    }
    squads[4]->setSquadOrder(five);

    //SquadOrder oneOrder(SquadOrderTypes::Attack, gs._regman->getCenterRegionId(bestActions[groupId1].action.targetRegion), 800, "MC");
    //squads[0]->setSquadOrder(oneOrder);
    //SquadOrder twoOrder(SquadOrderTypes::Attack, gs._regman->getCenterRegionId(bestActions[groupId2].action.targetRegion), 800, "MC");
    //squads[1]->setSquadOrder(twoOrder);
    //SquadOrder threeOrder(SquadOrderTypes::Attack, gs._regman->getCenterRegionId(bestActions[groupId3].action.targetRegion), 800, "MC");
    //squads[2]->setSquadOrder(threeOrder);
    //SquadOrder fourOrder(SquadOrderTypes::Attack, gs._regman->getCenterRegionId(bestActions[groupId4].action.targetRegion), 800, "MC");
    //squads[3]->setSquadOrder(fourOrder);
    //SquadOrder fiveOrder(SquadOrderTypes::Attack, gs._regman->getCenterRegionId(bestActions[groupId5].action.targetRegion), 800, "MC");
    //squads[4]->setSquadOrder(fiveOrder);
}

void CombatCommander::updateIdleSquad()
{
    Squad & idleSquad = _squadData.getSquad("Idle");
    for (auto & unit : _combatUnits)
    {
        // if it hasn't been assigned to a squad yet, put it in the low priority idle squad
        if (_squadData.canAssignUnitToSquad(unit, idleSquad))
        {
            idleSquad.addUnit(unit);
        }
    }
}

void CombatCommander::updateAttackSquads()
{
    Squad & mainAttackSquad = _squadData.getSquad("MainAttack");

    for (auto & unit : _combatUnits)
    {
        if (unit->getType() == BWAPI::UnitTypes::Zerg_Scourge && UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Hydralisk) < 30)
        {
            continue;
        }

        // get every unit of a lower priority and put it into the attack squad
        if (!unit->getType().isWorker() && (unit->getType() != BWAPI::UnitTypes::Zerg_Overlord) && _squadData.canAssignUnitToSquad(unit, mainAttackSquad) &&
            unit->getType() == BWAPI::UnitTypes::Zerg_Hydralisk)
        {
            _squadData.assignUnitToSquad(unit, mainAttackSquad);
        }
    }

    SquadOrder mainAttackOrder(SquadOrderTypes::Attack, getMainAttackLocation(&mainAttackSquad), 800, "Attack Enemy Base");
    mainAttackSquad.setSquadOrder(mainAttackOrder);

    Squad & extraAttackSquad = _squadData.getSquad("ExtraAttack");

    for (auto & unit : _combatUnits)
    {
        if (unit->getType() == BWAPI::UnitTypes::Zerg_Scourge && UnitUtil::GetAllUnitCount(BWAPI::UnitTypes::Zerg_Hydralisk) < 30)
        {
            continue;
        }

        // get every unit of a lower priority and put it into the attack squad
        if (!unit->getType().isWorker() && (unit->getType() != BWAPI::UnitTypes::Zerg_Overlord) && _squadData.canAssignUnitToSquad(unit, extraAttackSquad) &&
            unit->getType() == BWAPI::UnitTypes::Zerg_Zergling)
        {
            _squadData.assignUnitToSquad(unit, extraAttackSquad);
        }
    }

    SquadOrder extraAttackOrder(SquadOrderTypes::Attack, getMainAttackLocation(&extraAttackSquad), 800, "Assist");
    extraAttackSquad.setSquadOrder(extraAttackOrder);
}

void CombatCommander::updateDropSquads()
{
    if (Config::Strategy::StrategyName != "Protoss_Drop")
    {
        return;
    }

    Squad & dropSquad = _squadData.getSquad("Drop");

    // figure out how many units the drop squad needs
    bool dropSquadHasTransport = false;
    int transportSpotsRemaining = 8;
    auto & dropUnits = dropSquad.getUnits();

    for (auto & unit : dropUnits)
    {
        if (unit->isFlying() && unit->getType().spaceProvided() > 0)
        {
            dropSquadHasTransport = true;
        }
        else
        {
            transportSpotsRemaining -= unit->getType().spaceRequired();
        }
    }

    // if there are still units to be added to the drop squad, do it
    if (transportSpotsRemaining > 0 || !dropSquadHasTransport)
    {
        // take our first amount of combat units that fill up a transport and add them to the drop squad
        for (auto & unit : _combatUnits)
        {
            // if this is a transport unit and we don't have one in the squad yet, add it
            if (!dropSquadHasTransport && (unit->getType().spaceProvided() > 0 && unit->isFlying()))
            {
                _squadData.assignUnitToSquad(unit, dropSquad);
                dropSquadHasTransport = true;
                continue;
            }

            if (unit->getType().spaceRequired() > transportSpotsRemaining)
            {
                continue;
            }

            // get every unit of a lower priority and put it into the attack squad
            if (!unit->getType().isWorker() && _squadData.canAssignUnitToSquad(unit, dropSquad))
            {
                _squadData.assignUnitToSquad(unit, dropSquad);
                transportSpotsRemaining -= unit->getType().spaceRequired();
            }
        }
    }
    // otherwise the drop squad is full, so execute the order
    else
    {
        SquadOrder dropOrder(SquadOrderTypes::Drop, getMainAttackLocation(), 800, "Attack Enemy Base");
        dropSquad.setSquadOrder(dropOrder);
    }
}

void CombatCommander::updateScoutDefenseSquad() 
{
    if (_combatUnits.empty()) 
    { 
        return; 
    }

    // if the current squad has units in it then we can ignore this
    Squad & scoutDefenseSquad = _squadData.getSquad("ScoutDefense");
  
    // get the region that our base is located in
    BWTA::Region * myRegion = BWTA::getRegion(BWAPI::Broodwar->self()->getStartLocation());
    if (!myRegion && myRegion->getCenter().isValid())
    {
        return;
    }

    // get all of the enemy units in this region
	BWAPI::Unitset enemyUnitsInRegion;
    for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
    {
        if (BWTA::getRegion(BWAPI::TilePosition(unit->getPosition())) == myRegion)
        {
            enemyUnitsInRegion.insert(unit);
        }
    }

    // if there's an enemy worker in our region then assign someone to chase him
    bool assignScoutDefender = enemyUnitsInRegion.size() == 1 && (*enemyUnitsInRegion.begin())->getType().isWorker();

    // if our current squad is empty and we should assign a worker, do it
    if (scoutDefenseSquad.isEmpty() && assignScoutDefender)
    {
        // the enemy worker that is attacking us
        BWAPI::Unit enemyWorker = *enemyUnitsInRegion.begin();

        // get our worker unit that is mining that is closest to it
        BWAPI::Unit workerDefender = findClosestWorkerToTarget(_combatUnits, enemyWorker);

		if (enemyWorker && workerDefender)
		{
			// grab it from the worker manager and put it in the squad
            if (_squadData.canAssignUnitToSquad(workerDefender, scoutDefenseSquad))
            {
			    WorkerManager::Instance().setCombatWorker(workerDefender);
                _squadData.assignUnitToSquad(workerDefender, scoutDefenseSquad);
            }
		}
    }
    // if our squad is not empty and we shouldn't have a worker chasing then take him out of the squad
    else if (!scoutDefenseSquad.isEmpty() && !assignScoutDefender)
    {
        for (auto & unit : scoutDefenseSquad.getUnits())
        {
            unit->stop();
            if (unit->getType().isWorker())
            {
                WorkerManager::Instance().finishedWithWorker(unit);
            }
        }

        scoutDefenseSquad.clear();
    }
}

void CombatCommander::updateDefenseSquads() 
{
	if (_combatUnits.empty()) 
    { 
        return; 
    }
    
    BWTA::BaseLocation * enemyBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());
    BWTA::Region * enemyRegion = nullptr;
    if (enemyBaseLocation)
    {
        enemyRegion = BWTA::getRegion(enemyBaseLocation->getPosition());
    }

	// for each of our occupied regions
	for (BWTA::Region * myRegion : InformationManager::Instance().getOccupiedRegions(BWAPI::Broodwar->self()))
	{
        // don't defend inside the enemy region, this will end badly when we are stealing gas
        if (myRegion == enemyRegion)
        {
            continue;
        }

		BWAPI::Position regionCenter = myRegion->getCenter();
		if (!regionCenter.isValid())
		{
			continue;
		}

		// start off assuming all enemy units in region are just workers
		int numDefendersPerEnemyUnit = 2;

		// all of the enemy units in this region
		BWAPI::Unitset enemyUnitsInRegion;
        for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
        {
            // if it's an overlord, don't worry about it for defense, we don't care what they see
            if (unit->getType() == BWAPI::UnitTypes::Zerg_Overlord)
            {
                continue;
            }

            if (BWTA::getRegion(BWAPI::TilePosition(unit->getPosition())) == myRegion)
            {
                enemyUnitsInRegion.insert(unit);
            }
        }

        // we can ignore the first enemy worker in our region since we assume it is a scout
        for (auto & unit : enemyUnitsInRegion)
        {
            if (unit->getType().isWorker())
            {
                enemyUnitsInRegion.erase(unit);
                break;
            }
        }

        int numEnemyFlyingInRegion = std::count_if(enemyUnitsInRegion.begin(), enemyUnitsInRegion.end(), [](BWAPI::Unit u) { return u->isFlying(); });
        int numEnemyGroundInRegion = std::count_if(enemyUnitsInRegion.begin(), enemyUnitsInRegion.end(), [](BWAPI::Unit u) { return !u->isFlying(); });

        std::stringstream squadName;
        squadName << "Base Defense " << regionCenter.x << " " << regionCenter.y; 
        
        // if there's nothing in this region to worry about
        if (enemyUnitsInRegion.empty())
        {
            // if a defense squad for this region exists, remove it
            if (_squadData.squadExists(squadName.str()))
            {
                _squadData.getSquad(squadName.str()).clear();
            }
            
            // and return, nothing to defend here
            continue;
        }
        else 
        {
            // if we don't have a squad assigned to this region already, create one
            if (!_squadData.squadExists(squadName.str()))
            {
                SquadOrder defendRegion(SquadOrderTypes::Defend, regionCenter, 32 * 25, "Defend Region!");
                _squadData.addSquad(squadName.str(), Squad(squadName.str(), defendRegion, BaseDefensePriority));
            }
        }

        // assign units to the squad
        if (_squadData.squadExists(squadName.str()))
        {
            Squad & defenseSquad = _squadData.getSquad(squadName.str());

            // figure out how many units we need on defense
	        int flyingDefendersNeeded = numDefendersPerEnemyUnit * numEnemyFlyingInRegion;
	        int groundDefensersNeeded = numDefendersPerEnemyUnit * numEnemyGroundInRegion;

            updateDefenseSquadUnits(defenseSquad, flyingDefendersNeeded, groundDefensersNeeded);
        }
        else
        {
            UAB_ASSERT_WARNING(false, "Squad should have existed: %s", squadName.str().c_str());
        }
	}

    // for each of our defense squads, if there aren't any enemy units near the position, remove the squad
    std::set<std::string> uselessDefenseSquads;
    for (const auto & kv : _squadData.getSquads())
    {
        const Squad & squad = kv.second;
        const SquadOrder & order = squad.getSquadOrder();

        if (order.getType() != SquadOrderTypes::Defend)
        {
            continue;
        }

        bool enemyUnitInRange = false;
        for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
        {
            if (unit->getPosition().getDistance(order.getPosition()) < order.getRadius())
            {
                enemyUnitInRange = true;
                break;
            }
        }

        if (!enemyUnitInRange)
        {
            _squadData.getSquad(squad.getName()).clear();
        }
    }
}

void CombatCommander::updateDefenseSquadUnits(Squad & defenseSquad, const size_t & flyingDefendersNeeded, const size_t & groundDefendersNeeded)
{
    const BWAPI::Unitset & squadUnits = defenseSquad.getUnits();
    size_t flyingDefendersInSquad = std::count_if(squadUnits.begin(), squadUnits.end(), UnitUtil::CanAttackAir);
    size_t groundDefendersInSquad = std::count_if(squadUnits.begin(), squadUnits.end(), UnitUtil::CanAttackGround);

    // if there's nothing left to defend, clear the squad
    if (flyingDefendersNeeded == 0 && groundDefendersNeeded == 0)
    {
        defenseSquad.clear();
        return;
    }

    // add flying defenders if we still need them
    size_t flyingDefendersAdded = 0;
    while (flyingDefendersNeeded > flyingDefendersInSquad + flyingDefendersAdded)
    {
        BWAPI::Unit defenderToAdd = findClosestDefender(defenseSquad, defenseSquad.getSquadOrder().getPosition(), true);

        // if we find a valid flying defender, add it to the squad
        if (defenderToAdd)
        {
            _squadData.assignUnitToSquad(defenderToAdd, defenseSquad);
            ++flyingDefendersAdded;
        }
        // otherwise we'll never find another one so break out of this loop
        else
        {
            break;
        }
    }

    // add ground defenders if we still need them
    size_t groundDefendersAdded = 0;
    while (groundDefendersNeeded > groundDefendersInSquad + groundDefendersAdded)
    {
        BWAPI::Unit defenderToAdd = findClosestDefender(defenseSquad, defenseSquad.getSquadOrder().getPosition(), false);

        // if we find a valid ground defender add it
        if (defenderToAdd)
        {
            _squadData.assignUnitToSquad(defenderToAdd, defenseSquad);
            ++groundDefendersAdded;
        }
        // otherwise we'll never find another one so break out of this loop
        else
        {
            break;
        }
    }
}

BWAPI::Unit CombatCommander::findClosestDefender(const Squad & defenseSquad, BWAPI::Position pos, bool flyingDefender) 
{
	BWAPI::Unit closestDefender = nullptr;
	double minDistance = std::numeric_limits<double>::max();

    int zerglingsInOurBase = numZerglingsInOurBase();
    bool zerglingRush = zerglingsInOurBase > 0 && BWAPI::Broodwar->getFrameCount() < 5000;

	for (auto & unit : _combatUnits) 
	{
		if ((flyingDefender && !UnitUtil::CanAttackAir(unit)) || (!flyingDefender && !UnitUtil::CanAttackGround(unit)))
        {
            continue;
        }

        if (!_squadData.canAssignUnitToSquad(unit, defenseSquad))
        {
            continue;
        }

        // add workers to the defense squad if we are being rushed very quickly
        if (!Config::Micro::WorkersDefendRush || (unit->getType().isWorker() && !zerglingRush && !beingBuildingRushed()))
        {
            continue;
        }

        double dist = unit->getDistance(pos);
        if (!closestDefender || (dist < minDistance))
        {
            closestDefender = unit;
            minDistance = dist;
        }
	}

	return closestDefender;
}

BWAPI::Position CombatCommander::getDefendLocation()
{
	return BWTA::getRegion(BWTA::getStartLocation(BWAPI::Broodwar->self())->getTilePosition())->getCenter();
}

void CombatCommander::drawSquadInformation(int x, int y)
{
	_squadData.drawSquadInformation(x, y);
}

BWAPI::Position CombatCommander::getMainAttackLocation(Squad* squad/* = nullptr*/)
{
    /*
    BWTA::BaseLocation * enemyBaseLocation = InformationManager::Instance().getMainBaseLocation(BWAPI::Broodwar->enemy());

    // First choice: Attack an enemy region if we can see units inside it
    if (enemyBaseLocation)
    {
        BWAPI::Position enemyBasePosition = enemyBaseLocation->getPosition();

        // get all known enemy units in the area
        BWAPI::Unitset enemyUnitsInArea;
		MapGrid::Instance().GetUnits(enemyUnitsInArea, enemyBasePosition, 800, false, true);

        bool onlyOverlords = true;
        for (auto & unit : enemyUnitsInArea)
        {
            if (unit->getType() != BWAPI::UnitTypes::Zerg_Overlord)
            {
                onlyOverlords = false;
                break;
            }
        }

        if (!BWAPI::Broodwar->isExplored(BWAPI::TilePosition(enemyBasePosition)) || !enemyUnitsInArea.empty())
        {
            if (!onlyOverlords)
            {
                return enemyBaseLocation->getPosition();
            }
        }
    }

    // Second choice: Attack known enemy buildings
    for (const auto & kv : InformationManager::Instance().getUnitInfo(BWAPI::Broodwar->enemy()))
    {
        const UnitInfo & ui = kv.second;

        if (ui.type.isBuilding() && ui.lastPosition != BWAPI::Positions::None)
		{
			return ui.lastPosition;	
		}
    }
*/
    if (squad != nullptr /*&& BWAPI::Broodwar->getFrameCount() % 400 == 0*/) {
        if (true) {
            BWAPI::Broodwar->drawTextScreen(10, 60, "\x08 MCTSCD");
            // Staralgo
            //SquadOrder mainAttackOrder;
            GameState gs(combatSim, regionManager);
            gs.addAllEnemyUnits();
            //gs.addSelfBuildings();
            //for (auto& unit : mainAttackSquad.getUnits()) {
            //    gs.addFriendlyUnit(unit);
            //}
            searchAlg->addSquadToGameState(gs, squad->getUnits());
            playerActions_t bestActions = searchAlg->start(gs/*informationManager->gameState*/);
            for (int i = 0; i < bestActions.size(); ++i) // remove
                BWAPI::Broodwar->drawTextScreen(100, 60 + i * 10, "\x08 bestActions: %d, %d, %d", bestActions[i].action.orderID, bestActions[i].action.targetRegion, bestActions[i].pos);
            BWAPI::Broodwar->drawTextScreen(10, 50, "\x08 bestActions.size(): %d", bestActions.size());
            //std::map<SquadAgent*, BWAPI::Position> bestOrders;
            if (bestActions.size())
                return gs._regman->getCenterRegionId(bestActions.front().action.targetRegion);
            //BWAPI::Position targetPosition;
            //for (const auto& groupAction : bestActions) {
            //    BWAPI::Unitset squadSet = searchAlg->_idToSquad[groupAction.pos];

            //    for (auto& squad : squadSet) {
            //        targetPosition = gs.getCenterRegionId(groupAction.action.targetRegion);
            //        //bestOrders[squad] = targetPosition;
            //        mainAttackOrder = SquadOrder(SquadOrderTypes::Attack, targetPosition, 800, "MCTSCD move");
            //    //}
            //}
        }
        else {
            BWAPI::BroodwarPtr->sendText("Negamax");
            //BWAPI::GameWrapper::sendText("Negamax");
            //BWAPI::Broodwar->drawTextScreen(10, 50, "\x08 Negamax");
            GameState gs(combatSim, regionManager);
            playerActions_t bestActions = negamax(gs, 10); // MC 10
            if (bestActions.size())
                return gs._regman->getCenterRegionId(bestActions.front().action.targetRegion);
        }
    }
    else {
        // Third choice: Attack visible enemy units that aren't overlords
        for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
        {
            if (unit->getType() == BWAPI::UnitTypes::Zerg_Overlord)
            {
                continue;
            }

            if (UnitUtil::IsValidUnit(unit) && unit->isVisible())
            {
                return unit->getPosition();
            }
        }
    }
    // Fourth choice: We can't see anything so explore the map attacking along the way
    return MapGrid::Instance().getLeastExplored();
}

BWAPI::Unit CombatCommander::findClosestWorkerToTarget(BWAPI::Unitset & unitsToAssign, BWAPI::Unit target)
{
    UAB_ASSERT(target != nullptr, "target was null");

    if (!target)
    {
        return nullptr;
    }

    BWAPI::Unit closestMineralWorker = nullptr;
    double closestDist = 100000;
    
    // for each of our workers
	for (auto & unit : unitsToAssign)
	{
        if (!unit->getType().isWorker())
        {
            continue;
        }

		// if it is a move worker
        if (WorkerManager::Instance().isFree(unit)) 
		{
			double dist = unit->getDistance(target);

            if (!closestMineralWorker || dist < closestDist)
            {
                closestMineralWorker = unit;
                dist = closestDist;
            }
		}
	}

    return closestMineralWorker;
}

// when do we want to defend with our workers?
// this function can only be called if we have no fighters to defend with
int CombatCommander::defendWithWorkers()
{
	// our home nexus position
	BWAPI::Position homePosition = BWTA::getStartLocation(BWAPI::Broodwar->self())->getPosition();;

	// enemy units near our workers
	int enemyUnitsNearWorkers = 0;

	// defense radius of nexus
	int defenseRadius = 300;

	// fill the set with the types of units we're concerned about
	for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
	{
		// if it's a zergling or a worker we want to defend
		if (unit->getType() == BWAPI::UnitTypes::Zerg_Zergling)
		{
			if (unit->getDistance(homePosition) < defenseRadius)
			{
				enemyUnitsNearWorkers++;
			}
		}
	}

	// if there are enemy units near our workers, we want to defend
	return enemyUnitsNearWorkers;
}

int CombatCommander::numZerglingsInOurBase()
{
    int concernRadius = 600;
    int zerglings = 0;
    BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
    
    // check to see if the enemy has zerglings as the only attackers in our base
    for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
    {
        if (unit->getType() != BWAPI::UnitTypes::Zerg_Zergling)
        {
            continue;
        }

        if (unit->getDistance(ourBasePosition) < concernRadius)
        {
            zerglings++;
        }
    }

    return zerglings;
}

bool CombatCommander::beingBuildingRushed()
{
    int concernRadius = 1200;
    BWAPI::Position ourBasePosition = BWAPI::Position(BWAPI::Broodwar->self()->getStartLocation());
    
    // check to see if the enemy has zerglings as the only attackers in our base
    for (auto & unit : BWAPI::Broodwar->enemy()->getUnits())
    {
        if (unit->getType().isBuilding())
        {
            return true;
        }
    }

    return false;
}