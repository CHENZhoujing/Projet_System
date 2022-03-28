#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <string.h>
#include <signal.h>
#include <math.h>

#define MSG_LEN = 1024

#define article "Classique"
//#define ARTICLE "Rustique"

// Web server
#define SERVER "sw1"
//#define SERVER "sw2"

// Acheteur
#define ACHETEUR "Laure"
//#define ACHETEUR "Philippe"

// Transporteurs
#define TRANSPORTEUR "Xavier"
//#define TRANSPORTEUR "France"

// Informations
#define CAPACITE_PALLET 63
#define PRIX_ROULEAU 5
#define SURFACE 40
#define SURFACE_ROULEAU 1
#define STOCK 3000

enum
{
    BUFFER_SIZE = 1024
};

typedef int Pipe[2];

Pipe serverToBuyer;
Pipe serverToCarrier;
Pipe buyerToServer;
Pipe buyerToCarrier;
Pipe carrierToBuyer;

static void echoMsg(int step, char *source, char *dest, char *msg)
{
    printf("%d [%s]\tmsg de <%s>\t%s\n", step, source, dest, msg);
}

static void echoLog(int step, char *source, char *msg)
{
    printf("%d [%s]\tlog\t\t%s\n", step, source, msg);
}

static void buyer()
{
    char buffer[BUFFER_SIZE];

    printf("0 [ach]\tdémarrage\n");

    // 1
    sprintf(buffer, article);
    write(buyerToServer[1], article, strlen(article) + 1);
    echoLog(1, "ach", buffer);

    // 2
    read(serverToBuyer[0], buffer, BUFFER_SIZE);
    echoMsg(2, "ach", "srv", buffer);

    // 3
    sprintf(buffer, "quantité souhaité %d m²", SURFACE);
    write(buyerToServer[1], buffer, strlen(buffer) + 1);

    // 4
    read(serverToBuyer[0], buffer, BUFFER_SIZE);
    echoMsg(4, "ach", "srv", buffer);

    // 5
    sprintf(buffer, "numéro carte bancaire: 1234567812345678, cryptograme: 777");
    write(buyerToServer[1], buffer, strlen(buffer) + 1);

    // 6
    read(serverToBuyer[0], buffer, BUFFER_SIZE);
    echoMsg(6, "ach", "srv", buffer);

    // 8
    read(carrierToBuyer[0], buffer, BUFFER_SIZE);
    echoMsg(8, "ach", "trp", buffer);

    // 9
    sprintf(buffer, "bon signé");
    write(buyerToCarrier[1], buffer, BUFFER_SIZE);
}

static void server()
{
    char buffer[BUFFER_SIZE];
    printf("0 [srv]\tdémarrage\n");

    // 1
    read(buyerToServer[0], buffer, BUFFER_SIZE);
    echoMsg(1, "srv", "ach", buffer);

    // 2
    sprintf(buffer, "quantité disponible de rouleaux %d", STOCK);
    write(serverToBuyer[1], buffer, strlen(buffer) + 1);
    printf("2 [srv]\tlog\t\tenvoi de la quantité en stock\n");

    // 3
    read(buyerToServer[0], buffer, BUFFER_SIZE);
    echoMsg(3, "srv", "ach", buffer);

    // 4
    int nb_palette = ceil(SURFACE / (float)CAPACITE_PALLET);
    int price = SURFACE * PRIX_ROULEAU;

    sprintf(buffer, "nombre de palette: %d, prix: %d €", nb_palette, price);
    write(serverToBuyer[1], buffer, BUFFER_SIZE);
    printf("4 [srv]\tlog\t\tenvoi du nombre de palette et du prix total\n");

    // 5
    read(buyerToServer[0], buffer, BUFFER_SIZE);
    echoMsg(5, "srv", "ach", buffer);

    // 6
    sprintf(buffer, "accusé de réception, prix: %d €", price);
    write(serverToBuyer[1], buffer, BUFFER_SIZE);

    // 7
    char bon[BUFFER_SIZE / 3];
    sprintf(bon, "article: %s, quantité: %d, nombre de palette: %d", article, SURFACE, nb_palette);
    sprintf(buffer, "bon 1: %s, bon 2: %s", bon, bon);
    write(serverToCarrier[1], buffer, BUFFER_SIZE);
}

static void carrier()
{
    char buffer[BUFFER_SIZE];
    printf("0 [trp]\tdémarrage\n");

    // 7
    read(serverToCarrier[0], buffer, BUFFER_SIZE);
    echoMsg(7, "trp", "srv", buffer);

    // 8
    char order[BUFFER_SIZE / 3];
    strncpy(order, buffer, (strlen(buffer) / 2) - 1);
    sprintf(buffer, "livraison et bon: %s", order);
    write(carrierToBuyer[1], buffer, BUFFER_SIZE);

    // 9
    read(buyerToCarrier[0], buffer, BUFFER_SIZE);
    echoMsg(9, "trp", "ach", buffer);
}

int main()
{
    // Create pipes
    pipe(serverToBuyer);
    pipe(serverToCarrier);
    pipe(buyerToServer);
    pipe(buyerToCarrier);
    pipe(carrierToBuyer);

    // Start each routine in their own process
    // and exit after
    int buyer_pid = fork();
    if (buyer_pid == 0)
    {
        // In the buyer process
        int server_pid = fork();
        if (server_pid == 0)
        {
            // In the server process
            close(serverToBuyer[0]);
            close(serverToCarrier[0]);
            server();

            int carrier_pid = fork();
            if (carrier_pid == 0)
            {
                close(carrierToBuyer[0]);
                carrier();
            }
        }
        else if (server_pid > 0)
        {
            close(buyerToServer[0]);
            close(buyerToCarrier[0]);
            buyer();
        }
    }
}
