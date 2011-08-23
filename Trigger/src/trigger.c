#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cctk.h"
#include "cctk_WarnLevel.h"
#include "cctk_Arguments.h"
#include "cctk_Parameters.h"

/* Trigger GH extension structure */
typedef struct
{
  int number;
  int *checked_variable;
  int *output_variables;
  int *output_variables_number;
  int *last_checked;
  const char **relation;
  const char **reduction;
  const char **checked_parameter_thorn;
  const char **checked_parameter_name;
  CCTK_REAL *checked_value;
  const char **output_method;
  const char *out_dir;
  int debug;
} TriggerGH;

/* This routine will output the variable with varindex as index */
int Trigger_Write(const cGH *GH, int varindex, const char *method);
int Trigger_Write(const cGH *GH, int varindex, const char *method)
{
  char *full_name, *file_name;
  TriggerGH *my_GH;
  my_GH = (TriggerGH*)CCTK_GHExtension(GH, "Trigger");
  full_name=CCTK_FullName(varindex);
  if (!full_name)
    return 0;
  file_name = (char*) malloc(8+(int)strlen(CCTK_VarName(varindex))+1);
  snprintf(file_name, 8+(int)strlen(CCTK_VarName(varindex))+1,
           "%s%s", "trigger_", CCTK_VarName(varindex));
  if (my_GH->debug)
    CCTK_VInfo(CCTK_THORNSTRING,
      "Doing tiggered output of %s with method %s in file %s.\n",
      full_name, method, file_name);
  CCTK_OutputVarAsByMethod(GH, full_name, method, file_name);
  free(file_name);
  free(full_name);
  return 1;
}

/* This routine checks if a trigger is fullfilled */
int Trigger_TriggerFullFilled(const cGH *GH, int trigger);
int Trigger_TriggerFullFilled(const cGH *GH, int trigger)
{
  TriggerGH *my_GH;
  int varindex, reduction_handle=0, errno, ret;
  CCTK_REAL *tmp_value, value;
  cGH *not_const_GH;

  /* as long as GH in Reduce() is not const, we have to use a cast to
   * prevent a waring while compiling */
  not_const_GH=(cGH*) GH;
  
  my_GH = (TriggerGH*)CCTK_GHExtension(GH, "Trigger");
  /* check if output was already done; this is important for triggered
   * variables since they are only allocated if triggered output is
   * wanted and _later_ (OutputGH) they are not allocated anymore */
  if (my_GH->debug)
    CCTK_VInfo(CCTK_THORNSTRING,
               "last_checked: %d\n", my_GH->last_checked[trigger]);
  if (my_GH->last_checked[trigger]>=GH->cctk_iteration)
  {
    if (my_GH->debug)
      CCTK_VInfo(CCTK_THORNSTRING,
                 "not doing output for trigger %d twice\n", trigger);
    return 0;
  }
  /* do we have to use a reduction" */
  if (!CCTK_EQUALS(my_GH->reduction[trigger], ""))
  {
    /* get a reduction handle */
    reduction_handle=CCTK_ReductionHandle(my_GH->reduction[trigger]);
    if (reduction_handle<0)
      CCTK_WARN(0, "Unable to get reduction handle.");
  }
  /* get variable to check for */
  varindex=my_GH->checked_variable[trigger];
  /* Do reduce */
  if (reduction_handle)
  {
    if (my_GH->debug)
      CCTK_VInfo(CCTK_THORNSTRING,
                 "reducing %d %d\n", reduction_handle, varindex);
    errno=CCTK_Reduce(not_const_GH, -1, reduction_handle, 1,
                      CCTK_VARIABLE_REAL, &value, 1, varindex);
    if (my_GH->debug)
      CCTK_VInfo(CCTK_THORNSTRING,
                 "reducing was ok\n");
    if (errno)
      CCTK_WARN(0, "Reduce returned an error.");
  }
  else
    // -1 indicates a paramter
    if (varindex>=0)
      value=((CCTK_REAL *)CCTK_VarDataPtrI(GH , 0, varindex))[0];
    else
    {
      tmp_value=((CCTK_REAL *)CCTK_ParameterGet(
                               my_GH->checked_parameter_name[trigger],
                               my_GH->checked_parameter_thorn[trigger],NULL));
      value=tmp_value[0];
    }
  /* check condition of this trigger */
  ret=0;
  if ( (CCTK_EQUALS(my_GH->relation[trigger], ">") &&
       (value > my_GH->checked_value[trigger]))        ||
       (CCTK_EQUALS(my_GH->relation[trigger], "<") &&
       (value < my_GH->checked_value[trigger]))        ||
       (CCTK_EQUALS(my_GH->relation[trigger], "==") &&
       (value == my_GH->checked_value[trigger]))       ||
       (CCTK_EQUALS(my_GH->relation[trigger], "!=") &&
       (value != my_GH->checked_value[trigger]))
     )
    ret=1;
  if (ret)
    if (varindex>=0)
      CCTK_VInfo(CCTK_THORNSTRING,"trigger nr. %d fullfilled for %s (%f%s%f)",
                 trigger, CCTK_VarName(varindex),
                 value, my_GH->relation[trigger],
                 my_GH->checked_value[trigger]);
    else
      CCTK_VInfo(CCTK_THORNSTRING,
                 "trigger nr. %d fullfilled for %s::%s (%f%s%f)",
                 trigger, my_GH->checked_parameter_name[trigger],
                          my_GH->checked_parameter_thorn[trigger],
                 value, my_GH->relation[trigger],
                 my_GH->checked_value[trigger]);
  else
    if (my_GH->debug)
    {
      if (varindex>=0)
        CCTK_VInfo(CCTK_THORNSTRING,
                   "trigger nr. %d not fullfilled for %s (%f%s%f)",
                   trigger, CCTK_VarName(varindex),
                   value, my_GH->relation[trigger],
                   my_GH->checked_value[trigger]);
      else
        CCTK_VInfo(CCTK_THORNSTRING,
                   "trigger nr. %d not fullfilled for %s::%s (%f%s%f)",
                   trigger, my_GH->checked_parameter_name[trigger],
                            my_GH->checked_parameter_thorn[trigger],
                   value, my_GH->relation[trigger],
                   my_GH->checked_value[trigger]);
    }
  return ret;
}

/* This routine is looking for triggers and checks their output variables.
 * If one output variable of one trigger matches the requested varindex,
 * we return 1. We do not check if the trigger is fullfilled, because
 * some variables might not be allocated yet */
int Trigger_TimeForOutput(const cGH *GH, int varindex);
int Trigger_TimeForOutput(const cGH *GH, int varindex)
{
  TriggerGH *my_GH;
  int i,j;

  my_GH = (TriggerGH*)CCTK_GHExtension(GH, "Trigger");
  /* loop over all triggers */
  for (i=0; i<my_GH->number; i++)
  {
    /* loop over all output variables of one trigger */
    for (j=my_GH->output_variables_number[i]-1; j>=0; j--)
    {
      if (my_GH->output_variables[i*CCTK_NumVars()+j]==varindex)
      {
        if (my_GH->debug)
          CCTK_VInfo(CCTK_THORNSTRING,
            "Trigger_TimeForOutput: requesting output for %d\n", varindex);
        return 1;
      }
    }
  }
  return 0;
}

/* output triggered variables if nessesary,
 * This routine does _not_ nessecarily output varindex; it loops over all
 * triggers and outputs variables that are specified there. */
int Trigger_TriggerOutput(const cGH *GH, int varindex);
int Trigger_TriggerOutput(const cGH *GH, int varindex)
{
  int i, j, handle, ret=1;
  TriggerGH *my_GH;
  my_GH = (TriggerGH*)CCTK_GHExtension(GH, "Trigger");
  if (my_GH->debug)
    CCTK_VInfo(CCTK_THORNSTRING,
               "Trigger_TriggerOutput, varindex %d\n", varindex);
  /* loop over all triggers */
  for (i=0; i<my_GH->number; i++)
  {
    /* loop over all io methods */
    for (handle=CCTK_NumIOMethods()-1; handle>=0; handle--)
    {
      if (my_GH->debug)
        CCTK_VInfo(CCTK_THORNSTRING,
                   "io-method: %s, wanted:%s\n", CCTK_IOMethod(handle)->name,
                   my_GH->output_method[i]);
      /* check if we want to output using that io method */
      if (CCTK_EQUALS(CCTK_IOMethod(handle)->name, my_GH->output_method[i]))
      {
        /* check the condition of the trigger */
        if (Trigger_TriggerFullFilled(GH, i))
        {
          /* loop over all variables to output */
          for (j=my_GH->output_variables_number[i]-1; j>=0; j--)
          {
            /* check, if this trigger did want output for varindex */
            if (my_GH->output_variables[j]==varindex)
            {
              /* do the output */
              if (!Trigger_Write(GH,my_GH->output_variables[i*CCTK_NumVars()+j],
                                    CCTK_IOMethod(handle)->name))
                ret=0;
            }
          }
        }
        /* set it as being checked */
        my_GH->last_checked[i]=GH->cctk_iteration;
      }
    }
  }
  return ret;
}

/* This function gets called for triggered output variables */
void Trigger_Check(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_ARGUMENTS
  DECLARE_CCTK_PARAMETERS
  int varindex, ret, i;
  char *valstr;
  TriggerGH *my_GH;
  my_GH = (TriggerGH*)CCTK_GHExtension(cctkGH, "Trigger");
  if (my_GH->debug)
    CCTK_VInfo(CCTK_THORNSTRING, "Testing triggers\n");
  /* refresh internal variables */
  trigger_cctk_iteration[0]=(CCTK_REAL)cctk_iteration;
  trigger_cctk_time[0]=(CCTK_REAL)cctk_time;
  ret=0;
  /* loop over all variables */ 
  for (varindex = CCTK_NumVars()-1; varindex >= 0; varindex--)
    /* if it is time for output and output was ok, count this */
    if (Trigger_TimeForOutput(cctkGH, varindex) &&
        Trigger_TriggerOutput(cctkGH, varindex))
      ret++;
  /* check for parameter steering */
  /* loop over all triggers */
  for (i=0; i<my_GH->number; i++)
  {
    if (CCTK_EQUALS(Trigger_Output_Variables[i],"param"))
    {
      if (Trigger_TriggerFullFilled(cctkGH, i))
      {
        valstr=CCTK_ParameterValString(Trigger_Steered_Parameter_Name[i],
                                       Trigger_Steered_Parameter_Thorn[i]);
        if (CCTK_EQUALS(valstr, Trigger_Steered_Parameter_Value[i]))
          free(valstr);
        else
        {
          free(valstr);
          if (my_GH->debug)
            CCTK_VInfo(CCTK_THORNSTRING, "Steering parameter\n");
          ret=CCTK_ParameterSet(Trigger_Steered_Parameter_Name[i],
                                Trigger_Steered_Parameter_Thorn[i],
                                Trigger_Steered_Parameter_Value[i]);
          switch(ret)
          {
            case  0: CCTK_VInfo(CCTK_THORNSTRING, "Parameter steered\n"); break;
            case -1: CCTK_WARN(1,"Parameter is out of range."); break;
            case -2: CCTK_WARN(0,"Parameter was not found."); break;
            case -3: CCTK_WARN(0,"Parameter is not steerable."); break;
            default: CCTK_WARN(1,"Error occured while setting parameter.");
                     break;
          }
        }
      }
    }
  }
}

/* This struct is (only) used to pass three arguments to the callback of
 * CCTK_TransverseString instead of one */
typedef struct
{
  int trigger_number;
  int input_output;
  TriggerGH *my_GH;
} transverse_info;

/* private -> callback for setting the variable index */
static void Trigger_Transverse_Callback(int varindex, const char *optstring,
                                        void *arg)
{
  transverse_info *info;
  /* get the info back */
  info=(transverse_info*)arg;
  /* do we want to get the input or the output variables? */
  if (info->input_output==0) /*input*/
    info->my_GH->checked_variable[info->trigger_number]=varindex;
  else /* output */
  {
    info->my_GH->output_variables
                 [info->trigger_number*CCTK_NumVars()+
                  info->my_GH->output_variables_number[info->trigger_number]]
                                                       =varindex;
    info->my_GH->output_variables_number[info->trigger_number]++;
  }
}

/* this function is called by the flesh as callback to initialize the
 * GH-extension */
static void *Trigger_SetupGH(tFleshConfig *config, int conv_level, cGH *GH)
{
  DECLARE_CCTK_PARAMETERS
  TriggerGH *my_GH;
  int i;
  transverse_info *info;
  
  /* allocate internal data structures */
  my_GH = (TriggerGH*) malloc(sizeof(TriggerGH));
  info = (transverse_info*) malloc(sizeof(transverse_info));

  my_GH->last_checked     = (CCTK_INT*)   
                           calloc(Trigger_Number,sizeof(CCTK_INT));
  my_GH->checked_variable= (CCTK_INT*)   
                           calloc(Trigger_Number,sizeof(CCTK_INT));
  my_GH->output_variables= (CCTK_INT*)
                           calloc(Trigger_Number*CCTK_NumVars(),
                                  sizeof(CCTK_INT));
  my_GH->output_variables_number
                         = (CCTK_INT*)   
                           calloc(Trigger_Number,sizeof(CCTK_INT));
  my_GH->relation        = (const char**)
                           calloc(Trigger_Number,sizeof(const char *));
  my_GH->reduction       = (const char**)
                           calloc(Trigger_Number,sizeof(const char *));
  my_GH->checked_value   = (CCTK_REAL*)  
                           calloc(Trigger_Number,sizeof(CCTK_REAL));
  my_GH->checked_parameter_thorn = (const char**)
                           calloc(Trigger_Number,sizeof(const char *));
  my_GH->checked_parameter_name = (const char**)
                           calloc(Trigger_Number,sizeof(const char *));
  my_GH->output_method   = (const char**)
                           calloc(Trigger_Number,sizeof(const char *));

  /* initialize datastructure */
  info->my_GH=my_GH;
  my_GH->number=Trigger_Number;
  my_GH->debug=Trigger_Debug;
  /* loop over all triggers */
  for (i=Trigger_Number-1; i>=0; i--)
  {
    my_GH->last_checked[i]=-1;
    my_GH->output_method[i]=Trigger_Output_Method[i];
    my_GH->relation[i]=Trigger_Relation[i];
    my_GH->reduction[i]=Trigger_Reduction[i];
    my_GH->checked_value[i]=Trigger_Checked_Value[i];
    info->trigger_number=i;
    info->input_output=0;
    /* If it is no variable, try a parameter */
    if (CCTK_EQUALS(Trigger_Checked_Variable[i],"param"))
    {
        if (!CCTK_ParameterGet(Trigger_Checked_Parameter_Name[i],
                               Trigger_Checked_Parameter_Thorn[i],NULL))
            CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                      "No parameter with the name '%s' found",
                      Trigger_Checked_Parameter_Name[i]);
        my_GH->checked_variable[i]=-1;
        my_GH->checked_parameter_name[i] =Trigger_Checked_Parameter_Name[i];
        my_GH->checked_parameter_thorn[i]=Trigger_Checked_Parameter_Thorn[i];
    }
    else
        if (!CCTK_TraverseString(Trigger_Checked_Variable[i],
                                 Trigger_Transverse_Callback, info, CCTK_VAR))
            CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                       "No variable with the name '%s' found",
                       Trigger_Checked_Variable[i]);
    info->input_output=1;
    /* If it is no variable, try a parameter */
    if (CCTK_EQUALS(Trigger_Output_Variables[i],"param"))
    {
        if (!CCTK_ParameterGet(Trigger_Steered_Parameter_Name[i],
                               Trigger_Steered_Parameter_Thorn[i],NULL))
            CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                       "No parameter with the name '%s' found",
                       Trigger_Steered_Parameter_Name[i]);
        my_GH->output_variables
                 [i*CCTK_NumVars() + my_GH->output_variables_number[i]]
              =-1;
        my_GH->output_variables_number[i]++;
    }
    else
      if (!CCTK_TraverseString(Trigger_Output_Variables[i],
                               Trigger_Transverse_Callback,
                               info, CCTK_GROUP_OR_VAR))
        CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "No variable with the name '%s' found",
                   Trigger_Output_Variables[i]);
  }
  free(info);
  return my_GH;
}

/* This is the only routine, which is called from the scheduler.
 */
int Trigger_Startup()
{
  CCTK_RegisterGHExtensionSetupGH(CCTK_RegisterGHExtension("Trigger"),
                                  Trigger_SetupGH);
  return 0;
}

