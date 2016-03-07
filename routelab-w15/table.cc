#include "table.h"

#if defined(GENERIC)
ostream & Table::Print(ostream &os) const
{
  // WRITE THIS
  os << "Table()";
  return os;
}
#endif

#if defined(LINKSTATE)
Table::Table(){
preds.assign(number_of_nodes+1, number_of_nodes +1);
costs.assign(number_of_nodes+1, 100000000);
}
vector<int> Table::getCosts(){
    return costs;
}

vector<int> Table::getPreds(){
    return preds;
}

void Table::setCosts(vector<int> updatedCosts){
    costs = updatedCosts;
}

void Table::setPreds(vector<int> updatedPreds){
    preds = updatedPreds;
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