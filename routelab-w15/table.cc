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

#endif

#if defined(DISTANCEVECTOR)


#endif
