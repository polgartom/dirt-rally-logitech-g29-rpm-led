#include "base.h"

int server_listen()
{
     WSADATA wsaData;
     SOCKET ReceivingSocket;
     SOCKADDR_IN ReceiverAddr;
     int Port = 10001;
     char ReceiveBuf[1024];
     int BufLength = 1024;
     SOCKADDR_IN SenderAddr;
     int SenderAddrSize = sizeof(SenderAddr);
     int ByteReceived = 5;

     float write_freq_sec = 0.5f;
     double lastwrite_time = 0.0;
     unsigned int lastwrite_time_d = 0;

     FILE *raw = NULL;
     FILE *csv = NULL;

     // Initialize Winsock version 2.2

     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
     {

          printf("Server: WSAStartup failed with error: %ld\n", WSAGetLastError());

          return -1;
     }
     else
     {
          printf("Server: The Winsock DLL status is: %s.\n", wsaData.szSystemStatus);
     }

     // Create a new socket to receive datagrams on.

     ReceivingSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

     if (ReceivingSocket == INVALID_SOCKET)
     {

          // Print error message
          printf("Server: Error at socket(): %ld\n", WSAGetLastError());

          // Clean up
          WSACleanup();

          // Exit with error
          return -1;
     }
     else
     {
          printf("Server: socket() is OK!\n");
     }

     /*Set up a SOCKADDR_IN structure that will tell bind that
     we want to receive datagrams from all interfaces using port 5150.*/

     // The IPv4 family
     ReceiverAddr.sin_family = AF_INET;

     // Port no. (8888)
     ReceiverAddr.sin_port = htons(Port);

     // From all interface (0.0.0.0)
     ReceiverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

     // Associate the address information with the socket using bind.

     // At this point you can receive datagrams on your bound socket.

     struct timeval tv = {.tv_sec=50, .tv_usec=0};
     if (setsockopt(ReceivingSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv)) == SOCKET_ERROR) {
          perror("Error");
          return -1;
     }

     if (bind(ReceivingSocket, (SOCKADDR *)&ReceiverAddr, sizeof(ReceiverAddr)) == SOCKET_ERROR)
     {

          // Print error message
          printf("Server: Error! bind() failed!\n");

          // Close the socket
          closesocket(ReceivingSocket);

          // Do the clean up
          WSACleanup();

          // and exit with error
          return -1;
     }

     printf("Server: bind() is OK!\n");

     // Print some info on the receiver(Server) side...
     getsockname(ReceivingSocket, (SOCKADDR *)&ReceiverAddr, (int *)sizeof(ReceiverAddr));

     printf("Server: Receiving IP(s) used: %s\n", inet_ntoa(ReceiverAddr.sin_addr));

     printf("Server: Receiving port used: %d\n", htons(ReceiverAddr.sin_port));

     printf("Server: I\'m ready to receive data packages. Waiting...\n\n");

     size_t num_timeouts = 0;

     if (telementry.format & CSV) {
          char fname[MAX_PATH] = {0};
          int len = sprintf_s(fname, MAX_PATH, "%s\\csv_%d.csv", telementry.directory, (int)time(NULL));
          if (len <= 5) {
               perror("Failed to make fname string");
               return 1;
          }

          csv = fopen_mkdir(fname, "w");
          if (csv == NULL) {
               perror("Error opening file");
               return 1;
          }

          fwrite_telementry_csv_headers(csv);

          printf("Write CSV telementry to: %s\n", fname);
     }

     if (telementry.format & RAW) {
          char fname[MAX_PATH] = {0};
          int len = sprintf_s(fname, MAX_PATH, "%s\\raw_%d.bin", telementry.directory, (int)time(NULL));
          if (len <= 5) {
               perror("Failed to make fname string");
               return 1;
          }

          raw = fopen_mkdir(fname, "wb");
          if (raw == NULL) {
               perror("Error opening file");
               return 1;
          }

          printf("Write RAW telementry to: %s\n", fname);
     }

     // At this point you can receive datagrams on your bound socket.
     while (1)
     { // Server is receiving data until you will close it.(You can replace while(1) with a condition to stop receiving.)

          ByteReceived = recvfrom(ReceivingSocket, ReceiveBuf, BufLength, 0, (SOCKADDR *)&SenderAddr, &SenderAddrSize);

          if (ByteReceived >= 0) { 
               // printf("Server: Total Bytes received: %d\n", ByteReceived);
               //   printf("Server: The data is: %s\n", ReceiveBuf);
               // printf("\n");

               if (ByteReceived == 264) {
                    float *fdata = (float*)&ReceiveBuf[0];

                    float rpm = fdata[DRT1_RPM];
                    float maxrpm = fdata[DRT1_MAX_RPM];
                    float minrpm = fdata[DRT1_MIN_RPM];

                    if (maxrpm > 0.0 && minrpm > 0.0) {
                         double delta_time = (double)fdata[0] - lastwrite_time;
                         if (delta_time > write_freq_sec) {
                              lastwrite_time = (double)fdata[0];

                              if (telementry.format & CSV) {
                                   fwrite_telementry_csv_packet(csv, fdata);
                              }
                              if (telementry.format & RAW) {
                                   fwrite(fdata, 264, 1, raw);
                              }
                              if (telementry.format != NONE) {
                                   // printf("write! lastwrite: %f ; delta: %f\n", lastwrite_time, delta_time);
                              }
                         }

                         num_timeouts = 0;

                         rpm = max(rpm - minrpm, 0.0);
                         maxrpm = max(maxrpm - minrpm, 1.0);
                         float percent = rpm / maxrpm * 100;
                         unsigned char led_bits = map_rpm_percent_to_led_bits(percent);

                         // printf("\n-----------------------------------\n");
                         // float time = fdata[0];
                         // printf("%f", fdata[0], fdata[1]);
                         // fprintf(file, "%f ", fdata[1]);
                         // fwrite(fdata, sizeof(ReceiveBuf), 1, file);
                         // fprintf(file, "\n\n");
                         // printf("> r: %f (%f%%) ; m: %f ; l: %f | bits: %d | time: %f\n", rpm, percent, maxrpm, minrpm, led_bits);

                         set_wheel_leds(curr_wheel, led_bits);
                    } else {
                         num_timeouts += 1;
                    }

                    // for (int i = 0; i <= 65; i++) {
                    //      float val = fdata[i];
                    //      printf("#(%d, %d) %f\n", i, 4*i, val);
                    // }
               }
          } else { // ByteReceived == SOCKET_ERROR (-1)
               int err = WSAGetLastError();
               // printf("Server: recvfrom() failed with error code: %d\n", err);

               num_timeouts = min(num_timeouts+1, 51);

               if (err == WSAETIMEDOUT) {
                    
                    // printf("Sleep ms: %d\n", 1000);
                    if (num_timeouts > 50) {
                         // printf("Server: No data received in 5 seconds. Turning off LEDs.\n");
                         // Sleep(1000);
                    }

                    if (curr_wheel) {
                         set_wheel_leds(curr_wheel, 0);
                    }

               } else {
                    perror("Error");
                    printf("Server: recvfrom() failed with error code: %d\n", err);
               }
          }

          // Sleep(25);
          // size_t sleepms = num_timeouts > 50 ? 1000 : 100;
     }

     if (csv != NULL) {
          fclose(csv);
     }
     if (raw != NULL) {
          fclose(raw);
     }

     // Print some info on the sender(Client) side...
     getpeername(ReceivingSocket, (SOCKADDR *)&SenderAddr, &SenderAddrSize);
     printf("Server: Sending IP used: %s\n", inet_ntoa(SenderAddr.sin_addr));
     printf("Server: Sending port used: %d\n", htons(SenderAddr.sin_port));

     // When your application is finished receiving datagrams close the socket.
     printf("Server: Finished receiving. Closing the listening socket...\n");
     if (closesocket(ReceivingSocket) != 0)
     {
          printf("Server: closesocket() failed! Error code: %ld\n", WSAGetLastError());
     }
     else
     {
          printf("Server: closesocket() is OK\n");
     }

     // When your application is finished call WSACleanup.
     printf("Server: Cleaning up...\n");

     if (WSACleanup() != 0)
     {
          printf("Server: WSACleanup() failed! Error code: %ld\n", WSAGetLastError());
     }
     else
     {
          printf("Server: WSACleanup() is OK\n");
     }

     if (curr_wheel) {
          set_wheel_leds(curr_wheel, 0);
     }

     return 0;
}

int main(int argc, char **argv)
{
     init_logi_g29_wheel_hid();
     if (curr_wheel == NULL) {
          return 1;
     }
     set_wheel_leds(curr_wheel, 0);

     int success = -1;
     if (argc > 1 && strlen(argv[1]) > 4 /*.ini*/) {
          success = parse_settings_ini_file(argv[1]);
     }
     if (success != 0) {
          success = parse_settings_ini_file("settings.ini");
     }

     int err = server_listen();

     hid_close(curr_wheel);
     hid_exit(); /* Free static HIDAPI objects. */

     return err;
}
