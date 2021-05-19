#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "dir.h"
#include "usage.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#define PATH_MAX 256
void *interact(void *);
int cases(char* command);
int am_logged_in = 0;

int main(int argc, char **argv) {

    // This is the main program for the thread version of nc

    int i;

    // Check the command line arguments
    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }
    int port_value = 0;
    port_value = atoi(argv[1]);

    // The port value must be between >= 1024 and <= 65535
    if (port_value < 1024 || port_value > 65535) {
        usage(argv[0]);
        return -1;
    }

    // Create a TCP socket
/*    char *message = "OPENING SOCKET\n";*/
    printf("OPENING SOCKET\n");

    int socketd = socket(PF_INET, SOCK_STREAM, 0);
        if (socketd < 0)
        {
            perror("Failed to create the socket. ERROR.");
            exit(-1);
        }

        // Reuse the address
        int value = 1;
        if (setsockopt(socketd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) != 0)
        {
            perror("Failed to set the socket option");
            exit(-1);
        }


        // Bind the socket to a port
        struct sockaddr_in address;

        bzero(&address, sizeof(struct sockaddr_in));

        address.sin_family = AF_INET;
        address.sin_port = htons(port_value);
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        printf("BINDING TO A PORT... %d\n", port_value);
        if (bind(socketd, (const struct sockaddr*) &address, sizeof(struct sockaddr_in)) != 0)
        {
            perror("Failed to bind the socket. ERROR.");
            exit(-1);
        }

        // Set the socket to listen for connections
        printf("STARTED LISTENING...\n");
        if (listen(socketd, 4) != 0)
        {
            perror("Failed to listen for connections. ERROR.");
            exit(-1);
        }

    while (true)
    {
        // Accept the connection
        struct sockaddr_in clientAddress;

        socklen_t clientAddressLength = sizeof(struct sockaddr_in);

        printf("Waiting for incomming connections...\n");
        int clientd = accept(socketd, (struct sockaddr*) &clientAddress, &clientAddressLength);
        if (clientd < 0)
        {
            perror("Failed to accept the client connection");
            continue;
        }

        printf("Accepted the client connection from %s:%d.\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));


        // Create a separate thread to interact with the client
        pthread_t thread;
        if (pthread_create(&thread, NULL, interact, &clientd) != 0)
        {
            perror("Failed to create the thread");
            continue;
        }

        // The main thread just waits until the interaction is done
        pthread_join(thread, NULL);
        printf("Interaction thread has finished. Closing connection\n");
    }


    // This is how to call the function in dir.c to get a listing of a directory.
    // It requires a file descriptor, so in your code you would pass in the file descriptor
    // returned for the ftp server's data connection

    printf("Printed %d directory entries\n", listFiles(1, "."));
    return 0;
}

void* interact(void* args)
{
  int clientd = *(int*) args;
  char *message, *command, arg;
  int am_logged_in = 0, ascii_type_check = 0, steam_mode = 0, file_structure_mode = 0, retrieve_mode = 0, passive_mode = 0, passive_port = 0, count = 0;
  char buffer[1024];
  char *passive_char, *lengths, separate;
  char *r_dir;
  int ip1,ip2,ip3,ip4, port1, port2, port_number, socketd1;

  int new_clientd;
  struct sockaddr_in clientd_addr;
  socklen_t clientd_len;
  clientd_len = sizeof(clientd_addr);


  struct sockaddr_in server_address;
  socklen_t address1_length;
  int server_address_length = sizeof(server_address);

    message = "220 - Welcome User. Please input username. NOTE: This server only supports username: cs317\n";
    write(clientd, message, strlen(message));

    // Interact with the client
    while (true)
    {
        bzero(buffer, 1024);
        // Receive the client message
        ssize_t length = recv(clientd, buffer, 1024, 0);
        if (length < 0)
        {
            perror("Failed to read from the socket");
            break;
        }

        if (length == 0)
        {
            printf("EOF\n");
            break;
        }

        // getting rid of the white spaces and get the first argument
        char* clean_msg = strtok(buffer, "\n");
        char* command = strtok(clean_msg, " ");
        char* arg = strtok(NULL, " ");

        int exp;
        exp = cases(command);


        switch(exp) {
            // CASE: USER
            case 0:
                if (am_logged_in == 0 && arg != NULL) {
                    if (strcasecmp(arg, "cs317") == 0) {
                                am_logged_in = 1;
                                message = "230 - Login Successful. Proceed\n";
                                write(clientd, message, strlen(message));

                                char cwd[PATH_MAX];
                                if (getcwd(cwd, sizeof(cwd)) != NULL) {
                                    r_dir = cwd;
                                    length = snprintf(command, 1024, "Current working directory is: %s\n", r_dir);
                                } else {
                                    length = snprintf(command, 1024, "Error with gcwd\n");
                                    continue;
                                }
                            } else {
                               length = snprintf(command, 1024, "Server: %s\r\n", "430 - Needs to be cs317.\n");
                    }
            } else {
                length = snprintf(command, 1024, "Server: %s\r\n", "430 - Already logged in. Please use other command.\n");
            }


                break;


            // CASE: QUIT
            case 1:
                if (am_logged_in == 1 && arg == NULL) {
                        message = "221 User has quit. Terminating Connection\n";
                        write(clientd, message, strlen(message));
                        close(clientd);
                        break;
                        }

                break;


            // CASE: CWD
            case 2:
                if (am_logged_in == 1) {
                    if (arg != NULL) {
                        if (strcasecmp(arg, "..") == 0 || strcasecmp(arg, ".") == 0) {
                            length = snprintf(command, 1024, "550 - Can't accept command that starts with ./ or ../.\n");
                        } else {
                            char temp[PATH_MAX];
                            getcwd(temp, sizeof(temp));
                            strcat(temp, "/");
                            strcat(temp, arg);
                            if (chdir(temp) == -1) {
                                length = snprintf(command, 1024, "550 - Error. File does not exist\n");
                            } else {
                                length = snprintf(command, 1024, "250 - Changed Directory.\n");
                            }
                        }
                    }
                }
                break;

            // CASE: CDUP
            case 3:
                if (am_logged_in == 1) {
                    char c_dir[PATH_MAX];
                    getcwd(c_dir, PATH_MAX);
                    if (strcasecmp(c_dir, r_dir) == 0) {
                        length = snprintf(command, 1024, "550 - Not permitted to access parent of root directory. \n");
                    } else {
                            char temp[PATH_MAX];
                            char *last = strrchr(c_dir, '/');
                            strncpy(temp, c_dir, strlen(c_dir) - strlen(last));
                            chdir(temp);
                            length = snprintf(command, 1024, "250 - That is permitted.\n");
                    }
                }

                break;


            // CASE: TYPE
            case 4:
                if (am_logged_in == 1 && command != NULL) {
                    if (strcasecmp(command, "A") == 0) {
                        if (ascii_type_check == 0) {
                            ascii_type_check = 1;
                            length = snprintf(command, 1024, "200 - Permitted. Switching to ASCII.\n");
                        } else {
                            length = snprintf(command, 1024, "200 - Already in ASCII.\n");
                        }
                    } else {
                        if (strcasecmp(command, "I") == 0) {
                            if (ascii_type_check == 1) {
                                ascii_type_check = 0;
                                length = snprintf(command, 1024, "200 - Permitted. Switching to Image.\n");
                            } else {
                                length = snprintf(command, 1024, "200 - Already in image.\n");
                            }
                        }
                    }
                } else {
                    length = snprintf(command, 1024, "504 - Only supports TYPE A and TYPE I\n");
                }

                break;


            // CASE: NLST
            case 5:
                if (am_logged_in == 1 && arg == NULL) {
                    if (passive_mode == 0) {
                        length = snprintf(command, 1024, "425 - Need passive mode first.\n");
                    } else {
                        struct sockaddr_in clientAddress1;
                        socklen_t clientAddressLength1 = sizeof(struct sockaddr_in);
                        new_clientd = accept(socketd1, (struct sockaddr *)&clientAddress1,&clientAddressLength1);
                        char cwd[PATH_MAX];
                        getcwd(cwd, sizeof(cwd));
                        listFiles(new_clientd, cwd);

                        length = snprintf(command, 1024, "Here are the lists.\n");

                        close(socketd1);
                        close(new_clientd);
                        passive_mode = 0;
                    }
                }
                break;

                // CASE: MODE
                case 6:
                if (am_logged_in == 1) {
                    if (arg == NULL) {
                         length = snprintf(command, 1024, "500 - Can't be null.\n");
                    } else {
                        if (strcasecmp(arg,"s") == 0) {
                            if (steam_mode == 0) {
                                steam_mode = 1;
                                length = snprintf(command, 1024, "200 - Entering Steam mode.\n");
                            } else {
                                length = snprintf(command, 1024, "200 - Already in Steam mode.\n");
                            }
                        } else {
                            length = snprintf(command, 1024, "504 - Only supports Steam mode.\n");
                        }
                    }
                }

                break;


                // CASE: STRU
                case 7:
                if (am_logged_in == 1) {
                    if (arg == NULL) {
                        length = snprintf(command, 1024, "500 - Can't be null.\n");
                    } else {
                        if (strcasecmp(arg,"f") == 0) {
                            if (file_structure_mode == 0) {
                                file_structure_mode = 1;
                                length = snprintf(command, 1024, "200 - Data Structure set to File Structure Type.\n");
                            } else {
                                  length = snprintf(command, 1024, "200 - Already in File Structure Type.\n");
                              }
                        }  else {
                            length = snprintf(command, 1024, "500 - Only support File Structure Type.\n");
                        }
                    }

                }

                break;

                // CASE: PASV
                case 8:
                if (am_logged_in == 1 && arg == NULL) {
                    if (passive_mode == 0) {
                    socketd1 = socket(PF_INET, SOCK_STREAM, 0);

                    struct sockaddr_in address1;
                    bzero(&address1, sizeof(struct sockaddr_in));
                    address1.sin_family = AF_INET;
                    int port = 4204;
                    address1.sin_port = htons(port);
                    address1.sin_addr.s_addr = INADDR_ANY;
                         if (bind(socketd1, (const struct sockaddr*) &address1, sizeof(struct sockaddr_in)) != 0) {
                             length = snprintf(command, 1024, "Failed to bind the socket. ERROR.\n");
                         } else {
                                if (socketd1 < 0) {
                                length = snprintf(command, 1024, "500 - Error. Something Went Wrong With Passive Mode.\n");
                                } else {
                                  listen(socketd1,4);
                                  passive_mode = 1;

                                  address1_length = sizeof(address1);
                                  getsockname(socketd1, (struct sockaddr *) &address1, &address1_length);
//                                  port_number = (int) ntohs(address1.sin_port);

                                  port1 = port / 256;
                                  port2 = port % 256;

                                  passive_char = inet_ntoa(address1.sin_addr);

                                  lengths = strtok(passive_char, ".\r\n");
                                  while (count < 4) {
                                    if (lengths != NULL) {
                                        ip1 = atoi(lengths);
                                        count++;
                                        lengths = strtok(passive_char, ".\r\n");
                                    }
                                    if (lengths != NULL) {
                                        ip2 = atoi(lengths);
                                        count++;
                                        lengths = strtok(passive_char, ".\r\n");
                                    }
                                    if (lengths != NULL) {
                                        ip3 = atoi(lengths);
                                        count++;
                                        lengths = strtok(passive_char, ".\r\n");
                                    }
                                    if (lengths != NULL) {
                                        ip4 = atoi(lengths);
                                        count++;
                                        lengths = strtok(passive_char, ".\r\n");
                                    }
                                 }
                                       length = snprintf(command, 1024, "227 - Entering Passive Mode(%d,%d,%d,%d,%d,%d)\n", ip1,ip2,ip3,ip4,port1,port2);
                               }
                             }
                         } else {
                                 length = snprintf(command, 1024, "227 - Error. Already in Passive Mode.\n");
                     }
                }
                break;

                // CASE: RETR
                case 9:
                if (am_logged_in == 1) {
                    length = snprintf(command, 1024, "550 - %s.\n", arg);
                    if (arg == NULL) {
                        length = snprintf(command, 1024, "550 - Requested action not taken. Failed to open file.\n");
                    } else {
                        if (passive_mode == 0) {
                            length = snprintf(command, 1024, "425 - Please use PASV first.\n");
                        } else {
                             // open file for read
                             int read_amount = 0;
                             char file_path[PATH_MAX];
                             char file_buffer[512];
                             getcwd(file_path,PATH_MAX);
                             strcat(file_path, "/");
                             strcat(file_path, arg);

                             FILE *file_read = fopen(file_path, "r");
                             if (file_read == NULL) {
                                bzero(file_buffer, 512);
//                                length = snprintf(command, 1024, "550 - File not found.\n");
                             } else {
                                 struct sockaddr_in clientAddress1;
                                 socklen_t clientAddressLength1 = sizeof(struct sockaddr_in);
                                 new_clientd = accept(socketd1, (struct sockaddr *)&clientAddress1,&clientAddressLength1);
                                  // seek back to beginning of file
                                  fseek(file_read, 0, SEEK_SET);
                                  length = snprintf(command, 1024, " Error.\n");

                                  while ((read_amount = fread(file_buffer, sizeof(char), 512, file_read)) > 0) {
                                     int written_size = write(clientd,file_buffer, read_amount);
                                     if (written_size < 0) {
                                         length = snprintf(command, 1024, " Error.\n");
                                     } else {
                                     length = snprintf(command, 1024, " Erro12r.\n");
                                         bzero(file_buffer, 512);
                                     }
                                  }
                                  length = snprintf(command, 1024, "226 - Transfer was complete.\n");
                             }
                             fclose(file_read);
                             close(socketd1);
                             close(new_clientd);
                             passive_mode = 0;
                        }
                    }
                }

                    break;

                  case 10:
                  length = snprintf(command, 1024, "500 - ERROR. Command not recognized.\n");
                  break;



        }

        // Do something to the client message
        //length = snprintf(buffer, 1024, "Server: %s\r\n", str);

        // Send the server response
        if (send(clientd, buffer, length, 0) != length)
        {
            perror("Failed to send to the socket");
            break;
        }
    }

    close(clientd);

    return NULL;
}


// helper function to convert command into the right cases
int cases(char* command) {
    if (strcasecmp(command, "user") ==0) {
        return 0;
    } else {
        if (strcasecmp(command, "quit") ==0) {
            return 1;
        } else {
            if (strcasecmp(command, "cwd") ==0) {
                return 2;
            } else {
                if (strcasecmp(command, "cdup") ==0) {
                    return 3;
                } else {
                    if (strcasecmp(command, "type") ==0) {
                        return 4;
                    } else {
                        if (strcasecmp(command, "nlst") ==0) {
                            return 5;
                        } else {
                            if (strcasecmp(command, "mode") ==0) {
                                return 6;
                            } else {
                                if (strcasecmp(command, "stru") ==0) {
                                    return 7;
                                } else {
                                    if (strcasecmp(command, "pasv") ==0) {
                                        return 8;
                                    } else {
                                        if (strcasecmp(command, "retr") ==0) {
                                            return 9;
                                        } else {
                                            return 10;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
