#include "table.h"

using namespace std;

#if defined(GENERIC)
ostream & Table::Print(ostream &os) const
{
  // WRITE THIS
  os << "Table()";
  return os;
}
#endif

#if defined(LINKSTATE)
Table::Table(unsigned size){
preds.assign(size+1, size +1);
costs.assign(size+1, 100000000.0);
visited.assign(size+1, false);
}
vector<double> Table::getCosts(){
    return costs;
}

vector<int> Table::getPreds(){
    return preds;
}
vector<int> Table::Visited(){
    return visited;
}

void Table::setCosts(vector<double> updatedCosts){
    costs = updatedCosts;
}

void Table::setPreds(vector<int> updatedPreds){
    preds = updatedPreds;
}
void Table::setVisited(vector<bool> updatedPreds){
    preds = updatedPredsVisited;
}

ostream & Table::Print(ostream &os) const
{
  for(int i = 1;i<preds.size(); i++)
      os << "Node: "<< i << "Predecessor: " << preds[i] << "Cost: " << costs[i] << endl;
  return os;
}
#endif

#if defined(DISTANCEVECTOR)


#endif