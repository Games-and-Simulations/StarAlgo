# StarAlgo
*A squad movement planning library for StarCraft AI using Monte Carlo Tree Search and Negamax*

This library was originally created as a part of the 2017 bachelor thesis of Mykyta Viazovskyi at Czech Technical University in Prague. 
Some information about the project can be found in this [short document](https://certicky.github.io/files/publications/staralgo-arxiv-2018.pdf). 

### Usage:

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

### Contributing:

The StarAlgo library can be improved in multiple ways and we invite its users to contribute to the development. 
If you're willing to help, either send email to certicky@gmail.com, so we can add you as a collaborator, or just start creating pull requests.

The most interesting improvements at this time would be:

* Making sure that the library can deal with partial observability well.
* Support for squads with mixed unit types.
* Increasing the spatial granularity by using smaller areas instead of BWTA2 regions. The BWTA2 regions are often too big - instead, we could use smaller hexagons or squares to support run-bys, surrounding, etc.
* Improving the combat outcome estimation.
* Creating the more expressive user's guides or documentation here on Github.   

### Referencing this work:

If you use the library in some kind of academic work, please reference the following paper: M. Viazovskyi, M Certicky. [StarAlgo: A Squad Movement Planning Library for StarCraft using Monte Carlo Tree Search and Negamax](https://certicky.github.io/files/publications/staralgo-arxiv-2018.pdf). arXiv preprint. 2018. 

```
@article{viazovskyi2018,
  title={StarAlgo: A Squad Movement Planning Library for StarCraft using Monte Carlo Tree Search and Negamax},
  author={Viazovskyi, Mykyta and and {\v{C}}ertick{\`y}, Michal},
  journal={arXiv preprint},
  year={2018}
} 
```
