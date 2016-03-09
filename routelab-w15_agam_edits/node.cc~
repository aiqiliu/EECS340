#include "node.h"
#include "context.h"
#include "error.h"
#include "table.h"
#include <deque>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

using namespace std;

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
  context->SendToNeighbors(this, m);
}

void Node::SendToNeighbor(const Node *n, const RoutingMessage *m)
{
  context->SendToNeighbor(this, n, m);
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
{//consult the table.get the 
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

// function to get all nodes
void Node::GetAllNodes()
{
  cerr << "Getting all nodes in the network" << endl;
  number_of_nodes = 0; //resets num of nodes
  list_of_node_nums.clear(); //resets list of node numbers
  list_of_nodes.clear(); //resets list of nodes
  deque<Node*>* unvisited_nodes = GetNeighbors();
  deque<Node*> visited_nodes;
  list_of_node_nums.push_back(this->GetNumber()); //add this node's number to list_of_node_nums
  number_of_nodes++; //adds one to the number_of_nodes
  visited_nodes.push_back(this);
  while (!unvisited_nodes->empty()) //while unvisited nodes are empty
  {
    for(deque<Node*>::iterator curr_unvisited=unvisited_nodes->begin(); curr_unvisited!=unvisited_nodes->end(); curr_unvisited++) //loop through unvisited nodes
    {
      if(find(list_of_node_nums.begin(), list_of_node_nums.end(), (*curr_unvisited)->GetNumber()) == list_of_node_nums.end()) //if current unvisited node's number is not in list_of_nodes_nums
      {
        list_of_node_nums.push_back((*curr_unvisited)->GetNumber()); //add current unvisited neighbor's number to list_of_node_nums
        list_of_nodes.push_back(*curr_unvisited); //adds current univsited neighbor to list of nodes
        number_of_nodes++; //increment number_of_nodes counter by one
      }
      deque<Node*>* curr_neighbors = (*curr_unvisited)->GetNeighbors(); //get neighbors of current node
      for(deque<Node*>::iterator neighbor=curr_neighbors->begin(); neighbor!=curr_neighbors->end(); neighbor++) //loop through neighbors of current neighbor
      {
        if(find(unvisited_nodes->begin(), unvisited_nodes->end(), *neighbor) == unvisited_nodes->end()) //if neighbor is not in unvisited_nodes 
        {
          if(find(visited_nodes.begin(), visited_nodes.end(), neighbor) == visited_nodes.end()) //if neighbor is not in visited_nodes
          {
            unvisited_nodes->push_back(*neighbor); //add a pointer of neighbor (which is itself a pointer) to unvisited_nodes
          }
        }
      }
      visited_nodes.push_back(*curr_unvisited); //add current unvisited node to visited nodes
      unvisited_nodes->erase(curr_unvisited); //erase current unvisited node from unvisited nodes
    }
  }
  cerr << number_of_nodes << " nodes found in the network." << endl;
}


void Node::LinkHasBeenUpdated(const Link *l)
{
  //INCLUDE ADDING A NODE
  cerr << *this<<": Link Update: "<<*l<<endl;
  GetAllNodes();
  for(deque<Node*>::iterator node=list_of_nodes.begin(); node!=list_of_nodes.end(); node++)
  {
      (*node)->Djistras(); //call Djistras for each node in the network because it is possible the shortest path changed for every node
  }
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *m)
{ //message format: dest node, predecessor, new latency for the dest node
  cerr << *this << " Routing Message: "<<*m;
  Djistras();
}

void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored"<<endl;
}

Node *Node::GetNextHop(const Node *destination)
{
   // Calling Djistras to initialize the table 
   Djistras();
   // Using the initally initialised table to find the next hop
  unsigned dest_index = destination -> GetNumber();
   vector<unsigned> pVec = mytable.getPreds();
   unsigned current_pred = pVec[dest_index];
   Node* nextone;

   while(current_pred != number){
    dest_index = current_pred;
    current_pred = pVec[dest_index];    //change pred 
   }
   
   for(deque<Node*>::iterator point = list_of_nodes.begin(); point!=list_of_nodes.end();++point){
      if(dest_index == (*point)->GetNumber()){
        nextone = *point;
      }
   }
   return nextone;
}

Table *Node::GetRoutingTable()
{
  // WRITE

  return &(this->mytable);
}

void Node::Djistras(){
  
 
  unsigned srcIndex = GetNumber();

  bool  myneighbor_visited;
  GetAllNodes();
  // Creating an instance of class table to be able to get the predecessors and costs vectors
  Table* nodetable = new Table(number_of_nodes); 
  vector<unsigned> preds_vec = nodetable->getPreds();
  vector<double> costs_vec = nodetable->getCosts();
  vector<bool> visited_vec = nodetable->getVisited();
  
  mytable.setCosts(costs_vec);
  mytable.setPreds(preds_vec);
  mytable.setVisited(visited_vec);
  preds_vec = mytable.getPreds();
  costs_vec = mytable.getCosts();
  visited_vec = mytable.getVisited();
  
  // initializing the values for the root to be 0 
  preds_vec[srcIndex] = srcIndex;
  costs_vec[srcIndex] = 0.0;
  visited_vec[srcIndex] = true;
 // for(int i=0; i<number_of_nodes;i++)
  modifyTable(this, preds_vec, costs_vec, visited_vec);
  delete nodetable;
}

void Node::modifyTable(Node* root, vector<unsigned> preds_vec, vector<double> costs_vec, vector<bool> visited_vec){
    // START OF HELPER 
  unsigned destIndex;
  double linkCost, nodeLatency;
  unsigned myneighbor_number;
  double newCost;
  const double fixCost;
  unsigned srcIndex = this -> GetNumber();
  const Node* nextNode;
  this -> SetVisited(true);
  
  // Creating an instance of class topology to be able to get the neighbors and outgoing links for the current node
  Topology dTopology;
  deque<Node*> myneighbors = dTopology.GetNeighbors(root);
  deque<Link*> outLinks = dTopology.GetOutgoingLinks(root);
  
  for(deque<Link*>::iterator currLink = outLinks.begin(); currLink!= outLinks.end(); ++currLink){
    destIndex = (*currLink) -> GetDest();
    if(vec_visited[destIndex] == false){
    
        linkCost = (*currLink) -> GetLatency();
        //basically just root latency
        nodeLatency = root -> GetLatency();
        newCost = nodeLatency + linkCost;

         // replace if the new cost is less than the currentcost 
        if(newCost < costs_vec[destIndex]){
            preds_vec[destIndex] = srcIndex;
            costs_vec[destIndex] = newCost;
            // set latency ito be modified  
        }
    }
  }

  //Now we need to find the min value in this deque to get the next node to be set
  int min_value = 10000000000000;
  int min_index = 0;

  for (int i = 0; i < costs_vec.size(); i++) { //loop to find the next node to be fixed
    if (visited_vec[i] == false) { //skip the ones that were previously fixed
      if (costs_vec[i] < min_value) {
        min_value = costs_vec[i];
        min_index = i;
      }
    }
  }

  double myneighbor_cost;
  //MARKING the least cost neighbor as VISITED:
  for (deque<Node*>::iterator j = myneighbors.begin(); j != myneighbors.end(); ++j){
    //find the node to be fixed next
    myneighbor_number = (*j)->GetNumber();
    myneighbor_cost = (*j)->GetLatency();

    if(myneighbor_number == min_index) { //the min node is found
      visited_vec[myneighbor_number] = true; //set visited value to be true
      (*j)->SetLatency(costs_vec[myneighbor_number]); //set latency for this node
      nextNode = *j;
      break;
    }
  }

  modifyTable(nextNode, preds_vec, costs_vec, visited_vec); //run recursion
}

ostream & Node::Print(ostream &os) const
{
  os << "Node(number="<<number<<", lat="<<lat<<", bw="<<bw<<")";
  return os;
}

#endif


#if defined(DISTANCEVECTOR)

void Node::LinkHasBeenUpdated(const Link *link)
{
  cerr << *this << ": Link Update: " << *link << endl;

  unsigned dest = link->GetDest();
  double new_cost = link->GetLatency();
  RoutingPath* neighbor = table.GetRoutingPath(dest);

  /*
    Cases we considered
      1. No routing_path exists in the table -> should make a new one
      2. Cost to neighbor is lower than the current cost in table -> update this cost
      3. Cost in table is for direct path (information is possibly outdated) -> update the cost
  */
  if (neighbor == NULL|| 
    neighbor->cost > new_cost ||
    neighbor->destination == neighbor->next_node)
  {
    //edit the routing path with the new cost
    table.EditRoutingPath(dest, RoutingPath(dest, dest, new_cost));

    //send routing message to all neighbors with the updated information
    SendToNeighbors(new RoutingMessage(*this, Node(dest, context, 0, 0), new_cost));
    UpdatesFromNeighbors();
  }
}


void Node::ProcessIncomingRoutingMessage(const RoutingMessage *message)
{
  cerr << *this << ": Received Message: " << *message << endl;

  //unpack data from routing message
  Node src = message->src;
  unsigned src_number = src.GetNumber();
  Node dest = message->dest;
  unsigned dest_number = dest.GetNumber();
  double total_cost = message->cost;

  //unnecessary routing message received
  if (dest_number == GetNumber()) {
    return;
  }

  //check this node's distance to the destination in message
  RoutingPath* src_routing_path = table.GetRoutingPath(src_number);
  RoutingPath* dest_routing_path = table.GetRoutingPath(dest_number);

  //compare that with this node's distance to the source in the message + cost in the message

  /* Cases we considered:
      1. src_routing_path does not exist -> cannot update
      2. dest_routing_path does not exit -> should update
      3. cost in the routing table is greater than new cost -> should update
  */
  if (src_routing_path == NULL) {
    return;
  }
  else if (dest_routing_path == NULL || 
    dest_routing_path->cost > src_routing_path->cost + total_cost) 
  {
    double new_cost = src_routing_path->cost + total_cost;
    table.EditRoutingPath(dest_number, RoutingPath(dest_number, src_number, new_cost));

    SendToNeighbors(new RoutingMessage(*this, Node(dest_number, context, 0, 0), new_cost));
    UpdatesFromNeighbors();
  }
}

void Node::TimeOut()
{
  cerr << *this << " got a timeout: ignored" << endl;
}


Node *Node::GetNextHop(const Node *destination) const
{
  cerr << "Next Hop, Table: " << table << endl;
  unsigned dest_number = destination->GetNumber();
  RoutingPath* dest_routing_path = GetRoutingTable()->GetRoutingPath(dest_number);

  return new Node(dest_routing_path->next_node, NULL, 0, 0);
}

Table *Node::GetRoutingTable() const
{
  return new Table(table);
}

void Node::UpdatesFromNeighbors() 
{
  deque<RoutingPath> routing_paths = table.GetRoutingPaths();
  deque<Node*>* neighbors = GetNeighbors();

  for(deque<RoutingPath>::iterator routing_path = routing_paths.begin(); routing_path != routing_paths.end(); routing_path++)
  {
    double current_lowest_cost = routing_path->cost;
    unsigned current_next_node = routing_path->next_node;

    for(deque<Node*>::iterator neighbor = neighbors->begin(); neighbor != neighbors->end(); neighbor++)
    {
      RoutingPath* neighbor_routing_path = table.GetRoutingPath((*neighbor)->GetNumber());
      if (neighbor_routing_path != NULL)
      {
        double neighbor_cost = neighbor_routing_path->cost;
        RoutingPath* neighbor_to_dest = (*neighbor)->GetRoutingTable()->GetRoutingPath(routing_path->destination);
        if (neighbor_to_dest != NULL)
        {
          double this_cost = neighbor_cost + neighbor_to_dest->cost;
          if (this_cost < current_lowest_cost)
          {
            current_lowest_cost = this_cost;
            current_next_node = (*neighbor)->GetNumber();
          }
        }
      }
    }
    //if cost was lowered, we edit the routing path in the table and send to our neighbors
    if (current_lowest_cost != routing_path->cost)
    {
      table.EditRoutingPath(routing_path->destination, RoutingPath(routing_path->destination, current_next_node, current_lowest_cost));
      SendToNeighbors(new RoutingMessage(*this, Node(routing_path->destination, context, 0, 0), current_lowest_cost));
    }
  }
}


ostream & Node::Print(ostream &os) const
{
  os << "Node(number=" << number << ", lat=" << lat << ", bw=" << bw;
  return os;
}
#endif 
