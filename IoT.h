#ifndef IOT_H
#define IOT_H

void discover();
void setupIoT(int);
boolean handleButton();
void readUDP();
char* ePromGet (int , int );
int ePromPut (int , char* , int );

class ValueTable {
  public:
  typedef struct NamedValue {
    char *name;
    char *value;
    boolean changed;
    int size;
    int eProm;
  };

  NamedValue* valueTable[10];

  int numProperties = 0;

  ValueTable() {}

int findProperty (char* );
int findIP (char* );
int ePromWrite (char* , int , int );
char* ePromRead();
void nuEpromField (char* , int );
int nuEpromProperty (char* , char* , int );
int declareProperty (char* , int  =-1, int  = -1);;
void loadEprom ();
void updateProperty (char* , String , int =-1);
void updateProperty (char* , char* , int =-1);
int initializePersistent (char* , char* , int  =-1);
int declarePersistent (char* , int  =-1);
char* getValue (char* );
char* getValue (int );
char* getFromIP (char* ip);
boolean changed (char* );
};

class Properties : public ValueTable {
  public:
  void updateProperty (char* , char* , int  =-1);
  void doAction (char* , char* );
};

extern ValueTable scripts, rules, ipTable;
extern Properties properties;
extern int LED;


#endif //IOT_H

