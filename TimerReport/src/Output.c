 /*@@
   @file      Output.c
   @date      July 6 2003
   @author    Gabrielle Allen
   @desc
              Functions to deal with making TimerReport an IO method
   @enddesc
   @version   $Id$
 @@*/

#include "cctk.h"
#include "cctk_Parameters.h"
#include "cctk_Arguments.h"

#include "cctk_Schedule.h"

static const char *rcsid = "$Header$";

CCTK_FILEVERSION(CactusUtils_TimerReport_Output_c)

/********************************************************************
 *********************     Local Data Types   ***********************
 ********************************************************************/

/********************************************************************
 ********************* Local Routine Prototypes *********************
 ********************************************************************/

int TimerReport_TimeForOutput (const cGH *GH, int vindex);

/********************************************************************
 ********************* Other Routine Prototypes *********************
 ********************************************************************/

/********************************************************************
 *********************     Local Data   *****************************
 ********************************************************************/

/********************************************************************
 ********************    External Routines   ************************
 ********************************************************************/

void TimerReport_Startup (void);


 /*@@
   @routine   TimerReport_Startup
   @date      Sun 6th July 2003
   @author    Gabrielle Allen
   @desc
              The startup registration routine for TimerReport.
	      Registers TimerReport as an IO Method and provide the
	      only necessary method TimeForOutput
   @enddesc
   @calls     CCTK_RegisterGHExtensionSetupGH
@@*/
void TimerReport_Startup (void)
{
  int handle;

  handle = CCTK_RegisterIOMethod ("TimerReport");
  CCTK_RegisterIOMethodTimeToOutput (handle, TimerReport_TimeForOutput);
}


/********************************************************************
 ********************    Internal Routines   ************************
 ********************************************************************/

int TimerReport_TimeForOutput (const cGH *GH, int vindex);

 /*@@
   @routine    TimerReport_TimeForOutput
   @date       July 6 2003
   @author     Gabrielle Allen
   @desc
               Decides if it is time to output timer information
   @enddesc
   @calls      CheckSteerableParameters

   @var        GH
   @vdesc      Pointer to CCTK GH
   @vtype      const cGH *
   @vio        in
   @endvar
   @var        vindex
   @vdesc      index of variable to check for output
   @vtype      int
   @vio        in
   @endvar

   @returntype int
   @returndesc
               true/false (1 or 0) if analysis should be called
   @endreturndesc
@@*/
int TimerReport_TimeForOutput (const cGH *GH, int vindex)
{
  DECLARE_CCTK_PARAMETERS

  int retval=0;

  if (vindex==CCTK_VarIndex("timerreport::triggervar"))
  {
    if (next || out_at == GH->cctk_iteration)
    {
      retval = 1;
    }
    else if (out_every) 
    {
      if (GH->cctk_iteration%out_every == 0)
      {
	retval = 1;
      }
    }
  }

  return retval;
}

 /*@@
   @routine    TimerReport_Output
   @date       July 6 2003
   @author     Gabrielle Allen
   @desc
   Output the timer table
   @enddesc
   @calls      
@@*/
void TimerReport_Output(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS
  DECLARE_CCTK_PARAMETERS

  CCTK_SchedulePrintTimes(NULL);

  if (next) 
  {
    CCTK_ParameterSet("next", CCTK_THORNSTRING, "no");
  }

  return;
}

 /*@@
   @routine    TimerReport_Checkpoint
   @date       April 10 2004
   @author     Erik Schnetter
   @desc
   Output the timer table if before_checkpoint is set
   @enddesc
   @calls      
@@*/
void TimerReport_Checkpoint(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS
  DECLARE_CCTK_PARAMETERS

  if (before_checkpoint)
  {
    CCTK_SchedulePrintTimes(NULL);
  }

  return;
}
