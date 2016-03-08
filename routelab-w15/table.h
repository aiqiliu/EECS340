#ifndef _table
#define _table


#include <iostream>

using namespace std;

#if defined(GENERIC)
class Table {
  // Students should write this class

 public:
  ostream & Print(ostream &os) const;
};
#endif


#if defined(LINKSTATE)
// Djistra's Algorithm
class Table {
  // Students should write this class
 vector<int> preds; 
 vector<int> costs; 
 public:
 vector<int> getCosts();
 vector<int> getPreds();
 void setCosts(vector<int> updatedCosts);
 void setPreds(vector<int> updatedPreds);
 Table(unsigned size);
  ostream & Print(ostream &os) const;
};
#endif

#if defined(DISTANCEVECTOR)

#include <deque>

class Table {
 public:
  ostream & Print(ostream &os) const;
};
#endif

inline ostream & operator<<(ostream &os, const Table &t) { return t.Print(os);}

#endif