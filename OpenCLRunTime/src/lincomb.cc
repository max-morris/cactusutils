#include "OpenCLRunTime.h"

#include <cctk.h>
#include <cctk_Parameters.h>

#include <algorithm>
#include <cstdlib>
#include <map>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

namespace OpenCLRunTime {

typedef map<string, OpenCLKernel *> lincomb_kernels_t;
lincomb_kernels_t lincomb_kernels;

static string name(char const *const var, int const n) {
  stringstream buf;
  buf << var << n;
  return buf.str();
}

// static string name(int const var, int const tl)
// {
//   stringstream buf;
//   buf << CCTK_VarName(var);
//   for (int i=0; i<tl; ++i) {
//     buf << "_p";
//   }
//   return buf.str();
// }

extern "C" CCTK_INT OpenCLRunTime_LinearCombination(
    CCTK_POINTER_TO_CONST const cctkGH_, CCTK_INT const var, CCTK_INT const rl,
    CCTK_INT const tl, CCTK_REAL const scale,
    CCTK_INT const *restrict const srcs, CCTK_INT const *restrict const tls,
    CCTK_REAL const *restrict const facts, CCTK_INT const nsrcs) {
  cGH const *const cctkGH = static_cast<cGH const *>(cctkGH_);
  DECLARE_CCTK_PARAMETERS;

  if (scale == 0.0) {
    Accelerator_RequireInvalidData(cctkGH, &var, &rl, &tl, 1, 1);
  } else {
    Accelerator_RequireValidData(cctkGH, &var, &rl, &tl, 1, 1);
  }
  vector<CCTK_INT> const rls(nsrcs, rl);
  Accelerator_RequireValidData(cctkGH, srcs, &rls[0], tls, nsrcs, 1);

  // string const varname = name(var, 0);
  string const varname = "var";
  vector<string> srcnames(nsrcs);
  for (int n = 0; n < nsrcs; ++n) {
    // srcnames[n] = name(srcs[n], tls[n]);
    srcnames[n] = name("src", n);
  }

  // TODO: When grid function pointers are passed anew for every
  // kernel call, then do not generate different kernels for
  // different grid functions any more
  stringstream keybuf;
#if 0
    keybuf << varname << "=" << scale << "*" << varname;
    for (int n=0; n<nsrcs; ++n) {
      keybuf << "+" << facts[n] << "*" << srcnames[n];
    }
#else
  keybuf << "=" << scale;
  for (int n = 0; n < nsrcs; ++n) {
    keybuf << "+" << facts[n];
  }
#endif
  string const key = keybuf.str();

  string kernelname = string("LinearCombination") + key;
  replace(kernelname.begin(), kernelname.end(), '=', '_');
  replace(kernelname.begin(), kernelname.end(), '+', 'p');
  replace(kernelname.begin(), kernelname.end(), '-', 'm');
  replace(kernelname.begin(), kernelname.end(), '.', '_');

  // Look up the kernel pointer in the map. Insert a new element
  // with a NULL pointer if it does not exist yet.
  OpenCLKernel *&pkernel =
      (*lincomb_kernels.insert(make_pair(key, (OpenCLKernel *)0)).first).second;

  string source;

  if (not pkernel) {

    if (veryverbose) {
      CCTK_VInfo(CCTK_THORNSTRING, "Creating LinearCombination kernel %s",
                 kernelname.c_str());
    }

    stringstream buf;
    buf << "ptrdiff_t const di = 1;\n"
        << "ptrdiff_t const dj = CCTK_GFINDEX3D(cctkGH,0,1,0) - "
           "CCTK_GFINDEX3D(cctkGH,0,0,0);\n"
        << "ptrdiff_t const dk = CCTK_GFINDEX3D(cctkGH,0,0,1) - "
           "CCTK_GFINDEX3D(cctkGH,0,0,0);\n"
        << "LC_LOOP3VEC(" << kernelname << ",\n"
        << "  i,j,k, imin[0],imin[1],imin[2], imax[0],imax[1],imax[2],\n"
        << "  cctk_ash[0],cctk_ash[1],cctk_ash[2],\n"
        << "  CCTK_REAL_VEC_SIZE)\n"
        << "{\n"
        << "  ptrdiff_t const index = di*i + dj*j + dk*k;\n"
        << "  \n";
    if (scale == 0.0) {
      buf << "  CCTK_REAL_VEC " << varname << "L = ToReal(0.0);\n";
    } else {
      buf << "  CCTK_REAL_VEC " << varname << "L = vec_load(" << varname
          << "[index]);\n";
    }
    for (int n = 0; n < nsrcs; ++n) {
      if (facts[n] != 0.0) {
        buf << "  CCTK_REAL_VEC const " << srcnames[n] << "L = vec_load("
            << srcnames[n] << "[index]);\n";
      }
    }
    buf << "  \n";
    if (scale != 0.0) {
      buf << "  " << varname << "L = kmul(ToReal(" << scale << "), " << varname
          << "L);\n";
    }
    for (int n = 0; n < nsrcs; ++n) {
      if (facts[n] != 0.0) {
        buf << "  " << varname << "L = kmadd(ToReal(" << facts[n] << "), "
            << srcnames[n] << "L, " << varname << "L);\n";
      }
    }
    buf << "  \n"
        << "  vec_store_partial_prepare(i,lc_imin,lc_imax);\n"
        << "  vec_store_nta_partial(" << varname << "[index], " << varname
        << "L);\n"
        << "} LC_ENDLOOP3VEC(" << kernelname << ");\n";

    source = buf.str();

  } // if not pkernel

  char const *const sources[] = {"", source.c_str(), NULL};

  vector<int> varindices(nsrcs + 1);
  vector<int> timelevels(nsrcs + 1);
  vector<char const *> aliases(nsrcs + 1);
  varindices[0] = var;
  timelevels[0] = tl;
  aliases[0] = varname.c_str();
  for (int n = 0; n < nsrcs; ++n) {
    varindices[n + 1] = srcs[n];
    timelevels[n + 1] = tls[n];
    aliases[n + 1] = srcnames[n].c_str();
  }

  int const imin[3] = {0, 0, 0};
  int const *const imax = cctkGH->cctk_lsh;

  OpenCLRunTime_CallKernel(cctkGH, CCTK_THORNSTRING, kernelname.c_str(),
                           sources, NULL, &varindices.front(),
                           &timelevels.front(), &aliases.front(), nsrcs + 1,
                           imin, imax, &pkernel);

  Accelerator_NotifyDataModified(cctkGH, &var, &rl, &tl, 1, 1);

  return 0;
}

} // namespace OpenCLRunTime
