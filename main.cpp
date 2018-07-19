#include "mbed.h"
#include "FPC_R306.hpp"
#include "MQTTEthernet.h"
#include "MQTTClient.h"
#define ECHO_SERVER_PORT   7


FPC_R306 finger(D1,D0);

Serial pc(USBTX,USBRX);
int myid=456;

FILE *fp;
char userId[100];
int p;

int id =1;
int choice;
uint16_t deleteID;
uint8_t sendData[16];
uint8_t getImage(void);
uint8_t image2Tz(uint8_t slot);
int SendCommand();
int RecvResponse();

/*..........MQTT CALLBACK FUNCTION.........*/
MQTTEthernet ipstack = MQTTEthernet();

MQTT::Client<MQTTEthernet, Countdown> client = MQTT::Client<MQTTEthernet, Countdown>(ipstack);
MQTT::Message message;

void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\n", message.payloadlen, (char*)message.payload);
}

//....................................................MAIN Function.....................................................//
int main()
{
    wait_ms(500);
    pc.baud(9600);
    finger.baud(57600);

    /*****************************MQTT CONFIGURATION START FROM HERE *****************************************/

    printf("Wait a second...\r\n");
    char* topic = "openhab/parents/command";


    char* hostname = "iot.eclipse.org";
    int port = 1883;

    int rc = ipstack.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\n", rc);

    printf("Topic: %s\r\n",topic);

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = "parents";

    if ((rc = client.connect(data)) == 0)
        printf("rc from MQTT connect is %d\n", rc);


    /********************MQTT CONFIGURATION END FROM HERE********************/

    pc.printf("..................................R306.............................................\n");

    while(true) {
        pc.printf("\n");
        pc.printf("To Enroll ID-----------------------------------   1\n");

        pc.printf("To get Template Count--------------------------   2\n");

        pc.printf("To Search for an ID----------------------------   3\n");

        pc.printf("To Delete an ID--------------------------------   4\n");

        pc.printf("To Empty the library---------------------------   5\n");


        choice = pc.getc();
        choice -= '0';

//.........................................ENROLLING FINGERPRINT........................................................//

        switch(choice) {
            case 1:

                wait(1);
                p=-1;
                pc.printf("PLACE THE FINGER TO ENROLL\n");


//.............Enrolling 1st time

                while(p != FINGERPRINT_OK) {
                    pc.printf(".");
                    p = finger.CMD_GETIMG();;
                    switch (p) {
                        case FINGERPRINT_OK:
                            pc.printf("Image Taken\n");
                            break;

                        case FINGERPRINT_NOFINGER:
                            pc.printf(".");
                            wait(1);

                        case FINGERPRINT_PACKETRECIEVEERR:
                            //pc.printf("Communication error\n");
                            break;

                        case FINGERPRINT_IMAGEFAIL:
                            pc.printf("Imaging error\n");
                            break;

                        default:
                            pc.printf("Unknown Error\n");
                            break;
                    }
                }

                p= -1;
                while(p!= FINGERPRINT_OK) {
                    p = finger.CMD_IMG2Tz1();

                    switch (p) {
                        case FINGERPRINT_OK:
                            pc.printf("Image Converted\n");
                            break;

                        case FINGERPRINT_IMAGEMESS:
                            pc.printf("Image too messy\n");
                            return p;

                        case FINGERPRINT_FEATUREFAIL:
                            pc.printf("Could not find fingerprint features\n");
                            return p;

                        case FINGERPRINT_PACKETRECIEVEERR:
                            pc.printf("Communication error\n");
                            return p;

                        default:
                            pc.printf("Unknown error\n");
                            return p;
                    }
                }

                //...........Enrolling 2nd time


                pc.printf("Remove the finger\n");
                wait(1);
                p=0;

                while(p!= FINGERPRINT_NOFINGER) { //Waiting for finger to remove
                    p= finger.CMD_GETIMG();
                }
                p =-1;
                pc.printf("PLACE THE SAME FINGER AGAIN\n");

                while(p!= FINGERPRINT_OK) {
                    pc.printf(".");
                    p = finger.CMD_GETIMG();;
                    switch (p) {
                        case FINGERPRINT_OK:
                            pc.printf("Image Taken\n");
                            break;

                        case FINGERPRINT_NOFINGER:
                            pc.printf(".");
                            wait(1);

                        case FINGERPRINT_PACKETRECIEVEERR:
                            //pc.printf("Communication error\n");
                            break;

                        case FINGERPRINT_IMAGEFAIL:
                            pc.printf("Imaging error\n");
                            break;

                        default:
                            pc.printf("Unknown Error\n");
                            break;
                    }
                }

                p=-1;
                while(p!= FINGERPRINT_OK) {
                    p = finger.CMD_IMG2Tz2();

                    switch (p) {
                        case FINGERPRINT_OK:
                            pc.printf("Image Converted\n");
                            break;

                        case FINGERPRINT_IMAGEMESS:
                            pc.printf("Image too messy\n");
                            return p;

                        case FINGERPRINT_FEATUREFAIL:
                            pc.printf("Could not find fingerprint features\n");
                            return p;

                        case FINGERPRINT_PACKETRECIEVEERR:
                            pc.printf("Communication error\n");
                            return p;

                        default:
                            pc.printf("Unknown error\n");
                            return p;
                    }
                }

                //........Creating Model

                pc.printf("Creating Model\n");

                p = finger.createModel();

                if(p == FINGERPRINT_OK) {
                    pc.printf("Prints Matched\n");
                }

                else if(p == FINGERPRINT_ENROLLMISMATCH) {
                    pc.printf("Fingerprints did not match\n");
                    break;
                }

                else if (p == FINGERPRINT_PACKETRECIEVEERR) {
                    pc.printf("Communication error\n");
                    break;
                } else {
                    pc.printf("UNKNOWN ERROR\n");
                    break;
                }


//.....Storing Model

                //uint8_t test = (uint8_t)(id & 0xFF);
                pc.printf("%d\n", id);
                p = finger.STORE_MODEL(id);

                if(p == FINGERPRINT_OK) {
                    pc.printf("Stored\n");
                    id++;
                }

                else if (p == FINGERPRINT_PACKETRECIEVEERR) {
                    pc.printf("Communication error\n");
                    break;
                }

                else if (p == FINGERPRINT_BADLOCATION) {
                    pc.printf("Could not store in that location\n");
                    break;
                }

                else if (p == FINGERPRINT_FLASHERR) {
                    pc.printf("Error writing to flash\n");
                    break;
                } else {
                    pc.printf("Unknown error\n");
                    break;
                }

                break;


//.............................................Get TEMPLATE Count.......................................................//
            case 2:
                pc.printf("Getting template count\n\n");
                wait(1);
                p = -1;
                p = finger.TMPL_COUNT();

                if(p==FINGERPRINT_OK) {
                    pc.printf("SENSOR CONTAINS %d TEMPLATES",finger.templatecount);
                }

                else
                    pc.printf("ERROR\n");
                break;

//.................................................SEARCHING............................................................//

            case 3:
                p= -1;
                while(p!= FINGERPRINT_OK) {
                    p= finger.CMD_GETIMG();

                    switch (p) {
                        case FINGERPRINT_OK:
                            pc.printf("Image Taken\n");
                            break;

                        case FINGERPRINT_NOFINGER:
                            pc.printf(".");
                            wait(1);

                        case FINGERPRINT_PACKETRECIEVEERR:
                            //pc.printf("Communication error\n");
                            break;

                        case FINGERPRINT_IMAGEFAIL:
                            pc.printf("Imaging error\n");
                            break;

                        default:
                            pc.printf("Unknown Error\n");
                            break;
                    }
                }

                p = finger.CMD_IMG2Tz1();

                switch (p) {
                    case FINGERPRINT_OK:
                        pc.printf("Image Converted\n");
                        break;

                    case FINGERPRINT_IMAGEMESS:
                        pc.printf("Image too messy\n");
                        return p;

                    case FINGERPRINT_FEATUREFAIL:
                        pc.printf("Could not find fingerprint features\n");
                        return p;

                    case FINGERPRINT_PACKETRECIEVEERR:
                        pc.printf("Communication error\n");
                        return p;

                    default:
                        pc.printf("Unknown error\n");
                        return p;
                }

                p = finger.FAST_SEARCH();
                if (p == FINGERPRINT_OK) {
                    pc.printf("Found a print match!\n");
                    pc.printf("FOUND ID #%d  with confidence of %d\n", finger.fingerID, finger.MatchScore);
                    message.qos = MQTT::QOS0;
                    message.retained = false;
                    message.dup = false;
                    
                    sprintf(userId, "%d", finger.fingerID);

                    pc.printf("User id  over MQTT is : %s\n", userId);
                    message.payload =(void *)userId;
                    message.payloadlen = strlen(userId);
                    rc = client.publish("cdi/laxmi", message);
                    pc.printf("send via MQTT\n");

                }

                else if (p == FINGERPRINT_PACKETRECIEVEERR) {
                    pc.printf("Communication error\n");
                    break;
                }

                else if (p == FINGERPRINT_NOTFOUND) {
                    pc.printf("Did not find a match\n");
                    break;
                }

                else {
                    pc.printf("Unknown error");
                    break;
                }

                break;



//............................................DELETE ID................................................................//

            case 4:

                pc.printf("Please, Enter the ID to be Deleted\n");
                deleteID = pc.getc();
                deleteID -='0';
                pc.printf("Deleting the ID %d\n", deleteID);

                p= -1;
                p= finger.DELETE_ID(deleteID);

                if (p == FINGERPRINT_OK) {
                    pc.printf("Deleted!\n");
                }

                else if (p == FINGERPRINT_PACKETRECIEVEERR) {
                    pc.printf("Communication error\n");
                    return p;
                }

                else if (p == FINGERPRINT_BADLOCATION) {
                    pc.printf("Could not delete in that location\n");
                    return p;
                }

                else if (p == FINGERPRINT_FLASHERR) {
                    pc.printf("Error writing to flash\n");
                    return p;
                } else {
                    pc.printf("Unknown error\n");
                }
                break;

//................................................EMPTY LIBRARY......................................................//

            case 5:

                p= -1;
                p = finger.EMPTY_LIB();

                if (p == FINGERPRINT_OK) {
                    pc.printf("Library Cleared!\n");
                }

                else if (p == FINGERPRINT_PACKETRECIEVEERR) {
                    pc.printf("Communication error\n");
                    return p;
                }

                else if (p == FINGERPRINT_DBCLEARFAIL) {
                    pc.printf("Could not clear the library\n");
                    return p;
                }

                else {
                    pc.printf("Unknown error\n");
                }
                break;


        }
    }
}
