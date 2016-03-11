#include "node.h"
#include "context.h"
#include "error.h"

#include <new>
#include <string>
#include <vector>
#include <iostream>
#include <deque>

Node::Node(const unsigned n, SimulationContext *c, double b, double l) : 
    number(n), context(c), bw(b), lat(l) 
{}

Node::Node() 
{ throw GeneralException(); }

Node::Node(const Node &rhs) : 
  number(rhs.number), context(rhs.context), bw(rhs.bw), lat(rhs.lat) {}

Node & Node::operator=(const Node &rhs) 
{
  return *(new(this)Node(rhs));
}

void Node::SetNumber(const unsigned n) 
{ number=n;}

unsigned Node::GetNumber() const 
{ return number;}

void Node::SetLatency(const double l)
{ lat=l;}

double Node::GetLatency() const 
{ return lat;}

void Node::SetBW(const double b)
{ bw=b;}

double Node::GetBW() const 
{ return bw;}

Node::~Node()
{}

// Implement these functions  to post an event to the event queue in the event simulator
// so that the corresponding node can recieve the ROUTING_MESSAGE_ARRIVAL event at the proper time
void Node::SendToNeighbors(const RoutingMessage *m)
{
}

void Node::SendToNeighbor(const Node *n, const RoutingMessage *m)
{

}

deque<Node*> *Node::GetNeighbors()
{
  return context->GetNeighbors(this);
}

void Node::SetTimeOut(const double timefromnow)
{
  context->TimeOut(this,timefromnow);
}


bool Node::Matches(const Node &rhs) const
{
  return number==rhs.number;
}


#if defined(GENERIC)
void Node::LinkHasBeenUpdated(const Link *l)
{
  cerr << *this << " got a link update: "<<*l<<endl;
  //Do Something generic:
  SendToNeighbors(new RoutingMessage);
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *m)
{
  cerr << *this << " got a routing messagee: "<<*m<<" Ignored "<<endl;
}


void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored"<<endl;
}

Node *Node::GetNextHop(const Node *destination) const
{
  return 0;
}

Table *Node::GetRoutingTable() const
{
  return new Table;
}


ostream & Node::Print(ostream &os) const
{
  os << "Node(number="<<number<<", lat="<<lat<<", bw="<<bw<<")";
  return os;
}

#endif

#if defined(LINKSTATE)


void Node::LinkHasBeenUpdated(const Link *l)
{
  cerr << *this<<": Link Update: "<<*l<<endl;
  unsigned dest = l->GetDest();
  unsigned src = l->GetSrc();
  double cost = l->GetLatency();
  bool updated = false;

  for(deque<Link*>::iterator link=out_links.begin();link!=out_links.end();link++) {
    if(src==(*link)->GetSrc()) {
      if(dest==(*link)->GetDest()) {
        //then we update the existing link
        (*link)->SetLatency(cost);
        updated = true;
        break;
      } 
    }
  }

  //if new link *l does not exist in all_links
  if(updated==false) 
  {
    out_links.push_back(l); //node's outgoing links
  }
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *m)
{
  cerr << *this << " Routing Message: "<<*m;
}

void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored"<<endl;
}

// void Node::Djistras(Node* dest) {
  
//   unsigned srcIndex = GetNumber();
//   //number of all nodes. achieved through the link_update funtion that updates the node list
//   //int number_of_nodes = GetAllNodes(); 
 
//   // Creating an instance of class table to be able to get the predecessors and costs vectors
//   // Table* nodetable = new Table(10000); 
//   // vector<unsigned> preds_vec = nodetable->getPreds();
//   // vector<double> costs_vec = nodetable->getCosts();
//   // vector<bool> visited_vec = nodetable->getVisited();
  
//   vector<unsigned> preds_vec = mytable->getPreds();
//   vector<double> costs_vec = mytable->getCosts();
//   vector<bool> visited_vec = mytable->getVisited();
//   deque<Link*> outlinks = mytable->getOutLinks(); //outgoing links for node

//   unsigned neighborIndex;
//   double linkCost;
//   double newCost;
//   deque<Node*> myneighbors;

//   Node* fixedNode;

//   // mytable.setCosts(costs_vec);
//   // mytable.setPreds(preds_vec);
//   // mytable.setVisited(visited_vec);
//   // preds_vec = mytable.getPreds();
//   // costs_vec = mytable.getCosts();
//   // visited_vec = mytable.getVisited();
  
//   // initializing the values for the root to be 0 
//   preds_vec[srcIndex] = srcIndex;
//   costs_vec[srcIndex] = 0.0;
//   visited_vec[srcIndex] = true;
//   fixedNode = this;
//   // for(int i=0; i<number_of_nodes;i++)

//   //define variables
//   unsigned min_cost;
//   unsigned min_index;
  
//   //while the shortest path to dest is not found
//   while (fixedNode != dest){ //while the shortest path to dest is not found
//     all_nodes.push_back(fixedNode);
//     outlinks = fixedNode -> GetRoutingTable() -> getOutLinks();  //assuming that each node has a funciton called getOutLink that returns all outgoing links
//     min_cost = 100000000.0;
//     min_index = 0;

//     //update costs for unfixed neighbors
//     for(deque<Link*>::iterator currLink = outlinks->begin(); currLink!= outlinks->end(); ++currLink){
//       neighborIndex = (*currLink) -> GetDest();
      
//       if(visited_vec[neighborIndex] == false){  //only update the cost if the node is not fixed
//           linkCost = (*currLink) -> GetLatency();
//           newCost = costs_vec[srcIndex] + linkCost;

//            // replace if the new cost is less than the currentcost 
//           if(newCost < costs_vec[neighborIndex]){
//               preds_vec[neighborIndex] = srcIndex;
//               costs_vec[neighborIndex] = newCost; 
//               if(newCost < min_cost) { //keep track of minimum cost neighbor
//                 min_cost = newCost;
//                 min_index = neighborIndex;
//               }
//           }
//       }
//     }

//     //Find the min value in this deque to get the next node to be set
//     // unsigned  min_cost = 1000000000;
//     // unsigned  min_index = 0;

//     // for (unsigned i = 0; i < costs_vec.size(); i++) { //loop to find the next node to be fixed
//     //   if (visited_vec[i] == false) { //skip the ones that were previously fixed
//     //     if (costs_vec[i] < min_cost) {
//     //       min_cost = costs_vec[i];
//     //       min_index = i;
//     //     }
//     //   }
//     // }

//     //find the new fixedNode in the list
//     myneighbors = *(context->GetNeighbors(fixedNode));

//     for (deque<Node*>::iterator j = myneighbors->begin(); j != myneighbors->end(); ++j){
//       //find the node to be fixed next
//       neighborIndex = (*j)->GetNumber();

//       if(neighborIndex == min_index) { //the min node is found
//         visited_vec[neighborIndex] = true; //set visited value to be true    
//         fixedNode = *j;
//         break;
//       }
//     }
//   }
// }

Node *Node::GetNextHop(const Node *destination) const //do we neccessarily need the CONST??????
{
  //Djistras
  unsigned srcIndex = GetNumber();
  //number of all nodes. achieved through the link_update funtion that updates the node list
  //int number_of_nodes = GetAllNodes(); 
 
  // Creating an instance of class table to be able to get the predecessors and costs vectors
  // Table* nodetable = new Table(10000); 
  // vector<unsigned> preds_vec = nodetable->getPreds();
  // vector<double> costs_vec = nodetable->getCosts();
  // vector<bool> visited_vec = nodetable->getVisited();
  
  vector<unsigned> preds_vec = mytable->getPreds();
  vector<double> costs_vec = mytable->getCosts();
  vector<bool> visited_vec = mytable->getVisited();
  deque<Link*> outlinks = mytable->getOutLinks(); //outgoing links for node

  unsigned neighborIndex;
  double linkCost;
  double newCost;
  deque<Node*> myneighbors;

  Node* fixedNode;

  // mytable.setCosts(costs_vec);
  // mytable.setPreds(preds_vec);
  // mytable.setVisited(visited_vec);
  // preds_vec = mytable.getPreds();
  // costs_vec = mytable.getCosts();
  // visited_vec = mytable.getVisited();
  
  // initializing the values for the root to be 0 
  preds_vec[srcIndex] = srcIndex;
  costs_vec[srcIndex] = 0.0;
  visited_vec[srcIndex] = true;
  fixedNode = this;
  // for(int i=0; i<number_of_nodes;i++)

  //define variables
  unsigned min_cost;
  unsigned min_index;
  
  //while the shortest path to dest is not found
  while (fixedNode != destination){ //while the shortest path to dest is not found
    all_nodes.push_back(fixedNode);
    outlinks = fixedNode -> GetRoutingTable() -> getOutLinks();  //assuming that each node has a funciton called getOutLink that returns all outgoing links
    min_cost = 100000000.0;
    min_index = 0;

    //update costs for unfixed neighbors
    for(deque<Link*>::iterator currLink = outlinks->begin(); currLink!= outlinks->end(); ++currLink){
      neighborIndex = (*currLink) -> GetDest();
      
      if(visited_vec[neighborIndex] == false){  //only update the cost if the node is not fixed
          linkCost = (*currLink) -> GetLatency();
          newCost = costs_vec[srcIndex] + linkCost;

           // replace if the new cost is less than the currentcost 
          if(newCost < costs_vec[neighborIndex]){
              preds_vec[neighborIndex] = srcIndex;
              costs_vec[neighborIndex] = newCost; 
              if(newCost < min_cost) { //keep track of minimum cost neighbor
                min_cost = newCost;
                min_index = neighborIndex;
              }
          }
      }
    }

    //Find the min value in this deque to get the next node to be set
    // unsigned  min_cost = 1000000000;
    // unsigned  min_index = 0;

    // for (unsigned i = 0; i < costs_vec.size(); i++) { //loop to find the next node to be fixed
    //   if (visited_vec[i] == false) { //skip the ones that were previously fixed
    //     if (costs_vec[i] < min_cost) {
    //       min_cost = costs_vec[i];
    //       min_index = i;
    //     }
    //   }
    // }

    //find the new fixedNode in the list
    myneighbors = *(context->GetNeighbors(fixedNode));

    for (deque<Node*>::iterator j = myneighbors->begin(); j != myneighbors->end(); ++j){
      //find the node to be fixed next
      neighborIndex = (*j)->GetNumber();

      if(neighborIndex == min_index) { //the min node is found
        visited_vec[neighborIndex] = true; //set visited value to be true    
        fixedNode = *j;
        break;
      }
    }
  }

   // Using the initally initialised table to find the next hop
  unsigned curr = destination -> GetNumber();
  vector<unsigned> pVec = mytable.getPreds();
  unsigned prev = pVec[curr];
  Node* nextone;


  while(prev != GetNumber()){
    curr = prev;
    prev = pVec[curr];    //change pred 
  }

  //find the node (nextHop) with index same as curr 
  for(deque<Node*>::const_iterator point = all_nodes.begin(); point!=all_nodes.end();++point){
      if(curr == (*point)->GetNumber()){
        nextone = *point;
      }
  }
  return nextone;
}


Table *Node::GetRoutingTable() const
{
  // WRITE
  return &mytable;
}


ostream & Node::Print(ostream &os) const
{
  os << "Node(number="<<number<<", lat="<<lat<<", bw="<<bw<<")";
  return os;
}
#endif


#if defined(DISTANCEVECTOR)

void Node::LinkHasBeenUpdated(const Link *l)
{
  // update our table
  // send out routing mesages
  cerr << *this<<": Link Update: "<<*l<<endl;
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *m)
{

}

void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored"<<endl;
}


Node *Node::GetNextHop(const Node *destination) const
{
}

Table *Node::GetRoutingTable() const
{
}


ostream & Node::Print(ostream &os) const
{
  os << "Node(number="<<number<<", lat="<<lat<<", bw="<<bw;
  return os;
}
#endif
