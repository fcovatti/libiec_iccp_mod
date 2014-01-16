/*


 *  mms_client_unconfirmed.c
 *
 *  Copyright 2013 Michael Zillgith
 *
 *	This file is part of libIEC61850.
 *
 *	libIEC61850 is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	libIEC61850 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with libIEC61850.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	See COPYING file for the complete license text.
 */

#include <MmsPdu.h>
#include "mms_common.h"
#include "mms_client_connection.h"
#include "byte_buffer.h"

#include "mms_client_internal.h"
#include "mms_common_internal.h"

#include "stack_config.h"

static VariableSpecification_t*
createNewDomainVariableSpecification(char* domainId, char* itemId)
{
	VariableSpecification_t* varSpec = calloc(1, sizeof(ListOfVariableSeq_t));
	varSpec->present = VariableSpecification_PR_name;
	varSpec->choice.name.present = ObjectName_PR_domainspecific;
	varSpec->choice.name.choice.domainspecific.domainId.buf = domainId;
	varSpec->choice.name.choice.domainspecific.domainId.size = strlen(domainId);
	varSpec->choice.name.choice.domainspecific.itemId.buf = itemId;
	varSpec->choice.name.choice.domainspecific.itemId.size = strlen(itemId);

	return varSpec;
}

int
mmsClient_createUnconfirmedPDU(char* domainId, char* itemId, uint32_t time_stamp,
		ByteBuffer* writeBuffer)
{
	MmsPdu_t* mmsPdu = ( MmsPdu_t*) mmsClient_createUnconfirmedResponsePdu();
	
	mmsPdu->choice.unconfirmedPDU.unconfirmedService.present = UnconfirmedService_PR_informationReport;

	InformationReport_t* report =
		                      &(mmsPdu->choice.unconfirmedPDU.unconfirmedService.choice.informationReport);


	/* Create list of variable specifications */
	report->variableAccessSpecification.present = VariableAccessSpecification_PR_listOfVariable;

	report->variableAccessSpecification.choice.listOfVariable.list.count = 2;
	report->variableAccessSpecification.choice.listOfVariable.list.size = 2;
	report->variableAccessSpecification.choice.listOfVariable.list.array = calloc(2, sizeof(ListOfVariableSeq_t*)); ;
	
	
	/*ListOfVariableSeq_t* listOfVars = calloc(1, sizeof(ListOfVariableSeq_t));
	listOfVars->alternateAccess = NULL;
	listOfVars->variableSpecification.present = VariableSpecification_PR_name;
	listOfVars->variableSpecification.choice.name.present = ObjectName_PR_domainspecific;
	listOfVars->variableSpecification.choice.name.choice.domainspecific.domainId.buf = domainId;
	listOfVars->variableSpecification.choice.name.choice.domainspecific.domainId.size = strlen(domainId);
	listOfVars->variableSpecification.choice.name.choice.domainspecific.itemId.buf = "Transfer_Report_ACK";
	listOfVars->variableSpecification.choice.name.choice.domainspecific.itemId.size = strlen("Transfer_Report_ACK");
	report->variableAccessSpecification.choice.listOfVariable.list.array[0] = listOfVars;
*/
	report->variableAccessSpecification.choice.listOfVariable.list.array[0] =(ListOfVariableSeq_t*) createNewDomainVariableSpecification(domainId, "Transfer_Report_ACK");

	report->variableAccessSpecification.choice.listOfVariable.list.array[1] = (ListOfVariableSeq_t*)
			createNewDomainVariableSpecification(domainId, "Transfer_Set_Time_Stamp");


	/* Create list of Access Result */
	report->listOfAccessResult.list.count = 2;
	report->listOfAccessResult.list.size = 2;
	report->listOfAccessResult.list.array = calloc(2, sizeof(AccessResult_t*));


	AccessResult_t *accessResult1 = calloc(1, sizeof(AccessResult_t));
	AccessResult_t *accessResult2 = calloc(1, sizeof(AccessResult_t));

	Data_t* dataArray =  calloc(3, sizeof(Data_t*));	
	report->listOfAccessResult.list.array[0] = accessResult1;
	report->listOfAccessResult.list.array[0]->present = AccessResult_PR_structure;
	report->listOfAccessResult.list.array[0]->choice.structure.list.count = 3;
	report->listOfAccessResult.list.array[0]->choice.structure.list.array =(Data_t**) dataArray;


	MmsValue * value1 = MmsValue_newUnsignedFromUint32(1);
	MmsValue * value2 = MmsValue_newVisibleString(domainId);
	MmsValue * value3 = MmsValue_newVisibleString(itemId);

	Data_t* dataElement1 = mmsMsg_createBasicDataElement(value1);
	Data_t* dataElement2 = mmsMsg_createBasicDataElement(value2);
	Data_t* dataElement3 = mmsMsg_createBasicDataElement(value3);
	
	report->listOfAccessResult.list.array[0]->choice.structure.list.array[0] = dataElement1;
	report->listOfAccessResult.list.array[0]->choice.structure.list.array[1] = dataElement2;
	report->listOfAccessResult.list.array[0]->choice.structure.list.array[2] = dataElement3;
	
	
	report->listOfAccessResult.list.array[1] =  accessResult2;
	report->listOfAccessResult.list.array[1]->present =AccessResult_PR_integer;
	INTEGER_t * asnIndex = &report->listOfAccessResult.list.array[1]->choice.integer;
	asn_long2INTEGER(asnIndex, (long) time_stamp);

	asn_enc_rval_t rval;

	rval = der_encode(&asn_DEF_MmsPdu, mmsPdu,
				(asn_app_consume_bytes_f*) mmsClient_write_out, (void*) writeBuffer);


	if (DEBUG) xer_fprint(stdout, &asn_DEF_MmsPdu, mmsPdu);


	/* freee variables */
/*	free(report->listOfAccessResult.list.array[0]->choice.structure.list.array[0]);
	free(report->listOfAccessResult.list.array[0]->choice.structure.list.array[1]);
	free(report->listOfAccessResult.list.array[0]->choice.structure.list.array[2]);
	free(report->listOfAccessResult.list.array[0]->choice.structure.list.array);
	report->listOfAccessResult.list.array[0]->choice.structure.list.count = 0;
	free(report->listOfAccessResult.list.array);
	report->listOfAccessResult.list.count = 0;
	report->listOfAccessResult.list.size = 0;


	free(report->variableAccessSpecification.choice.listOfVariable.list.array[0]);
	free(report->variableAccessSpecification.choice.listOfVariable.list.array[1]);
	free(report->variableAccessSpecification.choice.listOfVariable.list.array);
	report->variableAccessSpecification.choice.listOfVariable.list.count = 0;
	report->variableAccessSpecification.choice.listOfVariable.list.size = 0;
*/

	free(report->variableAccessSpecification.choice.listOfVariable.list.array[0]);
	free(report->variableAccessSpecification.choice.listOfVariable.list.array[1]);
	
	free(asnIndex->buf);
	free(dataArray);
	free(accessResult1);
	free(accessResult2);
	free(dataElement1);
	free(dataElement2);
	free(dataElement3);

	report->listOfAccessResult.list.count = 0;
	report->listOfAccessResult.list.size = 0;
	report->variableAccessSpecification.choice.listOfVariable.list.count = 0;
	report->variableAccessSpecification.choice.listOfVariable.list.size = 0;

	MmsValue_delete(value1);
	MmsValue_delete(value2);
	MmsValue_delete(value3);
	asn_DEF_MmsPdu.free_struct(&asn_DEF_MmsPdu, mmsPdu, 0);

	return rval.encoded;
}
