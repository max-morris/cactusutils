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
  int *active;
  int *checked_variable;
  int *output_variables;
  int *output_variables_number;
  int *steered_scalar;
  int *last_checked;
  int *trigger_once;
  int *trigger_count;
  const char **relation;
  const char **reduction;
  const char **checked_parameter_thorn;
  const char **checked_parameter_name;
  CCTK_REAL *checked_value;
  const char **reaction;
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
  file_name = (char*) malloc(8+strlen(CCTK_VarName(varindex))+1);
  snprintf(file_name, 8+strlen(CCTK_VarName(varindex))+1,
           "%s%s", "trigger_", CCTK_VarName(varindex));
  if (my_GH->debug)
    CCTK_VInfo(CCTK_THORNSTRING,
      "Doing tiggered output of %s with method %s in file %s.",
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
  CCTK_REAL value;

  my_GH = (TriggerGH*)CCTK_GHExtension(GH, "Trigger");
  /* check if output was already done; this is important for triggered
   * variables since they are only allocated if triggered output is
   * wanted and _later_ (OutputGH) they are not allocated anymore */
  if (my_GH->debug)
    CCTK_VInfo(CCTK_THORNSTRING,
               "last_checked: %d", my_GH->last_checked[trigger]);
  if (my_GH->last_checked[trigger]>=GH->cctk_iteration)
  {
    if (my_GH->debug)
      CCTK_VInfo(CCTK_THORNSTRING,
                 "not doing output for trigger %d twice", trigger);
    return 0;
  }
  if (my_GH->trigger_count[trigger] > 0 && my_GH->trigger_once[trigger])
  {
    if (my_GH->debug)
      CCTK_VInfo(CCTK_THORNSTRING,
                 "skipping trigger %d because it was already "
                 "triggered in the past.", trigger);
    return 0;
  }
  /* do we have to use a reduction? */
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
    union anytypevalue_u {
      CCTK_REAL realval;
      CCTK_INT intval;
      CCTK_COMPLEX complexval;
    } anytypevalue;
    const int vartype=CCTK_VarTypeI(varindex);

    if (my_GH->debug)
      CCTK_VInfo(CCTK_THORNSTRING,
                 "reducing trigger %d red_handle %d varindex %d",
                 trigger, reduction_handle, varindex);
    errno=CCTK_Reduce(GH, -1, reduction_handle, 1,
                      vartype, &anytypevalue, 1, varindex);
    if (my_GH->debug)
      CCTK_VInfo(CCTK_THORNSTRING,
                 "reducing was ok");
    if (errno)
      CCTK_WARN(0, "Reduce returned an error.");

    switch (vartype) {
      case CCTK_VARIABLE_REAL:
        value=anytypevalue.realval;
        break;
      case CCTK_VARIABLE_INT:
        value=(CCTK_REAL)anytypevalue.intval;
        break;
      case CCTK_VARIABLE_COMPLEX:
        CCTK_ERROR("Cannot handle CCTK_COMPLEX variables");
        break;
      default:
        CCTK_VError(__LINE__, __FILE__, CCTK_THORNSTRING,
                    "Do not know how to handle variable type %d", vartype);
        break;
    }
  }
  else
  {
    // -1 indicates a parameter
    if (varindex>=0)
    {
      void *myVar = CCTK_VarDataPtrI(GH , 0, varindex);
      if (myVar == NULL)
      {
        CCTK_VError(__LINE__, __FILE__, CCTK_THORNSTRING,
                    "Variable '%s' has no storage", CCTK_FullName(varindex));
      }
      if (CCTK_VARIABLE_REAL == CCTK_VarTypeI(varindex))
        value=*(CCTK_REAL*)myVar;
      else if (CCTK_VARIABLE_INT == CCTK_VarTypeI(varindex))
        value=(CCTK_REAL) *(CCTK_INT*)myVar;
      else
        CCTK_VError(__LINE__, __FILE__, CCTK_THORNSTRING,
                    "Variable '%s' isn't CCTK_REAL or CCTK_INT, I don't know what to do with that.",
                    CCTK_FullName(varindex));
    }
    else
    {
      int type;
      const void *tmp_value;
      tmp_value=CCTK_ParameterGet(my_GH->checked_parameter_name[trigger],
                                  my_GH->checked_parameter_thorn[trigger],&type);
      switch (type) {
        case PARAMETER_REAL:
          value=*(const CCTK_REAL*)tmp_value;
          break;
        case PARAMETER_INT:
          value=(CCTK_REAL) *(const CCTK_INT*)tmp_value;
          break;
        case PARAMETER_BOOLEAN:
          value=(CCTK_REAL) *(const int*)tmp_value;
          break;
        default:
          CCTK_VError(__LINE__, __FILE__, CCTK_THORNSTRING,
                      "Cannot handle parameter type %d",type);
          break;
      }
    }
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
  {
    if (my_GH->debug)
    {
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
    }
  }
  else
  {
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
            "Trigger_TimeForOutput: requesting output for %d", varindex);
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
               "Trigger_TriggerOutput, varindex %d", varindex);
  /* loop over all triggers */
  for (i=0; i<my_GH->number; i++)
  {
    /* loop over all io methods */
    for (handle=CCTK_NumIOMethods()-1; handle>=0; handle--)
    {
      if (my_GH->debug)
        CCTK_VInfo(CCTK_THORNSTRING,
                   "io-method: %s, wanted:%s", CCTK_IOMethod(handle)->name,
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
    CCTK_VInfo(CCTK_THORNSTRING, "Testing triggers");
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
    // Reset this from parameter because it might have been steered
    my_GH->trigger_once[i] = Trigger_Once[i];
    if (CCTK_EQUALS(Trigger_Reaction[i],"steerparam"))
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
            CCTK_VInfo(CCTK_THORNSTRING, "Steering parameter");
          ret=CCTK_ParameterSet(Trigger_Steered_Parameter_Name[i],
                                Trigger_Steered_Parameter_Thorn[i],
                                Trigger_Steered_Parameter_Value[i]);
          switch(ret)
          {
            case  0: my_GH->trigger_count[i] += 1;
                     if (my_GH->debug) CCTK_VInfo(CCTK_THORNSTRING, "Parameter steered");
                     break;
            case -1: CCTK_WARN(1,"Parameter is out of range."); break;
            case -2: CCTK_WARN(0,"Parameter was not found."); break;
            case -3: CCTK_WARN(0,"Parameter is not steerable."); break;
            default: CCTK_WARN(1,"Error occured while setting parameter.");
                     break;
          }
        }
      }
    }
    if (CCTK_EQUALS(Trigger_Reaction[i],"steerscalar"))
    {
      if (Trigger_TriggerFullFilled(cctkGH, i))
      {
        if (my_GH->debug)
          CCTK_VInfo(CCTK_THORNSTRING, "Steering scalar");
        int type = CCTK_VarTypeI(my_GH->steered_scalar[i]);
        if (type == CCTK_VARIABLE_REAL)
        {
          CCTK_REAL *myVar = (CCTK_REAL *)(CCTK_VarDataPtrI(cctkGH,0,my_GH->steered_scalar[i]));
          if (myVar == NULL)
          {
            CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                      "Variable '%s' has no storage", CCTK_FullName(my_GH->steered_scalar[i]));
          }

          myVar[Trigger_Steered_Scalar_Index[i]] = atof(Trigger_Steered_Scalar_Value[i]);
          my_GH->trigger_count[i] += 1;
        }
        else if (type == CCTK_VARIABLE_INT)
        {
          CCTK_INT *myVar = (CCTK_INT *)(CCTK_VarDataPtrI(cctkGH,0,my_GH->steered_scalar[i]));
          if (myVar == NULL)
          {
            CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                      "Variable '%s' has no storage", CCTK_FullName(my_GH->steered_scalar[i]));
          }
          myVar[Trigger_Steered_Scalar_Index[i]] = atoi(Trigger_Steered_Scalar_Value[i]);
          my_GH->trigger_count[i] += 1;
        }
        else
          CCTK_WARN(0, "Cannot handle other types than CCTK_REAL and CCTK_INT");
      }
    }
  }
}


/* This struct is (only) used to pass three arguments to the callback of
 * CCTK_TransverseString instead of one */
typedef struct
{
  int trigger_number;
  enum enum_what_to_set {WTS_INPUT, WTS_OUTPUT, WTS_STEERSCALAR}
    what_to_set;
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
  if (info->what_to_set == WTS_INPUT)
    info->my_GH->checked_variable[info->trigger_number]=varindex;
  else if (info->what_to_set == WTS_STEERSCALAR)
  {
    info->my_GH->steered_scalar[info->trigger_number] = varindex;
  }
  else if (info->what_to_set == WTS_OUTPUT)
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

  my_GH->last_checked    = (int*)   
                           calloc(Trigger_Number,sizeof(int));
  my_GH->trigger_once    = (int*)   
                           calloc(Trigger_Number,sizeof(int));
  my_GH->trigger_count   = (int*)   
                           calloc(Trigger_Number,sizeof(int));
  my_GH->checked_variable= (int*)   
                           calloc(Trigger_Number,sizeof(int));
  my_GH->active          = (int*)   
                           calloc(Trigger_Number,sizeof(int));
  my_GH->steered_scalar  = (int*)   
                           calloc(Trigger_Number,sizeof(int));
  my_GH->output_variables= (int*)
                           calloc(Trigger_Number*CCTK_NumVars(),
                                  sizeof(int));
  my_GH->output_variables_number
                         = (int*)   
                           calloc(Trigger_Number,sizeof(int));
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
  my_GH->reaction        = (const char**)
                           calloc(Trigger_Number,sizeof(const char *));

  /* initialize datastructure */
  info->my_GH=my_GH;
  my_GH->number=Trigger_Number;
  my_GH->debug=Trigger_Debug;
  /* loop over all triggers */
  for (i=Trigger_Number-1; i>=0; i--)
  {
    my_GH->last_checked [i]=-1;
    my_GH->trigger_once [i]=Trigger_Once[i];
    my_GH->trigger_count[i]= 0;
    my_GH->output_method[i]=Trigger_Output_Method[i];
    my_GH->reaction     [i]=Trigger_Reaction[i];
    my_GH->relation     [i]=Trigger_Relation[i];
    my_GH->reduction    [i]=Trigger_Reduction[i];
    my_GH->checked_value[i]=Trigger_Checked_Value[i];
    info->trigger_number=i;
    info->what_to_set = WTS_INPUT;
    /* Are we looking for a variable or a parameter? */
    if (CCTK_EQUALS(Trigger_Checked_Variable[i],"param"))
    {
        if (!CCTK_Equals(Trigger_Checked_Parameter_Name[i], "") ||
            !CCTK_Equals(Trigger_Checked_Parameter_Thorn[i], ""))
        {
          if (!CCTK_ParameterGet(Trigger_Checked_Parameter_Name[i],
                                 Trigger_Checked_Parameter_Thorn[i],NULL))
              CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                        "No parameter with the name '%s' found",
                        Trigger_Checked_Parameter_Name[i]);
          my_GH->checked_variable[i]=-1;
          // TODO: this assumes that parameters strings do not go away
          my_GH->checked_parameter_name[i] =Trigger_Checked_Parameter_Name[i];
          my_GH->checked_parameter_thorn[i]=Trigger_Checked_Parameter_Thorn[i];
          my_GH->active[i]=1;
        }
    }
    else
    {
        if (CCTK_EQUALS(Trigger_Checked_Variable[i],""))
        {
          my_GH->checked_variable[i]=-1;
        }
        else
        {
          if (!CCTK_TraverseString(Trigger_Checked_Variable[i],
                                   Trigger_Transverse_Callback, info, CCTK_VAR))
              CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                         "No variable with the name '%s' found",
                         Trigger_Checked_Variable[i]);
          my_GH->active[i]=1;
        }
    }

    /* What should be done when the trigger is positive? */
    if (CCTK_EQUALS(Trigger_Reaction[i],"steerparam"))
    {
        if (!CCTK_ParameterGet(Trigger_Steered_Parameter_Name[i],
                               Trigger_Steered_Parameter_Thorn[i],NULL))
            CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                       "No parameter with the name '%s' found in trigger %d",
                       Trigger_Steered_Parameter_Name[i], i);
        /* TODO: check for steerability */
        my_GH->output_variables
                 [i*CCTK_NumVars() + my_GH->output_variables_number[i]]
              =-1;
        my_GH->output_variables_number[i]++;
    }
    /* scalar */
    else if (CCTK_EQUALS(Trigger_Reaction[i],"steerscalar"))
    {
      info->what_to_set = WTS_STEERSCALAR;
      if (!CCTK_TraverseString(Trigger_Steered_Scalar[i],
                               Trigger_Transverse_Callback,
                               info, CCTK_VAR))
        CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "No scalar with the name '%s' found",
                   Trigger_Steered_Scalar[i]);
    }
    /* output */
    else if (CCTK_EQUALS(Trigger_Reaction[i],"output"))
    {
      info->what_to_set = WTS_OUTPUT;
      if (!CCTK_TraverseString(Trigger_Output_Variables[i],
                               Trigger_Transverse_Callback,
                               info, CCTK_GROUP_OR_VAR))
        CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "No variable with the name '%s' found in trigger %d",
                   Trigger_Output_Variables[i], i);
    }
  }
  free(info);
  return my_GH;
}

/* Register our own GH extension
 */
int Trigger_Startup()
{
  CCTK_RegisterGHExtensionSetupGH(CCTK_RegisterGHExtension("Trigger"),
                                  Trigger_SetupGH);
  return 0;
}

/* Check some parameters, especially their steerability */
void Trigger_ParamCheck(CCTK_ARGUMENTS)
{
  DECLARE_CCTK_PARAMETERS
  TriggerGH *my_GH;
  my_GH = (TriggerGH*)CCTK_GHExtension(cctkGH, "Trigger");

  for (int i=Trigger_Number-1; i>=0; i--)
  {
    if (!my_GH->active[i])
      continue;

    if (CCTK_EQUALS(Trigger_Reaction[i],"steerparam"))
    {
      const cParamData *paramdata = CCTK_ParameterData(
                                      Trigger_Steered_Parameter_Name[i],
                                      Trigger_Steered_Parameter_Thorn[i]);
      if (!paramdata)
        CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "Parameter '%s::%s' not found",
                   Trigger_Steered_Parameter_Thorn[i],
                   Trigger_Steered_Parameter_Name[i]);
      if (paramdata->steerable != CCTK_STEERABLE_ALWAYS)
        CCTK_VWarn(0, __LINE__, __FILE__, CCTK_THORNSTRING,
                   "Parameter '%s::%s' not (always) steerable",
                   Trigger_Steered_Parameter_Thorn[i],
                   Trigger_Steered_Parameter_Name[i]);
    }
  }

  return;
}

