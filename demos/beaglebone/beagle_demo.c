/*
 *  beagle_demo.c
 *
 *  Copyright 2013 Michael Zillgith
 *
 *  This file is part of libIEC61850.
 *
 *  libIEC61850 is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libIEC61850 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  See COPYING file for the complete license text.
 */

#include "iec61850_server.h"
#include "iso_server.h"
#include "acse.h"
#include "thread.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "static_model.h"

#include "beaglebone_leds.h"

/* import IEC 61850 device model created from SCL-File */
extern IedModel iedModel;

static int running = 0;
static IedServer iedServer = NULL;

static bool automaticOperationMode = true;
static ClientConnection controllingClient = NULL;

void sigint_handler(int signalId)
{
	running = 0;
}

void
connectionIndicationHandler(IedServer server, ClientConnection connection, bool connected, void* parameter)
{
    char* clientAddress = ClientConnection_getPeerAddress(connection);

    if (connected) {
        printf("BeagleDemoServer: new client connection from %s\n", clientAddress);
    }
    else {
        printf("BeagleDemoServer: client connection from %s closed\n", clientAddress);

        if (controllingClient == connection) {
            printf("Controlling client has closed connection -> switch to automatic operation mode\n");
            controllingClient = NULL;
            automaticOperationMode = true;
        }
    }
}

static bool
performCheckHandler(void* parameter, MmsValue* ctlVal, bool test, bool interlockCheck, ClientConnection connection)
{
    if (controllingClient == NULL) {
        printf("Client takes control -> switch to remote control operation mode\n");
        controllingClient = connection;
        automaticOperationMode = false;
    }

    /* If there is already another client that controls the device reject the control attempt */
    if (controllingClient == connection)
        return true;
    else
        return false;
}

void
updateLED1stVal(MmsValue* value, MmsValue* timeStamp) {
    if (MmsValue_getBoolean(value))
        switchLED(LED1, 1);
    else
        switchLED(LED1, 0);

    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO1_stVal, value);
    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO1_t, timeStamp);
}

void
updateLED2stVal(MmsValue* value, MmsValue* timeStamp) {
    if (MmsValue_getBoolean(value))
        switchLED(LED2, 1);
    else
        switchLED(LED2, 0);

    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO2_stVal, value);
    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO2_t, timeStamp);
}

void
updateLED3stVal(MmsValue* value, MmsValue* timeStamp) {
    if (MmsValue_getBoolean(value))
        switchLED(LED3, 1);
    else
        switchLED(LED3, 0);

    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO3_stVal, value);
    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO3_t, timeStamp);
}

void
controlHandler(void* parameter, MmsValue* value, bool test)
{
    MmsValue* timeStamp = MmsValue_newUtcTimeByMsTime(Hal_getTimeInMs());

    if (parameter == IEDMODEL_GenericIO_GGIO1_SPCSO1) {
        if (MmsValue_getType(value) == MMS_BOOLEAN) {
            updateLED1stVal(value, timeStamp);
        }

    }

    if (parameter == IEDMODEL_GenericIO_GGIO1_SPCSO2) {
        if (MmsValue_getType(value) == MMS_BOOLEAN) {
            updateLED2stVal(value, timeStamp);
        }
    }

    if (parameter == IEDMODEL_GenericIO_GGIO1_SPCSO3) {
        if (MmsValue_getType(value), MMS_BOOLEAN) {
           updateLED3stVal(value, timeStamp);
        }
    }

    if (parameter == IEDMODEL_GenericIO_GGIO1_DPCSO1) { /* example for Double Point Control - DPC */
        if (MmsValue_getType(value) == MMS_BOOLEAN) {

            MmsValue* stVal = IedServer_getAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_DPCSO1_stVal);
            MmsValue_setBitStringFromInteger(stVal, 0); /* DPC_STATE_INTERMEDIATE */

            IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_DPCSO1_stVal, stVal);
            IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_DPCSO1_t, timeStamp);

            if (MmsValue_getBoolean(value)) {
                flashLED(LED4);
                Thread_sleep(3000);
                switchLED(LED4, 1);
                MmsValue_setBitStringFromInteger(stVal, 2); /* DPC_STATE_ON */
            }
            else {
                flashLED(LED4);
                Thread_sleep(3000);
                switchLED(LED4, 0);
                MmsValue_setBitStringFromInteger(stVal, 1); /* DPC_STATE_OFF */
            }

            IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_DPCSO1_stVal, stVal);
            IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_DPCSO1_t, timeStamp);
        }
    }

    MmsValue_delete(timeStamp);
}


int main(int argc, char** argv) {

    initLEDs();

	iedServer = IedServer_create(&iedModel);

	/* Set callback handlers */
	IedServer_setConnectionIndicationHandler(iedServer, (IedConnectionIndicationHandler) connectionIndicationHandler, NULL);

	IedServer_setPerformCheckHandler(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO1,
	        (ControlPerformCheckHandler) performCheckHandler, IEDMODEL_GenericIO_GGIO1_SPCSO1);

	IedServer_setControlHandler(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO1, (ControlHandler) controlHandler,
	        IEDMODEL_GenericIO_GGIO1_SPCSO1);

    IedServer_setPerformCheckHandler(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO2,
            (ControlPerformCheckHandler) performCheckHandler, IEDMODEL_GenericIO_GGIO1_SPCSO2);

	IedServer_setControlHandler(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO2, (ControlHandler) controlHandler,
	            IEDMODEL_GenericIO_GGIO1_SPCSO2);

    IedServer_setPerformCheckHandler(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO3,
            (ControlPerformCheckHandler) performCheckHandler, IEDMODEL_GenericIO_GGIO1_SPCSO3);

	IedServer_setControlHandler(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO3, (ControlHandler) controlHandler,
	            IEDMODEL_GenericIO_GGIO1_SPCSO3);

    IedServer_setPerformCheckHandler(iedServer, IEDMODEL_GenericIO_GGIO1_DPCSO1,
            (ControlPerformCheckHandler) performCheckHandler, IEDMODEL_GenericIO_GGIO1_DPCSO1);

	IedServer_setControlHandler(iedServer, IEDMODEL_GenericIO_GGIO1_DPCSO1, (ControlHandler) controlHandler,
	            IEDMODEL_GenericIO_GGIO1_DPCSO1);


	/* Initialize process values */

	MmsValue* DPCSO1_stVal = IedServer_getAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_DPCSO1_stVal);
	MmsValue_setBitStringFromInteger(DPCSO1_stVal, 1); /* set DPC to OFF */

    /* MMS server will be instructed to start listening to client connections. */
    IedServer_start(iedServer, 102);

	if (!IedServer_isRunning(iedServer)) {
		printf("Starting server failed! Exit.\n");
		IedServer_destroy(iedServer);
		exit(-1);
	}

	running = 1;

	signal(SIGINT, sigint_handler);

	MmsValue* timestamp = MmsValue_newUtcTime(0);

	float t = 0.f;

	MmsValue* an1 = MmsValue_newFloat(0.f);
	MmsValue* an2 = MmsValue_newFloat(0.f);
	MmsValue* an3 = MmsValue_newFloat(0.f);
	MmsValue* an4 = MmsValue_newFloat(0.f);

	MmsValue* ledStVal = MmsValue_newBoolean(false);
	uint64_t nextLedToggleTime = Hal_getTimeInMs() + 1000;

	while (running) {
	    uint64_t currentTime = Hal_getTimeInMs();

	    MmsValue_setUtcTimeMs(timestamp, currentTime);

	    if (automaticOperationMode) {
	        if (nextLedToggleTime <= currentTime) {
	            nextLedToggleTime = currentTime + 1000;

	            if (MmsValue_getBoolean(ledStVal))
	                MmsValue_setBoolean(ledStVal, false);
	            else
	                MmsValue_setBoolean(ledStVal, true);

	            updateLED1stVal(ledStVal, timestamp);
	            updateLED2stVal(ledStVal, timestamp);
	            updateLED3stVal(ledStVal, timestamp);
	        }
	    }


	    t += 0.1f;

	    MmsValue_setFloat(an1, sinf(t));
	    MmsValue_setFloat(an2, sinf(t + 1.f));
	    MmsValue_setFloat(an3, sinf(t + 2.f));
	    MmsValue_setFloat(an4, sinf(t + 3.f));

	    IedServer_lockDataModel(iedServer);

	    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_AnIn1_mag_f, an1);
	    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_AnIn1_t, timestamp);
	    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_AnIn2_mag_f, an2);
	    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_AnIn2_t, timestamp);
	    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_AnIn3_mag_f, an3);
	    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_AnIn3_t, timestamp);
	    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_AnIn4_mag_f, an4);
	    IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_AnIn4_t, timestamp);

	    IedServer_unlockDataModel(iedServer);

		Thread_sleep(100);
	}

	/* stop MMS server - close TCP server socket and all client sockets */
	IedServer_stop(iedServer);

	/* Cleanup - free all resources */
	MmsValue_delete(an1);
	MmsValue_delete(an2);
	MmsValue_delete(an3);
	MmsValue_delete(an4);
	MmsValue_delete(ledStVal);
	MmsValue_delete(timestamp);

	IedServer_destroy(iedServer);
} /* main() */
