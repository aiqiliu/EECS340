#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <iostream>
#include <algorithm>

#include "Minet.h"
#include "tcpstate.h"


using std::cout;
using std::endl;
using std::cerr;
using std::string;
using std::cin;
using std::min;
using std::max;


//          ~~~~~ MACROS ~~~~~

#define TMR_TRIES 5
#define MSS 536
#define GBN MSS*16
#define RTT 3


#define SEND_BUF_SIZE(state) (state.TCP_BUFFER_SIZE - state.SendBuffer.GetSize())


//           ~~~~~ HELPER ~~~~~

//helper function to make a packet
Packet MakePacket(Buffer data, Connection conn, unsigned int seq_n, unsigned int ack_n, size_t win_size, unsigned char flag)
{
  // Make Packet
  unsigned size = MIN_MACRO(IP_PACKET_MAX_LENGTH-TCP_HEADER_MAX_LENGTH, data.GetSize());
  Packet sndPacket(data.ExtractFront(size));

  // Make and push IP header
  IPHeader sendIPheader;
  sendIPheader.SetProtocol(IP_PROTO_TCP);
  sendIPheader.SetSourceIP(conn.src);
  sendIPheader.SetDestIP(conn.dest);
  sendIPheader.SetTotalLength(size + TCP_HEADER_BASE_LENGTH + IP_HEADER_BASE_LENGTH);
  sndPacket.PushFrontHeader(sendIPheader);

  // Make and push TCP header
  TCPHeader sendTCPheader;
  sendTCPheader.SetSourcePort(conn.srcport, sndPacket);
  sendTCPheader.SetDestPort(conn.destport, sndPacket);
  sendTCPheader.SetHeaderLen(TCP_HEADER_BASE_LENGTH/4, sndPacket);
  sendTCPheader.SetFlags(flag, sndPacket);
  sendTCPheader.SetWinSize(win_size, sndPacket); // to fix
  sendTCPheader.SetSeqNum(seq_n, sndPacket);
  if (IS_ACK(flag))
  {
    sendTCPheader.SetAckNum(ack_n, sndPacket);
  }
  sendTCPheader.RecomputeChecksum(sndPacket);
  sndPacket.PushBackHeader(sendTCPheader);

  cerr << "~~~MAKING PACKET~~~" << endl;
  cerr << sendIPheader << endl;
  cerr << sendTCPheader << endl;
  cerr << sndPacket << endl;

  return sndPacket;
}

//helper function to make packet and send it using Minet
void SendPacket(MinetHandle handle, Buffer data, Connection conn, unsigned int seq_n, unsigned int ack_n, size_t win_size, unsigned char flag)
{
  Packet pack = MakePacket(data, conn, seq_n, ack_n, win_size, flag); // ack
  MinetSend(handle, pack);
}

//helper function to receive a packet from a handle using minet and save it to a packet
Packet ReceivePacket(MinetHandle handle) 
{
  Packet pack;
  MinetReceive(handle, pack);
  return pack;
}

//         ~~~~~ MAIN ~~~~~                

int main(int argc, char *argv[])
{
  MinetHandle mux, sock;
  srand(time(0)); // generate a random number 
  ConnectionList<TCPState> connectionsList;

  MinetInit(MINET_TCP_MODULE);

  mux = MinetIsModuleInConfig(MINET_IP_MUX) ? MinetConnect(MINET_IP_MUX) : MINET_NOHANDLE;
  sock = MinetIsModuleInConfig(MINET_SOCK_MODULE) ? MinetAccept(MINET_SOCK_MODULE) : MINET_NOHANDLE;

  if (MinetIsModuleInConfig(MINET_IP_MUX) && mux == MINET_NOHANDLE) 
  {
    MinetSendToMonitor(MinetMonitoringEvent("Can't connect to mux"));
    return -1;
  }

  if (MinetIsModuleInConfig(MINET_SOCK_MODULE) && sock == MINET_NOHANDLE) 
  {
    MinetSendToMonitor(MinetMonitoringEvent("Can't accept from sock module"));
    return -1;
  }

  MinetSendToMonitor(MinetMonitoringEvent("tcp_module handling TCP traffic"));

  MinetEvent event;

  while (MinetGetNextEvent(event, 10) == 0) 
  {

    if (event.eventtype == MinetEvent::Timeout)
    {
      //loop through all the connections in the connections list
      for (ConnectionList<TCPState>::iterator cxn = connectionsList.begin(); cxn != connectionsList.end(); cxn++)
      {
        //if connection is closed, erase it from the connections list
        if (cxn->state.GetState() == CLOSED)
        {
          connectionsList.erase(cxn);
        }

        // check for active timers
        Time curr_time = Time();
        if (cxn->bTmrActive == true && cxn->timeout < curr_time)
        {
          // CAN GET RID OF THIS?
          // if there are no more timer tries for this state
          if (cxn->state.ExpireTimerTries()) //true if no more timer tries
          {
            //closes connection if the number of time outs reaches a threshold
            SockRequestResponse res;
            res.type = CLOSE;
            res.connection = cxn->connection;
            res.error = EOK;
            MinetSend(sock, res);
          }
          // else handle each case of timeout
          else
          {
            Packet sndPacket;
            unsigned char sendFlag;
            switch(cxn->state.GetState())
            {
              case SYN_RCVD:
              {
                cerr << "!!! TIMEOUT: SYN_RCVD STATE !!!  RE-SENDING SYN ACK !!!" << endl;
                SET_SYN(sendFlag);
                SET_ACK(sendFlag);
                SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), cxn->state.GetRwnd(), sendFlag);
              }
              break;
              case SYN_SENT:
              {
                cerr << "!!! TIMEOUT: SYN_SENT STATE !!! RE-SENDING SYN!!!" << endl;
                SET_SYN(sendFlag);
                SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), SEND_BUF_SIZE(cxn->state), sendFlag);
              } 
              break;
              case ESTABLISHED:
              {
                cerr << "!!! TIMEOUT: ESTABLISHED STATE !!! RE-SENDING DATA !!!" << endl;
                // use GBN to resend data
                if (cxn->state.N > 0)
                {
                  cerr << "!!! TIMEOUT: ESTABLISHED STATE !!! RE-SEND DATA USING GBN !!!" << endl;

                  unsigned int numInflight = cxn->state.GetN();
                  unsigned int recWindow = cxn->state.GetRwnd(); // receiver congestion window
                  size_t sndWindow = cxn->state.SendBuffer.GetSize(); // sender congestion window

                  Buffer data;
                  while(numInflight < GBN && sndWindow != 0 && recWindow != 0)
                  {
                    cerr << "\n numInflight: " << numInflight << endl;
                    cerr << "\n recWindow: " << recWindow << endl;
                    cerr << "\n sndWindow: " << sndWindow << endl;
                    sendFlag = 0;

                    // if MSS < recWindow and MSS < sndWindow
                    // space in recWindow and sndWindow
                    if(MSS < recWindow && MSS < sndWindow)
                    {
                      cerr << "space in recWindow and sndWindow" << endl;
                      data = cxn->state.SendBuffer.Extract(numInflight, MSS);

                      numInflight = numInflight + MSS;
                      CLR_SYN(sendFlag);
                      SET_ACK(sendFlag);
		                  SET_PSH(sendFlag);
                      sndPacket = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), sendFlag);

                      cerr << "Last last sent: " << cxn->state.GetLastSent() << endl;
                      cxn->state.SetLastSent(cxn->state.GetLastSent() + MSS);
                      cerr << "Last sent: " << cxn->state.GetLastSent() << endl;
                    }

                    // else space in sndWindow or recWindow
                    else
                    {
                      cerr << "space in either or" << endl;
                      data = cxn->state.SendBuffer.Extract(numInflight, min((int)recWindow, (int)sndWindow));

                      numInflight = numInflight + min((int)recWindow, (int)sndWindow);
                      CLR_SYN(sendFlag);
                      SET_ACK(sendFlag);
		                  SET_PSH(sendFlag);
                      sndPacket = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), sendFlag);
                      cxn->state.SetLastSent(cxn->state.GetLastSent() + min((int)recWindow, (int)sndWindow));
                    }

                    MinetSend(mux, sndPacket);

                    if ((recWindow < recWindow - numInflight) && (sndWindow < sndWindow - numInflight)) 
                    {
                      break;
                    }
                    else
                    {
                      recWindow = recWindow - numInflight;
                      sndWindow = sndWindow - numInflight;                
                    }

                    cerr << "\n numInflight: " << numInflight << endl;
                    cerr << "recWindow: " << recWindow << endl;
                    cerr << "sndWindow: " << sndWindow << endl;

                    // set timeout
                    cxn->bTmrActive = true;
                    cxn->timeout = Time() + RTT;
                  }


                  cxn->state.N = numInflight;

                }
                // otherwise just need to resend ACK
                else
                {
                  cerr << "!!! TIMEOUT: ESTABLISHED STATE !!! RE-SENDING ACK !!!" << endl;
                  SET_ACK(sendFlag);
                  SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, cxn->state.GetLastRecvd(), sendFlag);
                }
              }
              break;
              case CLOSE_WAIT:
              {
                cerr << "!!! TIMEOUT: CLOSE_WAIT STATE !!! RE-SENDING ACK !!!" << endl;
                SET_ACK(sendFlag);
                SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), cxn->state.GetLastRecvd(), sendFlag);
              }
              break;
              case FIN_WAIT1:
              {
                cerr << "!!! TIMEOUT: FIN_WAIT1 STATE !!! RE-SENDING FIN !!!" << endl;
                SET_FIN(sendFlag);
                SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), SEND_BUF_SIZE(cxn->state), sendFlag);
              }
              break;
              case CLOSING:
              {
                cerr << "!!! TIMEOUT: CLOSING STATE !!! RE-SENDING ACK !!!" << endl;
                SET_ACK(sendFlag);
                SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), cxn->state.GetLastRecvd(), sendFlag);
              }
              break;
              case LAST_ACK:
              {
                cerr << "!!! TIMEOUT: LAST_ACK !!! RE-SENDING FIN !!!" << endl;
                SET_FIN(sendFlag);
                SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), SEND_BUF_SIZE(cxn->state), sendFlag);
              }
              break;
              case TIME_WAIT:
              {
                cerr << "!!! TIMEOUT: TIME_WAIT !!! RE-SENDING ACK !!!" << endl;
                SET_ACK(sendFlag);
                SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd(), cxn->state.GetLastRecvd(), sendFlag);
              }
            }
            MinetSend(mux, sndPacket);
            cxn->timeout = Time() + RTT;
          }
        }
      }
    }
    // Unexpected event type, signore
    else if (event.eventtype != MinetEvent::Dataflow || event.direction != MinetEvent::IN)
    {
      MinetSendToMonitor(MinetMonitoringEvent("Unknown event ignored."));
    } 
    //  Data from the IP layer below 
    else if (event.handle == mux) 
    {
      cerr << "\n~~~ START OF IP LAYER ~~~ \n";

      cerr << "  ~~~  PACKET RECEIVED FROM BELOW ~~~\n";

      Packet receivedPacket = ReceivePacket(mux);
      // MinetReceive(mux, receivedPacket);

      unsigned tcphlen = TCPHeader::EstimateTCPHeaderLength(receivedPacket);
      receivedPacket.ExtractHeaderFromPayload<TCPHeader>(tcphlen);

      IPHeader recIPheader = receivedPacket.FindHeader(Headers::IPHeader);
      TCPHeader recTCPheader = receivedPacket.FindHeader(Headers::TCPHeader);

      // cerr << recIPheader <<"\n";
      // cerr << recTCPheader << "\n";
      // cerr << "Checksum is " << (recTCPheader.IsCorrectChecksum(receivedPacket) ? "VALID\n\n" : "INVALID\n\n");
      // cerr << receivedPacket << "\n";

      // Unpack useful data
      Connection conn;
      // what's the connection class?
      recIPheader.GetDestIP(conn.src);
      recIPheader.GetSourceIP(conn.dest);
      recIPheader.GetProtocol(conn.protocol);
      recTCPheader.GetDestPort(conn.srcport);
      recTCPheader.GetSourcePort(conn.destport);

      unsigned short recWindow; // cpuld change it to int recWindow
      recTCPheader.GetWinSize(recWindow);

      unsigned int recSeqNum;
      unsigned int recAckNum;
      unsigned int sendAckNum;
      unsigned int sendSeqNum;
      recTCPheader.GetSeqNum(recSeqNum);
      recTCPheader.GetAckNum(recAckNum);
      // Since ack_n denotes the next packet expected
      sendAckNum = recSeqNum + 1;

      unsigned char receivedFlag;
      recTCPheader.GetFlags(receivedFlag);

      ConnectionList<TCPState>::iterator cxn = connectionsList.FindMatching(conn);
      if (cxn != connectionsList.end() && recTCPheader.IsCorrectChecksum(receivedPacket))
      {   
        // cerr << "Found matching connection\n";
        // cerr << "Last Acked: " << cxn->state.GetLastAcked() << endl;
        // cerr << "Last Sent: " << cxn->state.GetLastSent() << endl;
        // cerr << "Last Recv: " << cxn->state.GetLastRecvd() << endl << endl;;
        recTCPheader.GetHeaderLen((unsigned char&)tcphlen); // what is the GetHeaderLen () ?
        tcphlen -= TCP_HEADER_MAX_LENGTH;
        Buffer data = receivedPacket.GetPayload().ExtractFront(tcphlen); // getting the contents as long as tcphlen from the payload
        data.Print(cerr); // why?
        cerr << endl;

        unsigned char sendFlag = 0; // SET_SYN function needs a parameter of type unsigned char which is then ORed with 10 (bitwise OR)
        SockRequestResponse res;
        Packet sndPacket;

        switch(cxn->state.GetState())
        {
          case CLOSED:
          {
            cerr << "\n   ~~~ MUX: CLOSED STATE ~~~\n";
          }
          break;
          case LISTEN:
          {
            cerr << "\n   ~~~ MUX: LISTEN STATE ~~~\n";
            // coming from ACCEPT in socket layer
            if (IS_SYN(receivedFlag))
            {
              sendSeqNum = rand(); // As the sequence number for the packet sent back is randomly chosen
              //cerr << "generated seq: " << sendSeqNum << endl;

              cxn->state.SetState(SYN_RCVD);
              cxn->state.SetLastRecvd(recSeqNum);
             // cerr << "SET1: " << cxn->state.GetLastSent() << endl;
              cxn->state.SetLastSent(sendSeqNum); 
              // cerr << "SET2: " << cxn->state.GetLastSent() << endl;

              cxn->bTmrActive = true; // bTmrActive is ??? To denote that the timer is now active
              cxn->timeout = Time() + RTT; // To set the timeout interval to be that of 1 RTT starting the current time. This sets a timeout for receiving an ACK from remote side.
              SET_SYN(sendFlag);
              SET_ACK(sendFlag);
              SendPacket(mux, Buffer(NULL, 0), conn, sendSeqNum, sendAckNum, cxn->state.GetLastRecvd(), sendFlag);
            }
          }
          break;
          case SYN_RCVD:
          {
            cerr << "\n   ~~~ MUX: SYN_RCVD STATE ~~~\n";
            if (IS_ACK(receivedFlag)) {
              cerr << "Received packet is ACKED! Will try to establish connection!" << endl;
            }

            if (IS_ACK(receivedFlag) && cxn->state.GetLastSent() == recAckNum - 1)
            {
              cerr << "Connection is established!" << endl;
              cxn->state.SetState(ESTABLISHED);
              cxn->state.SetLastRecvd(recSeqNum - 1); // next will have same seq num

              // timer
              cxn->bTmrActive = false;
              cxn->state.SetTimerTries(TMR_TRIES);

              // create res to send to sock
              res.type = WRITE;
              res.connection = conn;
              res.bytes = 0;
              res.error = EOK;
              MinetSend(sock, res);
            }
          }
          break;
          case SYN_SENT:
          {
            cerr << "\n   ~~~ MUX: SYN_SENT STATE ~~~\n";
            if (IS_SYN(receivedFlag) && IS_ACK(receivedFlag))
            {
              cerr << "Last Acked: " << cxn->state.GetLastAcked() << endl;
              cerr << "Last Sent: " << cxn->state.GetLastSent() << endl;
              cerr << "Last Recv: " << cxn->state.GetLastRecvd() << endl;
              sendSeqNum = cxn->state.GetLastSent() + data.GetSize() + 1;
              cerr << "increment" << data.GetSize() + 1;

              cxn->state.SetState(ESTABLISHED);
              cxn->state.SetLastAcked(recAckNum - 1);
              cxn->state.SetLastRecvd(recSeqNum); // first data will be the same as this


              SET_ACK(sendFlag);
              SendPacket(mux, Buffer(NULL, 0), conn, sendSeqNum, sendAckNum, SEND_BUF_SIZE(cxn->state), sendFlag);

              // NOTE: we do this for compatibility with our tcp_server testing, which seems to receives size 6 ACKs
              // from us even though the size is shown as zero above.
              cxn->state.SetLastSent(max((int) cxn->state.GetLastSent() + 7, (int) sendSeqNum));

              res.type = WRITE;
              res.connection = conn;
              res.bytes = 0;
              res.error = EOK;
              MinetSend(sock, res);
            }
          }
          break;
          case SYN_SENT1:
          {
            cerr << "\n   ~~~ MUX: SYN_SENT STATE ~~~\n";
            // NO IMPLEMENTATION NEEDED
          }
          break;
          case ESTABLISHED:
          {
            cerr << "\n   ~~~ MUX: ESTABLISHED STATE ~~~\n";

            if (IS_FIN(receivedFlag))
            { // part16. activate close
              cerr << "FIN flagged.\n";
              sendSeqNum = cxn->state.GetLastSent() + data.GetSize() + 1;

              cxn->state.SetState(CLOSE_WAIT); //passive close
              cerr << "Last last sent: " << cxn->state.GetLastSent() << endl;
              cxn->state.SetLastSent(sendSeqNum);
              cerr << "Last sent: " << cxn->state.GetLastSent() << endl;
              cxn->state.SetLastRecvd(recSeqNum);

              SET_ACK(sendFlag); //send back an ACK for the FIN received
              SendPacket(mux, Buffer(NULL, 0), conn, sendSeqNum, sendAckNum, cxn->state.GetLastRecvd(), sendFlag);

              //might include later for debugging
              // res.type = CLOSE;
              // res.error = EOK; 
              // MinetSend(sock, res); 
            }
            //else, is a dataflow packet
            else
            { 
              if (IS_ACK(receivedFlag) && cxn->state.GetLastRecvd() < recSeqNum)
              {
                cerr << "ACK is flagged." << endl;

                //packet has data
                if (data.GetSize() > 0)
                {
                  cerr << "The received packet has data" << endl;
                  cerr << "Recv: " << cxn->state.GetLastRecvd() << endl;
                
                  size_t recvBufferSize = cxn->state.GetLastRecvd();
                  //if there is an overflow of the received data
                  if (recvBufferSize < data.GetSize()) 
                  {
                    cxn->state.RecvBuffer.AddBack(data.ExtractFront(recvBufferSize)); //extract first n bits from data to recvbuffer
                    sendAckNum = recSeqNum + recvBufferSize - 1;
                    cxn->state.SetLastRecvd(sendAckNum);
                  }
                  else //if there is no overflow
                  {
                    cxn->state.RecvBuffer.AddBack(data);
                    sendAckNum = recSeqNum + data.GetSize() - 1;
                    cxn->state.SetLastRecvd(sendAckNum);
                  }

                  //grabbing the next sequence number to send 
                  sendSeqNum = cxn->state.GetLastSent() + min(recvBufferSize, data.GetSize());
                  cxn->state.SetLastSent(sendSeqNum);

                  //send an empty packet with ACK flag to mux to acknowledge the last received packet
                  SET_ACK(sendFlag);
                  SendPacket(mux, Buffer(NULL, 0), conn, sendSeqNum, sendAckNum + 1, cxn->state.GetLastRecvd(), sendFlag);

                  //create a socketrequestresponse to send to sock (its a write request to the socket)
                  res.type = WRITE;
                  res.connection = conn;
                  res.data = cxn->state.RecvBuffer; //send data in recvbuffer to sock
                  res.bytes = cxn->state.RecvBuffer.GetSize();
                  res.error = EOK;
                  MinetSend(sock, res);
                }
                else //we receive an empty ACK packet, and want to send our packet out of the send buffer using flow control
                { //second cycle, client sends an ACK to server 
                  cxn->state.SendBuffer.Erase(0, recAckNum - cxn->state.GetLastAcked() - 1);
                  cxn->state.N = cxn->state.N - (recAckNum - cxn->state.GetLastAcked() - 1);

                  cxn->state.SetLastAcked(recAckNum);
                  cxn->state.SetLastRecvd(recSeqNum);
                  cxn->state.last_acked = recAckNum;


                  cerr << "\nSend Buffer: ";
                  cxn->state.SendBuffer.Print(cerr);
                  cerr << endl;

                  cxn->state.N = cxn->state.N - (recAckNum - cxn->state.GetLastAcked() - 1);
  
                  // send some of the information in the buffer if there is an overflow in the sendbuffer
                  if (cxn->state.SendBuffer.GetSize() - cxn->state.GetN() > 0)
                  {
                    //WHAT IS STATE.GETN() or STATE.N???
                    unsigned int numInflight = cxn->state.GetN(); //packets in flight
                    unsigned int recWindow = cxn->state.GetRwnd(); //receiver congestion window
                    size_t sndWindow = cxn->state.SendBuffer.GetSize(); //sender congestion window

                    while(numInflight < GBN && sndWindow != 0 && recWindow != 0) //GBN is a macro defined at the top
                    {
                      cerr << "\n numInflight: " << numInflight << endl;
                      cerr << "\n recWindow: " << recWindow << endl;
                      cerr << "\n sndWindow: " << sndWindow << endl;
                      sendFlag = 0;

                      // if MSS < recWindow and MSS < sndWindow
                      // space in recWindow and sndWindow
                      if(MSS < recWindow && MSS < sndWindow)
                      {
                        cerr << "There is still space in the receiver window and sender window" << endl; 
                        data = cxn->state.SendBuffer.Extract(numInflight, MSS); //extract data of size MSS from send buffer (offset of # packets inflight)
                        // set new sequence number
                        // move on to the next set of packets
                        numInflight = numInflight + MSS;
                        CLR_SYN(sendFlag);
                        SET_ACK(sendFlag);
			                  SET_PSH(sendFlag);
                        sndPacket = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), sendFlag);

                        cerr << "Last last sent: " << cxn->state.GetLastSent() << endl;
                        cxn->state.SetLastSent(cxn->state.GetLastSent() + MSS); //adjust LastSent to account for the MSS just sent
                        cerr << "Last sent: " << cxn->state.GetLastSent() << endl;
                      }
                      // else there space is not enough space in sender window or receiver window
                      else
                      {
                        cerr << "Limited space in either sender window or receiver window" << endl;
                        data = cxn->state.SendBuffer.Extract(numInflight, min((int)recWindow, (int)sndWindow)); //extract data of size min(windows) from send buffer
                        // set new sequence number
                        // move on to the next set of packets
                        numInflight = numInflight + min((int)recWindow, (int)sndWindow);
                        CLR_SYN(sendFlag);
                        SET_ACK(sendFlag);
			                  SET_PSH(sendFlag);
                        sndPacket = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), sendFlag);
                        cxn->state.SetLastSent(cxn->state.GetLastSent() + min((int)recWindow, (int)sndWindow));
                      }

                      MinetSend(mux, sndPacket); //send the packet to mus

                      if ((recWindow < recWindow - numInflight) && (sndWindow < sndWindow - numInflight)) // CAN GET RID OF THIS FIRST IF?
                      {
                        break;
                      }
                      else
                      {
                        recWindow = recWindow - numInflight;
                        sndWindow = sndWindow - numInflight;                
                      }

                      cerr << "\n numInflight: " << numInflight << endl;
                      cerr << "recWindow: " << recWindow << endl;
                      cerr << "sndWindow: " << sndWindow << endl;
                      // set timeout LOOK IN THIS TIMER THING
                      cxn->bTmrActive = true;
                      cxn->timeout = Time() + RTT;
                    }

                    cxn->state.N = numInflight;
                  }
                  
                }
              }
            }
          }
          break;
          case SEND_DATA:
          {
            cerr << "\n   ~~~ MUX: SEND_DATA STATE ~~~\n";
            // NO IMPLEMENTATION NEEDED
          }
          break;
          case CLOSE_WAIT:
          {
            cerr << "\n   ~~~ MUX: CLOSE_WAIT STATE ~~~\n";
            //at this stage, need to wait for local user to terminate connection, then we send our own FIN

            if (IS_FIN(receivedFlag))
            {
              // send a fin ack back
              sendSeqNum = cxn->state.GetLastSent() + data.GetSize() + 1;

              cxn->state.SetState(LAST_ACK);
              cxn->state.SetLastRecvd(recSeqNum);
              cerr << "Last last sent: " << cxn->state.GetLastSent() << endl;
              cxn->state.SetLastSent(sendSeqNum);
              cerr << "Last sent: " << cxn->state.GetLastSent() << endl;

              // timeout stuff
              cxn->bTmrActive = true;
              cxn->timeout = Time() + RTT;
              cxn->state.SetTimerTries(TMR_TRIES);

              SET_FIN(sendFlag);
              SendPacket(mux, Buffer(NULL, 0), conn, sendSeqNum, sendAckNum, cxn->state.GetLastRecvd(), sendFlag);
            }
          }
          break;
          case FIN_WAIT1: //this state is after the client actively sent a FIN to the other user and is waiting for a response
          {
            cerr << "\n   ~~~ MUX: FIN_WAIT1 STATE ~~~\n";
            if (IS_FIN(receivedFlag)) //if other user sends a FIN back, we close the connection
            {
              sendSeqNum = cxn->state.GetLastSent() + data.GetSize() + 1;

              cxn->state.SetState(CLOSING); //set state to closing
              cxn->state.SetLastRecvd(recSeqNum); //can replace these code with helper 
              cerr << "Last last sent: " << cxn->state.GetLastSent() << endl;
              cxn->state.SetLastSent(sendSeqNum);
              cerr << "Last sent: " << cxn->state.GetLastSent() << endl;

              // set timeout
              cxn->bTmrActive = true;
              cxn->timeout = Time() + RTT;
              cxn->state.SetTimerTries(TMR_TRIES);

              SET_FIN(sendFlag); 
              SET_ACK(sendFlag); 
              SendPacket(mux, Buffer(NULL, 0), conn, sendSeqNum, sendAckNum, SEND_BUF_SIZE(cxn->state), sendFlag);
            }
            else if (IS_ACK(receivedFlag)) //received an ACK back after first sending a FIN, so set state to FIN_WAIT2
            {

              cxn->state.SetState(FIN_WAIT2);
              cxn->state.SetLastSent(sendSeqNum);
              cxn->state.SetLastAcked(recAckNum - 1);
            }
          }
          break;
          case CLOSING:
          {
            cerr << "\n   ~~~ MUX: CLOSING STATE ~~~\n";
            if (IS_ACK(receivedFlag))
            {
              // done, not sending any other packets
              // move to time-wait
              cxn->state.SetState(TIME_WAIT);
              cxn->state.SetLastAcked(recAckNum - 1);
              cxn->state.SetLastRecvd(recSeqNum);
            }
          }
          break;
          case LAST_ACK:
          { //start of sending the second fin
            cerr << "\n   ~~~ MUX: LAST_ACK STATE ~~~\n";
            if (IS_ACK(receivedFlag))
            {
              cxn->state.SetState(CLOSED);
              cxn->state.SetLastAcked(recAckNum - 1);
              cxn->state.SetLastRecvd(recSeqNum);
            }
          }
          break;
          case FIN_WAIT2: //waiting for a FIN from the server, and then will send an ACK back and change to time_wait state
          {
            cerr << "\n   ~~~ MUX: FIN_WAIT2 STATE ~~~\n";
            if (IS_FIN(receivedFlag)) //if receive FIN from server, will send an ACK back to server and change to time_wait state
            {
              sendSeqNum = cxn->state.GetLastSent() + data.GetSize() + 1;

              cxn->state.SetState(TIME_WAIT);
              cxn->state.SetLastRecvd(recSeqNum);
              cxn->state.SetLastSent(sendSeqNum);
              cxn->state.SetLastAcked(recAckNum - 1);
 
              // set timeout
              cxn->bTmrActive = true;
              cxn->timeout = Time() + RTT;
              cxn->state.SetTimerTries(TMR_TRIES);

              //send ACK back to server after receiving their FIN
              SET_ACK(sendFlag); 
              SendPacket(mux, Buffer(NULL, 0), conn, sendSeqNum, sendAckNum, SEND_BUF_SIZE(cxn->state), sendFlag);
            }
          }
          break;
          case TIME_WAIT:
          {
            cerr << "\n   ~~~ MUX: TIME_WAIT STATE ~~~\n";
            cxn->timeout = Time() + 30;
            cxn->state.SetState(CLOSED);
          }
          break;
          default:
          {
            cerr << "\n   ~~~ MUX: DEFAULTED STATE ~~~\n";
          }
          break;
        }
      }
      // else there is no open connection
      else
      {
        cerr << "Could not find matching connection\n";
      }
      cerr << "\n~~~ END OF IP LAYER ~~~ \n";
    }

    //  Data from the Sockets layer above  //
    else if (event.handle == sock) 
    {
      cerr << "\n~~~ START OF SOCKET LAYER ~~~ \n";
      SockRequestResponse req;
      SockRequestResponse res;
      MinetReceive(sock, req);
      Packet sndPacket;
      unsigned char sendFlag;
      cerr << "Received Socket Request:" << req << endl;

      switch(req.type)
      {
        case CONNECT:
        {
          cerr << "\n   ~~~ SOCK: CONNECT ~~~\n";

          unsigned int initialSeqNum = rand(); // Can make this a specific wierd value rather than the rand() function.
          TCPState connectConn(initialSeqNum, SYN_SENT, TMR_TRIES); //state is SYN_SENT
          connectConn.N = 0; // number of packets allowed in flight
          ConnectionToStateMapping<TCPState> newConn(req.connection, Time(), connectConn, true); // sets properties for connection, timeout, state, timer
          connectionsList.push_front(newConn); // Add this new connection to the list of connections
         
          res.type = STATUS;
          res.error = EOK; 
          MinetSend(sock, res); //send an ok status to sock

          SET_SYN(sendFlag);
          // Packet sndPacket = MakePacket(Buffer(NULL, 0), newConn.connection, initialSeqNum, 0, SEND_BUF_SIZE(newConn.state), sendFlag);
          // MinetSend(mux, sndPacket); //send a SYN packet to mux
          SendPacket(mux, Buffer(NULL, 0), newConn.connection, initialSeqNum, 0, SEND_BUF_SIZE(newConn.state), sendFlag);

          //sleep for ARP caveat problem
          // sleep(5);
          // SendPacket(mux, Buffer(NULL, 0), newConn.connection, initialSeqNum, 0, SEND_BUF_SIZE(newConn.state), sendFlag);
          // cerr << "\n~~~ SOCK: END CONNECT ~~~\n";
        }
        break;
        case ACCEPT:
        {
          // passive open
          cerr << "\n   ~~~ SOCK: ACCEPT ~~~\n";

          TCPState acceptConnection(rand(), LISTEN, TMR_TRIES);
          acceptConnection.N = 0;
          ConnectionToStateMapping<TCPState> newConnection(req.connection, Time(), acceptConnection, false); //new connection with accept connection state
          connectionsList.push_front(newConnection); //push newconnection to connection list
         
          res.type = STATUS;
          res.connection = req.connection;
          res.bytes = 0;
          res.error = EOK;
          MinetSend(sock, res); //send sockrequestresponse to sock

          cerr << "\n   ~~~ SOCK: END ACCEPT ~~~\n";
        }
        break;
        case WRITE: 
        {
          cerr << "\n   ~~~ SOCK: WRITE ~~~\n";

          ConnectionList<TCPState>::iterator cxn = connectionsList.FindMatching(req.connection);
          if (cxn != connectionsList.end() && cxn->state.GetState() == ESTABLISHED)
          {
            cerr << "\n   ~~~ SOCK: WRITE: CONNECTION FOUND ~~~\n";

            size_t sendBufferSize = SEND_BUF_SIZE(cxn->state);
            //if there is overflow in the sendbuffer
            if (sendBufferSize < req.bytes)
            {
              //add to some of the data from req to the sendbuffer
              cxn->state.SendBuffer.AddBack(req.data.ExtractFront(sendBufferSize));

              res.bytes = sendBufferSize; //size of res.data is the size of the send buffer
              res.error = EBUF_SPACE; //error to indicate lack fo buffer space
            }
            //otherwise if there is no overflow
            else
            {
              cxn->state.SendBuffer.AddBack(req.data);

              res.bytes = req.bytes;
              res.error = EOK;
            }
            
            cxn->state.SendBuffer.Print(cerr);
            cerr << endl;

            res.type = STATUS;
            res.connection = req.connection;
            MinetSend(sock, res);

            // send data from buffer using "Go Back N"
            unsigned int numInflight = 0; // packets in flight
            unsigned int recWindow = cxn->state.GetRwnd(); // receiver congestion window
            size_t sndWindow = cxn->state.SendBuffer.GetSize(); // sender congestion window

            cerr << "\n outside of gbn loop\n";
            cerr << "numInflight: " << numInflight << endl;
            cerr << "recWindow: " << recWindow << endl;
            cerr << "sndWindow: " << sndWindow << endl;
            cerr << "last SENT: " << cxn->state.GetLastSent() << endl;
            cerr << "last ACKED: " << cxn->state.GetLastAcked() << endl;
            
            // iterate through all the packets
            Buffer data;
            while(numInflight < GBN && sndWindow != 0 && recWindow != 0) //GBN is a macro defined at the top (16)
            {
              cerr << "\n   ~~~ SOCK: WRITE: GBN LOOP ~~~\n";
              // if MSS < recWindow and MSS < sndWindow
              // space in recWindow and sndWindow
              if(MSS < recWindow && MSS < sndWindow)
              {
                cerr << "There is still space in the receiver window and sender window" << endl;
                data = cxn->state.SendBuffer.Extract(numInflight, MSS); //extract data of size MSS from send buffer (offset of # packets inflight)
                // set new seq_n
                // move on to the next set of packets
                numInflight = numInflight + MSS;
                CLR_SYN(sendFlag);
                SET_ACK(sendFlag);
		            SET_PSH(sendFlag);
                sndPacket = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), sendFlag);

                cerr << "Last last sent: " << cxn->state.GetLastSent() << endl; 
                cxn->state.SetLastSent(cxn->state.GetLastSent() + MSS); //adjust LastSent to account for the MSS just sent
                cerr << "Last sent: " << cxn->state.GetLastSent() << endl;
              }
              // else there space is not enough space in sender window or receiver window
              else
              {
                cerr << "Limited space in either sender window or receiver window" << endl;
                data = cxn->state.SendBuffer.Extract(numInflight, min((int)recWindow, (int)sndWindow)); //extract data of size min(windows) from send buffer
                // set new seq_n
                // move on to the next set of packets
                numInflight = numInflight + min((int)recWindow, (int)sndWindow);
                CLR_SYN(sendFlag);
                SET_ACK(sendFlag);
		            SET_PSH(sendFlag);
                sndPacket = MakePacket(data, cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, SEND_BUF_SIZE(cxn->state), sendFlag);
                cxn->state.SetLastSent(cxn->state.GetLastSent() + min((int)recWindow, (int)sndWindow));
              }

              MinetSend(mux, sndPacket); //send the packet to mux

              if ((recWindow < recWindow - numInflight) && (sndWindow < sndWindow - numInflight)) // CAN GET RID OF THIS FIRST IF?
              {
                break;
              }
              else
              {
                recWindow = recWindow - numInflight;
                sndWindow = sndWindow - numInflight;                
              }

              cerr << "\n numInflight: " << numInflight << endl;
              cerr << "recWindow: " << recWindow << endl;
              cerr << "sndWindow: " << sndWindow << endl;
              // set timeout
              //NEED TO SET TIMEOUT FOR THIS TOO???
            }

            cxn->state.N = numInflight;
          }
          else
          {
            cerr << "\n   ~~~ SOCK: WRITE: NO CONNECTION FOUND ~~~\n";
            res.connection = req.connection;
            res.type = STATUS;
            res.bytes = req.bytes;
            res.error = ENOMATCH;
          }
          
          cerr << "\n   ~~~ SOCK: END WRITE ~~~\n";
        }
        break;
        case FORWARD:
        {
          cerr << "\n   ~~~ SOCK: FORWARD ~~~\n";
          
          cerr << "\n   ~~~ SOCK: END FORWARD ~~~\n";
        }
        break;
        case CLOSE:
        {
          cerr << "\n   ~~~ SOCK: CLOSE ~~~\n";
          ConnectionList<TCPState>::iterator cxn = connectionsList.FindMatching(req.connection);
          if (cxn->state.GetState() == ESTABLISHED) //established connection, now call for close 
          {
            unsigned char sendFlag;
            Packet sndPacket;
            cxn->state.SetState(FIN_WAIT1);
            SET_FIN(sendFlag); //send fin to activate closing
            SendPacket(mux, Buffer(NULL, 0), cxn->connection, cxn->state.GetLastSent(), cxn->state.GetLastRecvd() + 1, cxn->state.GetLastRecvd(), sendFlag);
          }
          cerr << "\n   ~~~ SOCK: END CLOSE ~~~\n";
        }
        break; 
        case STATUS: //MIGHT HAVE TO CHANGE THIS!!! PERSONALIZE IT!!!
        {
          cerr << "\n   ~~~ SOCK: STATUS ~~~\n";
          ConnectionList<TCPState>::iterator cxn = connectionsList.FindMatching(req.connection);
          if (cxn->state.GetState() == ESTABLISHED)
          {
            //ISNT RECVBUFFER THE AMOUNT THAT CAN BE RECEIVED? NOT THE AMOUNT THAT IS ACTUALLY RECEIVED.
            cerr << req.bytes << " out of " << cxn->state.RecvBuffer.GetSize() << " bytes read" << endl;
            // if all data read
            if (req.bytes == cxn->state.RecvBuffer.GetSize())
            {
              cxn->state.RecvBuffer.Clear();
            }
            // if some data still in buffer
            else
            {
              cxn->state.RecvBuffer.Erase(0, req.bytes);

              //resend the WRITE request to the sock
              res.type = WRITE;
              res.connection = req.connection;
              res.data = cxn->state.RecvBuffer;
              res.bytes = cxn->state.RecvBuffer.GetSize();
              res.error = EOK;

              MinetSend(sock, res);
            }
          }
          cerr << "\n   ~~~ SOCK: END STATUS ~~~\n";
        }
        break;
        default:
        {
          cerr << "\n   ~~~ SOCK: DEFAULT ~~~\n";
          cerr << "\n   ~~~ SOCK: END DEFAULT ~~~\n";
        } 
          // TODO: responsd to request with
        break;

      }

      cerr << "\n~~~ END OF SOCKET LAYER ~~~ \n";

    }
  }
  return 0;
}
