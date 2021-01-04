

#ifndef SolutionEtudiant_hpp
#define SolutionEtudiant_hpp


#include <iostream>

// Dans ce fichier, rajouter tout ce qui pourrait manquer pour programmer la solution
void initialisation();
void terminerProgramme();
void* vieDuPhilosophe(void* idPtr);
void* scheduler(void* idPtr);
void changingState(int id, char state);

void cleanSem();
void switchGroup();

#endif /* SolutionEtudiant_hpp */
