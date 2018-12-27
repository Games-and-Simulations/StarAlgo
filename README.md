# StarAlgo
A squad movement planning library for StarCraft AI using Monte Carlo Tree Search and Negamax

General overview of how to setup the library:

1. Add library file to project.
2. Include the headers.
3. Create several squads for your units (the more the better).
4. Create the search object.
5. Distribute units among the squads.
6. Call the search on the search object (#4) and get the result.
7. Assign the result back to the squads.

The algorithm assigns units to distinct groups by their type and region, so if your squads share the same approach, the result will be more accurate for the squad (otherwise the prevailing group will be used).

### Example:

* Take a look into folder `examples/UAlbertaBot with StarAlgo` to see how the UAlbertaBot can be modified to use StarAlgo library. The important file is `UAlbertaBot/Source/CombatManager.cpp`.  
* You can also use the files in the `patches` folder to integrate with UAlbertaBot.
