#include "rlang.h"


// In particular, this returns 1 for environments
r_size_t vec_length(SEXP x) {
  switch (r_kind(x)) {
  case LGLSXP:
  case INTSXP:
  case REALSXP:
  case CPLXSXP:
  case STRSXP:
  case RAWSXP:
  case VECSXP:
    return r_length(x);
  case NILSXP:
    return 0;
  default:
    return 1;
  }
}

SEXP r_scalar_lgl(bool x) {
  return Rf_ScalarLogical(x);
}


// Copy --------------------------------------------------------------

void vec_copy_n(SEXP src, r_size_t n, SEXP dest,
                r_size_t offset_dest,
                r_size_t offset_src) {
  switch (r_kind(dest)) {
  case LGLSXP: {
    int* src_data = LOGICAL(src);
    int* dest_data = LOGICAL(dest);
    for (r_size_t i = 0; i != n; ++i)
      dest_data[i + offset_dest] = src_data[i + offset_src];
    break;
  }
  case INTSXP: {
    int* src_data = INTEGER(src);
    int* dest_data = INTEGER(dest);
    for (r_size_t i = 0; i != n; ++i)
      dest_data[i + offset_dest] = src_data[i + offset_src];
    break;
  }
  case REALSXP: {
    double* src_data = REAL(src);
    double* dest_data = REAL(dest);
    for (r_size_t i = 0; i != n; ++i)
      dest_data[i + offset_dest] = src_data[i + offset_src];
    break;
  }
  case CPLXSXP: {
    r_complex_t* src_data = COMPLEX(src);
    r_complex_t* dest_data = COMPLEX(dest);
    for (r_size_t i = 0; i != n; ++i)
      dest_data[i + offset_dest] = src_data[i + offset_src];
    break;
  }
  case RAWSXP: {
    r_byte_t* src_data = RAW(src);
    r_byte_t* dest_data = RAW(dest);
    for (r_size_t i = 0; i != n; ++i)
      dest_data[i + offset_dest] = src_data[i + offset_src];
    break;
  }
  case STRSXP: {
    SEXP elt;
    for (r_size_t i = 0; i != n; ++i) {
      elt = STRING_ELT(src, i + offset_src);
      SET_STRING_ELT(dest, i + offset_dest, elt);
    }
    break;
  }
  case VECSXP: {
    SEXP elt;
    for (r_size_t i = 0; i != n; ++i) {
      elt = VECTOR_ELT(src, i + offset_src);
      SET_VECTOR_ELT(dest, i + offset_dest, elt);
    }
    break;
  }
  default:
    r_abort("Copy requires vectors");
  }
}


// Coercion ----------------------------------------------------------

SEXP namespace_rlang_sym(SEXP sym) {
  static SEXP rlang_sym = NULL;
  if (!rlang_sym)
    rlang_sym = Rf_install("rlang");
  return(Rf_lang3(Rf_install("::"), rlang_sym, sym));
}

SEXP vec_coercer_sym(SEXP dest) {
  switch(r_kind(dest)) {
  case LGLSXP: return namespace_rlang_sym(Rf_install("as_logical"));
  case INTSXP: return namespace_rlang_sym(Rf_install("as_integer"));
  case REALSXP: return namespace_rlang_sym(Rf_install("as_double"));
  case CPLXSXP: return namespace_rlang_sym(Rf_install("as_complex"));
  case STRSXP: return namespace_rlang_sym(Rf_install("as_character"));
  case RAWSXP: return namespace_rlang_sym(Rf_install("as_bytes"));
  default: r_abort("No coercion implemented for `%s`", Rf_type2str(r_kind(dest)));
  }
}

void vec_copy_coerce_n(SEXP src, r_size_t n, SEXP dest,
                       r_size_t offset_dest,
                       r_size_t offset_src) {
  if (r_kind(src) != r_kind(dest)) {
    if (r_is_object(src))
      r_abort("Can't splice S3 objects");
    // FIXME: This callbacks to rlang R coercers with an extra copy.
    PROTECT_INDEX ipx;
    SEXP call, coerced;
    PROTECT_WITH_INDEX(call = vec_coercer_sym(dest), &ipx);
    REPROTECT(call = Rf_lang2(call, src), ipx);
    REPROTECT(coerced = r_eval(call, R_BaseEnv), ipx);
    vec_copy_n(coerced, n, dest, offset_dest, offset_src);
    FREE(1);
  } else {
    vec_copy_n(src, n, dest, offset_dest, offset_src);
  }
}