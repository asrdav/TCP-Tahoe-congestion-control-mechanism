#include "sender.h"

enum {slowstart, congestionavoidance};



void c_socket(int *s_socket){
    // Creating the socket
    if((*s_socket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation failure");
        exit(EXIT_FAILURE);
    }
    // Binding
    if(c_connect(*s_socket) < 0)
    {
        perror("Socket binding failure");
        exit(EXIT_FAILURE);
    }
    // Listening
    if(listen(*s_socket, 10) < 0)
    {
        perror("Socket listening failure");
        exit(EXIT_FAILURE);
    }
}


int c_connect(int sock){

    int client_port = 3000;
    struct sockaddr_in  s_remote;

    // Connection Setup
    s_remote.sin_family = PF_INET;
    s_remote.sin_addr.s_addr = htonl(INADDR_ANY);
    s_remote.sin_port = htons(client_port);

    // Binding
    return bind(sock, (struct sockaddr *)&s_remote, sizeof(s_remote));
}


void c_write(int r_socket, int num_seq, unsigned char *packet, char *buffer, double BER, int *end){
    
    int tmp;
    short int crc;

    packet[0] = num_seq >> 8;
    packet[1] = num_seq & 0xff;

    // Get data from file
    if(num_seq >= 1000){
    
        tmp = (num_seq - 1000) * 2;
        packet[2] = buffer[tmp];
        packet[3] = buffer[tmp + 1];

        if(packet[2] == 0 || packet[3] == 0)
            *end = 1;

        // CRC
        crc = calculate_CCITT16(packet, PACKET_SIZE - 2, GENERATE_CRC);
        packet[4] = crc >> 8;
        packet[5] = crc & 0xff;

        //null terminated
        packet[6] = '\0';
        
        //adding congestion
        AddCongestion((char *)packet, BER);
    }

    printf("sending %d: %d %d %d %d ('%c' '%c')\n", num_seq, packet[0], packet[1],
           packet[2], packet[3], packet[2], packet[3]);
    
    // send file data
    if(send(r_socket, packet, PACKET_SIZE, 0) < 0){
        perror("Sending packet failure");
        exit(EXIT_FAILURE);
    }
}


void c_read(char *filename, char *buffer){
    
    FILE *file;
    //read the input file
    file = fopen(filename, "r");
    
    if(file == NULL){
        fprintf(stderr, "I/O Error: Output file creation failure\n");
        exit(EXIT_FAILURE);
    }

    fgets(buffer, BUFFER_SIZE, file);
    // Output file closing
    if(fclose(file) < 0){
        perror("Closing input file failure");
        exit(EXIT_FAILURE);
    }
}


void c_close(int s_socket, int r_socket)
{
    // Closing the reciever socket
    if(close(r_socket) < 0){
        perror("Closing connection socket failure");
        exit(EXIT_FAILURE);
    }

    // Closing the sender socket
    if(close(s_socket) < 0){
        perror("Closing socket failure");
        exit(EXIT_FAILURE);
    }
}






//Sender program
int main(int argc, char *argv[])
{
    int s_socket, r_socket;
    struct sockaddr_in client;
    socklen_t client_len;
    
    char address[MAX_ADDRESS]; //input arg
    char filename[MAX_LENGTH]; //input arg
    double BER; //input arg

    char buffer[BUFFER_SIZE];
    
    unsigned char packet[PACKET_SIZE + 1];
    unsigned char ack[ACK_SIZE];
    int acks[MAX_PACKETS];;
    
    int num_seq = 1000;
    int rec;
    double time_1 = 0.0, time_2;
    int i;
    
    int count = 0;
    double cwnd = 1.0;
    int left_bound_cwnd = num_seq;
    double ssthresh = 16.0;
    int state = slowstart;
    int end = 0;
    unsigned int new_ack = 0;

    srand(0);
  
    //check for correct arguments
    if(argc == 4)
    {
        strcpy(address, argv[1]);
        strcpy(filename, argv[2]);
        BER = atof(argv[3]);
    }
    else
    {
        fprintf(stderr, "%s <client_IP> <filename> <BER>\n", argv[0]);
        return EXIT_FAILURE;
    }

    c_read(filename, buffer);

    c_socket(&s_socket);
 
    // Accepting connections from the receiver
    client_len = sizeof(client);
    if((r_socket = accept(s_socket, (struct sockaddr *)&client, &client_len)) < 0)
    {
        perror("Accepting connection failure");
        return EXIT_FAILURE;
    }
    
    for(i = 0; i < MAX_PACKETS; i++)
        acks[i] = -1;

    int print = 0;

    while(end != 2) //loop over the entire input string
    {
        if(end)
            num_seq = 0;

        if(!print){
            printf("sn: %d, left_bound: %d, cwnd: %d\n", num_seq, left_bound_cwnd,(int) cwnd);
            print = 1;
        }

        if(end != 2
           && acks[num_seq - 1000] < 0
           && num_seq <= left_bound_cwnd + cwnd - 1){
            time_1 = clock() * 1000 / CLOCKS_PER_SEC;
            c_write(r_socket, num_seq, packet, buffer, BER, &end);
            acks[num_seq - 1000] = 0;
            num_seq++;
        }

        rec = recv(r_socket, ack, ACK_SIZE, MSG_DONTWAIT);
        time_2 = clock() * 1000 / CLOCKS_PER_SEC;

        if(rec == 0) //error
        {
            end = 2;
            break;
        }
        else if(rec == ACK_SIZE) //ack
        {
            left_bound_cwnd = num_seq;
            print = 0;
            
            time_2 = 0;
            
            new_ack = (ack[1] | ack[0] << 8);
            
            printf("ACK: %d\n", new_ack);
            new_ack--;
            
            acks[new_ack - 1000]++;

            if(new_ack <= 0){
                end = 2;
                break;
            }

            if(acks[new_ack - 1000] < 3) // non dup ack
            {
                if(state == slowstart)
                {
                    cwnd += 1.0;

                    if(cwnd >= ssthresh)
                        state = congestionavoidance;
                }
                else if(state == congestionavoidance)
                {
                    cwnd += 1 / (floor(cwnd));
                }
            }
            else //dup ack
            {
                printf("3rd dup ack %d!\n", new_ack + 1);
                
                //retransmit
                num_seq = new_ack + 1;
                acks[num_seq - 1000] = -1;
                acks[new_ack - 1000] = -1;

                state = slowstart;
                ssthresh = cwnd / 2;
                cwnd = 1.0;
            }

            if(time_2 >= time_1 + TIMEOUT * 1000)
            {
                printf("timeout!\n");

                acks[num_seq - 1000] = -1;

                state = slowstart;
                ssthresh = cwnd / 2;
                cwnd = 1.0;

            }
        }

        count++;
    }

    c_close(s_socket, r_socket);

    return EXIT_SUCCESS;
} //end of main
