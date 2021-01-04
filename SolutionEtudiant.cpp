#include <iostream>
#include <ctime> // time_t time()
#include <unistd.h> // entre autres : usleep
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string>
#include <stdlib.h>

#include "SolutionEtudiant.hpp"
#include "Header_Prof.h"

int* custom_IDs;
pthread_mutex_t mutexInit;

pthread_t createThread(int *customID){
	pthread_t tid;

	pthread_create(&tid, 0, vieDuPhilosophe, customID);

	return tid;
}

#ifdef SOLUTION_1


pthread_mutex_t* mutexEat;
int excluded;
pthread_t sched;

void initialisation()
{
	cleanSem();
	pthread_mutex_init(&mutexInit, NULL); //Mutex Qui bloque tout les autres thread tant que l'initialisation() est pas finit
	pthread_mutex_init(&mutexCout, NULL);
	pthread_mutex_init(&mutexEtats, NULL);

	pthread_mutex_lock(&mutexInit);

	semFourchettes = (sem_t**) malloc(NB_PHILOSOPHES*sizeof(sem_t*));
	threadsPhilosophes = (pthread_t*) malloc(NB_PHILOSOPHES*sizeof(pthread_t));
	etatsPhilosophes = (char*) malloc(NB_PHILOSOPHES*sizeof(char));
	mutexEat = (pthread_mutex_t*) malloc(NB_PHILOSOPHES*sizeof(pthread_mutex_t));
	custom_IDs = (int*) malloc(NB_PHILOSOPHES*sizeof(int));

	for(int i=0; i<NB_PHILOSOPHES; i++){
		pthread_mutex_init(&mutexEat[i], NULL);
		custom_IDs[i] = 4000+i;
		std::string strName = "/Fourch_" + std::to_string(i);
		semFourchettes[i] = sem_open(strName.c_str(), O_CREAT | O_EXCL,0600,1);
	}

	pthread_create(&sched, 0, scheduler, NULL);

	usleep(1000); //Pour laisser le temps au sched de s'init

	for(int i=0; i<NB_PHILOSOPHES; i++){
		etatsPhilosophes[i] = P_PENSE;
		threadsPhilosophes[i] = createThread(&custom_IDs[i]);
	}

	instantDebut = time(0);

	for(int i=0; i<NB_PHILOSOPHES; i++){
		etatsPhilosophes[i] = P_FAIM;
	}

	pthread_mutex_unlock(&mutexInit);
}

void* vieDuPhilosophe(void* idPtr)
{
	/*FILE *fp;
	double penseTime = 0;
	double mangeTime = 0;
	double faimTime = 0;
	int totalCycle = 0;
	struct timespec tstart={0,0}, tend={0,0};
	clock_gettime(CLOCK_MONOTONIC, &tstart); //Pour les données moyenne etc*/

	srand(time(NULL));
	int id = * ((int*)idPtr);

	int indice_Id = id - 4000;

	int fork_l = indice_Id;
	int fork_r = fork_l+1;
	if(fork_r==NB_PHILOSOPHES)
		fork_r = 0;

	int randInt = 0;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    pthread_mutex_lock(&mutexInit);
    pthread_mutex_unlock(&mutexInit); //On lock et unlock pour synchro les thread

    while(1) {
    	pthread_mutex_lock(&mutexEat[indice_Id]); //On lock pour manger,
    	//normalement le sched aura lock ce mutex pour empecher le philo de manger

		/*clock_gettime(CLOCK_MONOTONIC, &tend);//Fini d'avoir faim
		faimTime = (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));*/

    	sem_wait(semFourchettes[fork_l]);
		sem_wait(semFourchettes[fork_r]);

		actualiserEtAfficherEtatsPhilosophes(indice_Id, P_MANGE);

		randInt = rand() % (int)(DUREE_MANGE_MAX_S * 1000000);
		//mangeTime = (double)randInt/1000000;
		usleep(randInt);

		sem_post(semFourchettes[fork_l]);
		sem_post(semFourchettes[fork_r]);

		actualiserEtAfficherEtatsPhilosophes(indice_Id, P_PENSE);

		pthread_mutex_unlock(&mutexEat[indice_Id]); //On a fini de manger donc on peut unlock

		randInt = rand() % (int)(DUREE_PENSE_MAX_S*1000000);
		//penseTime = (double)randInt/1000000;
		usleep(randInt);

		actualiserEtAfficherEtatsPhilosophes(indice_Id, P_FAIM);

		/*pthread_mutex_lock(&mutexCout);
		std::cout << "SOl2, Philo[" << id << "] MAnge, Pense, FAim, Cycle : " << mangeTime << " "
																				<< penseTime << " "
																				<< faimTime << " "
																				<< totalCycle << std::endl;
		pthread_mutex_unlock(&mutexCout);
		fp = fopen("./dataSol.csv","a");
		fprintf(fp, "SOL2; %d; %f; %f; %f; %d\n", id, mangeTime, penseTime, faimTime, totalCycle);
		fclose(fp);
		struct timespec tstart={0,0}, tend={0,0}; //Remet Timer Faim à 0
		clock_gettime(CLOCK_MONOTONIC, &tstart);

		totalCycle++;*/

        pthread_testcancel(); // point où l'annulation du thread est permise
    }
    
    return NULL;
}

void actualiserEtAfficherEtatsPhilosophes(int idPhilosopheChangeant, char nouvelEtat)
{
	pthread_mutex_lock(&mutexEtats);
    etatsPhilosophes[idPhilosopheChangeant] = nouvelEtat;
    pthread_mutex_unlock(&mutexEtats);

    pthread_mutex_lock(&mutexCout);

	for (int i=0;i<NB_PHILOSOPHES;i++) {
		if (i==idPhilosopheChangeant)
			std::cout << "*";
		else
			std::cout << " ";
		std::cout << etatsPhilosophes[i];
		if (i==idPhilosopheChangeant)
			std::cout << "* ";
		else
			std::cout << "  ";
	}
	std::cout << "          (t=" << difftime(time(NULL), instantDebut) << ")" << std::endl;

	pthread_mutex_unlock(&mutexCout);
}

void terminerProgramme()
{
	for(int i=0; i<NB_PHILOSOPHES; i++){
		std::string strName = "/Fourch_" + std::to_string(i);
		sem_unlink(strName.c_str());
		sem_close(semFourchettes[i]);
		pthread_cancel(threadsPhilosophes[i]);
		pthread_mutex_destroy(&mutexEat[i]);
	}

	pthread_cancel(sched);

	pthread_mutex_destroy(&mutexInit);
	pthread_mutex_destroy(&mutexCout);
	pthread_mutex_destroy(&mutexEtats);

    free(semFourchettes);
    free(threadsPhilosophes);
    free(etatsPhilosophes);
    free(mutexEat);
}

void* scheduler(void* idPtr)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

	for(int i=0; i<NB_PHILOSOPHES; i++){
		pthread_mutex_lock(&mutexEat[i]); //ON lock tout les mutex des philo pour etre sur que aucun puisse manger au début
	}

	pthread_mutex_lock(&mutexInit);
	pthread_mutex_unlock(&mutexInit); //Pour synchro avec l'init

	srand(time(NULL));
	while(1)
	{
		if(NB_PHILOSOPHES%2!=0){ //Si NB philosophe = impair
			excluded = rand() % 2;			//Choisis aléatoirement le 1er ou dernier philo
			excluded *= (NB_PHILOSOPHES-1); //qui ne mangera pas
		}else{
			excluded = NB_PHILOSOPHES;
		}

		for(int i=0; i<NB_PHILOSOPHES; i+=2){
			if(i!=excluded){
				while(etatsPhilosophes[i]!='F'); //Attends qu'ils aient faim
			}
		}
		for(int i=0; i<NB_PHILOSOPHES; i+=2){
			if(i!=excluded){
				pthread_mutex_unlock(&mutexEat[i]); //Les laisse manger
			}
		}
		for(int i=0; i<NB_PHILOSOPHES; i+=2){
			if(i!=excluded){
				while(etatsPhilosophes[i]!='M'); //Attends qu'ils mangent
			}
		}
		for(int i=0; i<NB_PHILOSOPHES; i+=2){
			if(i!=excluded){
				pthread_mutex_lock(&mutexEat[i]); //Lock pour les empecher de manger
			}
		}
		for(int i=0; i<NB_PHILOSOPHES; i+=2){
			if(i!=excluded){
				while(etatsPhilosophes[i]=='M'); //Attends qu'ils aient finit de manger
			}
		}

		for(int i=1; i<NB_PHILOSOPHES; i+=2){ //Pareil que précédemment mais pour le 2eme groupe (les impaires)
			while(etatsPhilosophes[i]!='F');
		}
		for(int i=1; i<NB_PHILOSOPHES; i+=2){
			pthread_mutex_unlock(&mutexEat[i]);
		}
		for(int i=1; i<NB_PHILOSOPHES; i+=2){
			while(etatsPhilosophes[i]!='M');
		}
		for(int i=1; i<NB_PHILOSOPHES; i+=2){
			pthread_mutex_lock(&mutexEat[i]);
		}
		for(int i=1; i<NB_PHILOSOPHES; i+=2){
			while(etatsPhilosophes[i]=='M');
		}

	    pthread_testcancel();
	}
}

void cleanSem()
{
	for(int i=0; i<NB_PHILOSOPHES; i++){
		std::string strName = "/Fourch_" + std::to_string(i);
		sem_unlink(strName.c_str());
	}
}

#endif

#ifdef SOLUTION_2
bool GroupAisEating = true;
bool GroupBisEating = false;
bool allAhungry= true;
bool allBhungry = true;
bool excludedAlreadyDone = false;
bool* p_alreadyPense;
bool* p_alreadyFaim;

pthread_mutex_t mutexTab;

int excluded=NB_PHILOSOPHES;
int penseCounter = 0;
int faimCounterA = 0;
int faimCounterB = 0;

void initialisation()
{
	cleanSem();
	pthread_mutex_init(&mutexInit, NULL);
	pthread_mutex_init(&mutexCout, NULL);
	pthread_mutex_init(&mutexEtats, NULL);
	pthread_mutex_init(&mutexTab, NULL);

	pthread_mutex_lock(&mutexInit);

	semFourchettes = (sem_t**) malloc(NB_PHILOSOPHES*sizeof(sem_t*));
	threadsPhilosophes = (pthread_t*) malloc(NB_PHILOSOPHES*sizeof(pthread_t));
	etatsPhilosophes = (char*) malloc(NB_PHILOSOPHES*sizeof(char));
	custom_IDs = (int*) malloc(NB_PHILOSOPHES*sizeof(int));

	p_alreadyPense = (bool*) malloc(NB_PHILOSOPHES*sizeof(bool));
	p_alreadyFaim = (bool*) malloc(NB_PHILOSOPHES*sizeof(bool));

	for(int i=0; i<NB_PHILOSOPHES; i++){
		custom_IDs[i] = 4000+i;
		std::string strName = "/Fourch_" + std::to_string(i);
		semFourchettes[i] = sem_open(strName.c_str(), O_CREAT | O_EXCL,0600,1);
		p_alreadyPense[i] = false;
	}

	if((NB_PHILOSOPHES%2!=0)){ //Si NB philosophe = impair
		excluded = rand() % 2;			//Choisis aléatoirement le 1er ou dernier philo
		excluded *= (NB_PHILOSOPHES-1); //qui ne mangera pas
		//std::cout << "excluded : " << excluded << " " << id << std::endl;
	}

	for(int i=0; i<NB_PHILOSOPHES; i++){
		etatsPhilosophes[i] = P_PENSE;
		threadsPhilosophes[i] = createThread(&custom_IDs[i]);
	}

	instantDebut = time(0);

	for(int i=0; i<NB_PHILOSOPHES; i++){
		etatsPhilosophes[i] = P_FAIM;
	}
	srand(time(NULL));
	pthread_mutex_unlock(&mutexInit);
}

void* vieDuPhilosophe(void* idPtr)
{
	/*FILE *fp;
	double penseTime = 0;
	double mangeTime = 0;
	double faimTime = 0;
	int totalCycle = 0;
	struct timespec tstart={0,0}, tend={0,0};
	clock_gettime(CLOCK_MONOTONIC, &tstart); *///Start Time Pour faim (1st time)

	//Pour les données à enregistrer

	srand(time(NULL));
	int id = * ((int*)idPtr);

	int indice_Id = id - 4000;

	int fork_l = indice_Id;
	int fork_r = fork_l+1;
	if(fork_r==NB_PHILOSOPHES)
		fork_r = 0;


	int randInt = 0;

	pthread_mutex_lock(&mutexInit);
	pthread_mutex_unlock(&mutexInit);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    while(1) {

		actualiserEtAfficherEtatsPhilosophes(indice_Id, P_MANGE);
		/*clock_gettime(CLOCK_MONOTONIC, &tend);//Fini d'avoir faim
		faimTime = (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));*/
		/*
		 * Contrairement à la solution 1, ici on prends les fourchettes après avoir demander de changer d'état.
		 * C'est parce que l'ordonnanceur se fait dans actualiserEtAfficherEtatsPhilosophes. Donc on risque
		 * une situation d'interbloquage : Une tache prends une fourchette puis est bloquée par l'ordonnanceur,
		 * mais en même temps une autre tache attends la fourchette => Tout est bloqué.
		 * DE plus les mettre juste avant l'ordonnanceur reviendrait à avoir le probleme qu'on a sans ordonnanceur,
		 * à savoir une situation d'interbloquage quand tout les threads prennent leurs sémaphore de gauche en même
		 * temps.
		 */
		sem_wait(semFourchettes[fork_l]);
		sem_wait(semFourchettes[fork_r]);

		randInt = rand() % (int)(DUREE_MANGE_MAX_S * 1000000);
		//mangeTime = (double)randInt/1000000;
		usleep(randInt);

		sem_post(semFourchettes[fork_l]);
		sem_post(semFourchettes[fork_r]);

		actualiserEtAfficherEtatsPhilosophes(indice_Id, P_PENSE);

		randInt = rand() % (int)(DUREE_PENSE_MAX_S*1000000);
		//penseTime = (double)randInt/1000000;
		usleep(randInt);

		actualiserEtAfficherEtatsPhilosophes(indice_Id, P_FAIM);

		/*pthread_mutex_lock(&mutexCout);
		std::cout << "SOl2, Philo[" << id << "] MAnge, Pense, FAim, Cycle : " << mangeTime << " "
																				<< penseTime << " "
																				<< faimTime << " "
																				<< totalCycle << std::endl;
		pthread_mutex_unlock(&mutexCout);
		fp = fopen("./dataSol.csv","a");
		fprintf(fp, "SOL2; %d; %f; %f; %f; %d\n", id, mangeTime, penseTime, faimTime, totalCycle);
		fclose(fp);
		struct timespec tstart={0,0}, tend={0,0}; //Remet Timer Faim à 0
		clock_gettime(CLOCK_MONOTONIC, &tstart);

		totalCycle++;*/

		pthread_testcancel(); // point où l'annulation du thread est permise
    }

    return NULL;
}

void actualiserEtAfficherEtatsPhilosophes(int idPhilosopheChangeant, char nouvelEtat)
{
	changingState(idPhilosopheChangeant, nouvelEtat); //Le "scheduler"

	pthread_mutex_lock(&mutexCout);
	pthread_mutex_lock(&mutexEtats);
    etatsPhilosophes[idPhilosopheChangeant] = nouvelEtat;
    pthread_mutex_unlock(&mutexEtats);


	for (int i=0;i<NB_PHILOSOPHES;i++) {
		if (i==idPhilosopheChangeant)
			std::cout << "*";
		else
			std::cout << " ";
		std::cout << etatsPhilosophes[i];
		if (i==idPhilosopheChangeant)
			std::cout << "* ";
		else
			std::cout << "  ";
	}
	std::cout << "          (t=" << difftime(time(NULL), instantDebut) << ")" << std::endl;

	pthread_mutex_unlock(&mutexCout);
}

void terminerProgramme()
{
	for(int i=0; i<NB_PHILOSOPHES; i++){
		std::string strName = "/Fourch_" + std::to_string(i);
		sem_unlink(strName.c_str());
		sem_close(semFourchettes[i]);
		pthread_cancel(threadsPhilosophes[i]);
		//std::cout << i << std::endl;
	}

	pthread_mutex_destroy(&mutexInit);
	pthread_mutex_destroy(&mutexCout);
	pthread_mutex_destroy(&mutexEtats);
	pthread_mutex_destroy(&mutexTab);

    free(semFourchettes);
    free(threadsPhilosophes);
    free(etatsPhilosophes);
    //free(p_askEat);
    free(p_alreadyFaim);
    free(p_alreadyPense);
}

void changingState(int id, char changingToState)
{
	if(changingToState == P_MANGE){

		if(id%2 == 0){//Si impair
			while(GroupBisEating || (faimCounterA != 0) || id == excluded);
		}else{
			while(GroupAisEating || (faimCounterB != 0));
		}
	}
	else if(changingToState==P_PENSE){
		if(!p_alreadyPense[id]){
			penseCounter++;
			pthread_mutex_lock(&mutexTab);
			p_alreadyPense[id] = true;
			pthread_mutex_unlock(&mutexTab);
		}

		if(penseCounter == NB_PHILOSOPHES/2){ //Si le groupe pense (Donc a fini de manger)
			for(int i = id%2; i<NB_PHILOSOPHES; i+=2){
				pthread_mutex_lock(&mutexTab);
				p_alreadyPense[i] = false;
				pthread_mutex_unlock(&mutexTab);
			}
			penseCounter = 0;
			switchGroup(); //O change le groupe
		}
	}
	else if(changingToState==P_FAIM){
		if(!p_alreadyFaim[id]){
			if(id%2 == 0 && id != excluded){
				faimCounterA++;
			}
			else if(id%2 != 0){
				faimCounterB++;
			}
			pthread_mutex_lock(&mutexTab);
			p_alreadyFaim[id] = true;
			pthread_mutex_unlock(&mutexTab);
		}

		if(id%2==0){
			if(faimCounterA == NB_PHILOSOPHES/2){
				for(int i = 0; i<NB_PHILOSOPHES; i+=2){
					pthread_mutex_lock(&mutexTab);
					p_alreadyFaim[i] = false;
					pthread_mutex_unlock(&mutexTab);
				}
				if((NB_PHILOSOPHES%2!=0) && (id%2 == 0)){ //Si NB philosophe = impair
					excluded = rand() % 2;			//Choisis aléatoirement le 1er ou dernier philo
					excluded *= (NB_PHILOSOPHES-1); //qui ne mangera pas
				}

				faimCounterA = 0;
			}
		}else{
			if(faimCounterB == NB_PHILOSOPHES/2){
				for(int i = 1; i<NB_PHILOSOPHES; i+=2){
					pthread_mutex_lock(&mutexTab);
					p_alreadyFaim[i] = false;
					pthread_mutex_unlock(&mutexTab);
				}
				faimCounterB = 0;
			}
		}
	}
}

void switchGroup()
{
	GroupAisEating = !GroupAisEating;
	GroupBisEating = !GroupBisEating;
}

void cleanSem()
{
	for(int i=0; i<NB_PHILOSOPHES; i++){
		std::string strName = "/Fourch_" + std::to_string(i);
		sem_unlink(strName.c_str());
	}
}

#endif


