/*
 *  server_example3.c
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

/* import IEC 61850 device model created from SCL-File */
extern IedModel iedModel;

static int running = 0;
static IedServer iedServer = NULL;

void sigint_handler(int signalId)
{
	running = 0;
}

void
controlHandler(void* parameter, MmsValue* value, bool test)
{
    printf("received control command %i %i: ", value->type, value->value.boolean);

    if (value->value.boolean)
        printf("on\n");
    else
        printf("off\n");

    MmsValue* timeStamp = MmsValue_newUtcTimeByMsTime(Hal_getTimeInMs());

    if (parameter == IEDMODEL_GenericIO_GGIO1_SPCSO1) {
        IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO1_stVal, value);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO1_t, timeStamp);
    }

    if (parameter == IEDMODEL_GenericIO_GGIO1_SPCSO2) {
        IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO2_stVal, value);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO2_t, timeStamp);
    }

    if (parameter == IEDMODEL_GenericIO_GGIO1_SPCSO3) {
        IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO3_stVal, value);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO3_t, timeStamp);
    }

    if (parameter == IEDMODEL_GenericIO_GGIO1_SPCSO4) {
        IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO4_stVal, value);
        IedServer_updateAttributeValue(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO4_t, timeStamp);
    }

    MmsValue_delete(timeStamp);
}

int main(int argc, char** argv) {

	iedServer = IedServer_create(&iedModel);

	/* MMS server will be instructed to start listening to client connections. */
	IedServer_start(iedServer, 102);

	IedServer_setControlHandler(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO1, (ControlHandler) controlHandler,
	        IEDMODEL_GenericIO_GGIO1_SPCSO1);

	IedServer_setControlHandler(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO2, (ControlHandler) controlHandler,
	            IEDMODEL_GenericIO_GGIO1_SPCSO2);

	IedServer_setControlHandler(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO3, (ControlHandler) controlHandler,
	            IEDMODEL_GenericIO_GGIO1_SPCSO3);

	IedServer_setControlHandler(iedServer, IEDMODEL_GenericIO_GGIO1_SPCSO4, (ControlHandler) controlHandler,
	            IEDMODEL_GenericIO_GGIO1_SPCSO4);

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

	while (running) {
	    MmsValue_setUtcTimeMs(timestamp, Hal_getTimeInMs());

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
	IedServer_destroy(iedServer);
} /* main() */
