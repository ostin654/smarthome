
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sqlite3.h> 
#include <RF24/RF24.h>

using namespace std;

const uint64_t pipe_out = 555555555;
const uint64_t pipe_in_well = 555555550;
//const uint64_t pipe_in_weather = 555555551;

typedef char request_packet;

typedef struct {
  float water_level;
} response_packet_well;

/*typedef struct {
  float temperature;
  float humidity;
  float pressure;
} response_packet_weather;*/

request_packet request = 0;
response_packet_well response_well;
//response_packet_weather response_weather;

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
   return 0;
}

int main(int argc, char** argv)
{
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    char *sql;
    char radio1, radio2; // args 1 and 2
    char channel; // arg 3
    std::time_t unixtime;

    sql = (char*)malloc(1000);

    radio1 = atoi(argv[1]);
    radio2 = atoi(argv[2]);
    channel = atoi(argv[3]);

    //debug
    //printf("%d %d %d %s\n", radio1, radio2, channel, argv[4]);

    RF24 radio(radio1, radio2);

    radio.begin();
    sleep(2);
    radio.setDataRate(RF24_1MBPS);
    radio.setCRCLength(RF24_CRC_16);
    radio.setPALevel(RF24_PA_MAX);

    radio.setChannel(channel);
    radio.setRetries(15, 15);

    radio.setAutoAck(true);
    radio.enableAckPayload();
    radio.enableDynamicPayloads();

    radio.powerUp();

    sleep(2);

    radio.openWritingPipe(pipe_out);
    radio.openReadingPipe(1,pipe_in_well);

    sleep(1);

    radio.printDetails();

    while (1) {
        /* Open database */
        rc = sqlite3_open(argv[4], &db);

        if (rc) {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            return(0);
        } else {
            fprintf(stderr, "Opened database successfully\n");
        }

        request++;

        if (radio.write(&request, sizeof(request_packet))) {
            if (!radio.available()) {
                printf("[well] Ok, but got blank response\n");
            } else {
                while (radio.available()) {
                    radio.read(&response_well, sizeof(response_packet_well));
                    printf("[well] Ok, water_level = %.2f\n", response_well.water_level);

                    unixtime = std::time(nullptr);
                    sprintf(sql, "INSERT INTO history VALUES(NULL, %ld, %.2f)", unixtime, 100*response_well.water_level/7);

                    /* Execute SQL statement */
                    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);

                    if (rc != SQLITE_OK) {
                        fprintf(stderr, "SQL error: %s\n", zErrMsg);
                        sqlite3_free(zErrMsg);
                    } else {
                        fprintf(stderr, "Records created successfully\n");
                    }
                }
            }
        } else {
            printf("[well] Failed\n\r");
        }

        sqlite3_close(db);
        delay(20000);
    }
}
