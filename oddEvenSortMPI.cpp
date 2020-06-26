#include <iostream>
#include <mpi.h>

using namespace std;

void inserisco(int*,int);
void riempiRandom(int*,int);
void oddEvenSort(int*,int);
int trovaVicino(int,int,int);
void merge(int*,int*,int*,int,int);

int main(int arg,char *arc[]){
  int* arrayIntero;
  int dimensioneArray, processore, processoriSize;
  double inizioTempo, fineTempo;
  MPI_Status status;

  MPI_Init(&arg,&arc);
  MPI_Comm_size(MPI_COMM_WORLD,&processoriSize);
  MPI_Comm_rank(MPI_COMM_WORLD,&processore);

  if(processore == 0){
    cout << "Inserisci la dimenzione: ";
    cin >> dimensioneArray;
    if(dimensioneArray < 0)
      dimensioneArray = processoriSize;
    if((dimensioneArray % processoriSize) != 0)
      dimensioneArray = ((dimensioneArray/processoriSize)+1)*processoriSize;
    arrayIntero = new int[dimensioneArray];
    //inserisco(arrayIntero, dimensioneArray);
    riempiRandom(arrayIntero, dimensioneArray);
    inizioTempo = MPI_Wtime();
  }

  MPI_Barrier(MPI_COMM_WORLD);
	MPI_Bcast(&dimensioneArray,1,MPI_INT,0,MPI_COMM_WORLD);	//Passa la dimensione a tutti i processori
	MPI_Barrier(MPI_COMM_WORLD);
//setto la dimensione su cui deve andare a lavorare ogni singolo processore
  int sottoSequenzaPerProcessore = dimensioneArray/processoriSize;

  int* array= new int [sottoSequenzaPerProcessore];
  int* temp = new int[sottoSequenzaPerProcessore*2];
  int* arrayVicino = new int[sottoSequenzaPerProcessore];

//Riempio l'array in ordine decrescente
//passiamo arrayIntero dal processo 0 a tutti gli altri
	MPI_Scatter(arrayIntero,sottoSequenzaPerProcessore,MPI_INT,
              array,sottoSequenzaPerProcessore,MPI_INT,
              0,MPI_COMM_WORLD);
	oddEvenSort(array,sottoSequenzaPerProcessore);
  for(int processoInCorso = 0; processoInCorso < processoriSize; processoInCorso++){
      MPI_Barrier(MPI_COMM_WORLD);
      //Il vicino è il primo elemento del prossimo processo, bisogna controllare il caso in cui bisogna scambiare l'ultimo elemento del 'processore corrente' con il primo elemento del 'processore succesivo'
      int vicino = trovaVicino(processoInCorso, processore, processoriSize);
                         MPI_Barrier(MPI_COMM_WORLD);
      if(vicino >= 0 && vicino < processoriSize){
      //Invia i miei valori al mio vicino e ricevi valori dal mio vicino

        MPI_Sendrecv(array, sottoSequenzaPerProcessore, MPI_INT, 	
        			 vicino, processoInCorso, arrayVicino, sottoSequenzaPerProcessore, 	
        			 MPI_INT, vicino, processoInCorso,
                   	 MPI_COMM_WORLD, &status);
                   
      //Se il mio processore è < del rango del mio vicino, mantieni i valori più piccoli
        if(processore < vicino) //Se confronto P2 con P3 Allora terrò i valori più piccoli
            merge(array, arrayVicino, temp, sottoSequenzaPerProcessore, 1);
        else//Se confronto P2 con P0 Allora terrò i valori più grandi
            merge(array, arrayVicino, temp, sottoSequenzaPerProcessore, 0);
      }
  }
	MPI_Barrier(MPI_COMM_WORLD);
  //Crea un array composto da tanti piccoli array da tutti i processori sul processo ZERO
  MPI_Gather(array, sottoSequenzaPerProcessore, MPI_INT,
              arrayIntero, sottoSequenzaPerProcessore, MPI_INT,
              0, MPI_COMM_WORLD);
  if(processore == 0){
    fineTempo = MPI_Wtime();
    //for(int i = 0; i < dimensioneArray; i++)
    //  cout << "Array[" << i << "] -> " << arrayIntero[i] << endl;
    cout << "Tempo impiegato: " << fineTempo-inizioTempo << endl;
    delete[] arrayIntero;
  }
  delete[] array;
  delete[] arrayVicino;
  delete[] temp;

  MPI_Finalize();
  return 0;
}

void inserisco(int* arrayIntero, int dimensioneArray){
	cout << "Inserisci " << dimensioneArray << " numeri." <<endl;
	for(int i=0; i<dimensioneArray; i++)
		cin >> arrayIntero[i];
	cout << "array riempito correttamente." << endl;
}

void riempiRandom(int* arrayIntero, int dimensioneArray){
	for(int i = 0; i < dimensioneArray; i++)
		arrayIntero[i] = rand()%dimensioneArray;
}

void oddEvenSort(int* arrayIntero, int dimensioneArray){
    bool ordinato = false;
    while (!ordinato){ //Se non è ancora ordinato
        ordinato = true;
      for (int i=1; i<dimensioneArray-1; i=i+2) // Una specie di buble sort di elementi dispari
        if (arrayIntero[i] > arrayIntero[i+1]){ // Se è necessario ordinare
          swap(arrayIntero[i], arrayIntero[i+1]); // Ordina
          ordinato = false; //Avendo fatto 'swap' non era ordinato
        }
      for (int i=0; i<dimensioneArray-1; i=i+2) // Pari
        if (arrayIntero[i] > arrayIntero[i+1]){
          swap(arrayIntero[i], arrayIntero[i+1]);
          ordinato = false;
        }
    }
}

int trovaVicino(int processoInCorso, int processo, int processoriSize){
  int vicino;
  if(processoInCorso% 2 != 0) {  //Se siamo nella fase dispari
    if(processo % 2 != 0) {  //Processo dispari
      vicino = processo + 1;
    } else {  //Processo pari
      vicino = processo - 1;
    }
  } else {  //Se siamo nella fase pari
    if(processo % 2 != 0) {  //Processo dispari
      vicino = processo - 1;
    } else {  //Processo pari
      vicino = processo + 1;
    }
  }
  if(vicino < 0 || vicino >= processoriSize) //Casi limite
    vicino = -1;
  return vicino;
}

void merge(int* array, int* arrayVicino, int* temp, int sottoSequenzaPerProcessore, int x){
  int i, j, k;
  // Array già ordinato
  for(i = 0, j = 0, k = 0; i < sottoSequenzaPerProcessore*2; i++){
    if(j < sottoSequenzaPerProcessore && k < sottoSequenzaPerProcessore){
      if(array[j] < arrayVicino[k]){
        temp[i] = array[j]; //Aggiungo a temp l'elemento di array[j] perché è più piccolo
        j++;
      } else {
        temp[i] = arrayVicino[k]; //Aggiungo a temp l'elemento del vicino[k] perché è più piccolo
        k++;
      }
    } else if(j < sottoSequenzaPerProcessore) {
      temp[i] = array[j];
      j++;
    } else {
      temp[i] = arrayVicino[k];
      k++;
    }
  }
  if(x% 2 != 0) //Se devo salvate i valori più piccoli
    for(i = 0; i < sottoSequenzaPerProcessore; i++)
      array[i] = temp[i];
  else //Se devo salvate i valori più grandi
    for(i = sottoSequenzaPerProcessore, j=0; j < sottoSequenzaPerProcessore; i++,j++) //Parto dalla metà
      array[j]= temp[i];
}
