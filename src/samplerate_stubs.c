#include <caml/alloc.h>
#include <caml/callback.h>
#include <caml/custom.h>
#include <caml/fail.h>
#include <caml/memory.h>
#include <caml/mlvalues.h>
#include <caml/signals.h>

#include <samplerate.h>

#include <assert.h>
#include <stdio.h>

#define Int_conv(c) Int_val(c)

CAMLprim value ocaml_samplerate_get_conv_name(value conv)
{
  return caml_copy_string(src_get_name(Int_conv(conv)));
}

CAMLprim value ocaml_samplerate_get_conv_descr(value conv)
{
  return caml_copy_string(src_get_description(Int_conv(conv)));
}

CAMLprim value ocaml_samplerate_convert(value vconv, value vchans, value vratio, value vinbuf, value inofs, value len)
{
  CAMLparam2(vratio, vinbuf);
  int chans = Int_val(vchans);
  float ratio = Double_val(vratio);
  int inbuflen = Int_val(len);
  float *inbuf = (float*)malloc(sizeof(float) * inbuflen);
  int outbuflen = (int)(inbuflen * ratio) + 64;
  float *outbuf = (float*)malloc(sizeof(float) * outbuflen);
  SRC_DATA src_data;
  int i, ret;
  value ans;
  int anslen;

  for (i=0; i<inbuflen; i++)
    inbuf[i] = Double_field(vinbuf, i + Int_val(inofs));

  src_data.data_in = inbuf;
  src_data.input_frames = inbuflen / chans;
  src_data.data_out = outbuf;
  src_data.output_frames = outbuflen / chans;
  src_data.src_ratio = ratio;

  caml_enter_blocking_section();
  ret = src_simple(&src_data, Int_val(vconv), chans);
  caml_leave_blocking_section();

  free(inbuf);
  if (ret != 0)
  {
    fprintf(stderr, "ocaml-samplerate (%d): %s\n", ret, src_strerror(ret));
    assert(ret == 0);
  }
  assert(src_data.input_frames_used == src_data.input_frames);
  anslen = src_data.output_frames_gen * chans;
  ans = caml_alloc(anslen * Double_wosize, Double_array_tag);

  for (i=0; i<anslen; i++)
    Store_double_field(ans, i, outbuf[i]);
  free(outbuf);

  CAMLreturn(ans);
}

CAMLprim value ocaml_samplerate_convert_byte(value *argv, int argc)
{
  return ocaml_samplerate_convert(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5]);
}

#define State_val(v) (*((SRC_STATE**)Data_custom_val(v)))

static void finalize_src(value s)
{
  src_delete(State_val(s));
}

static struct custom_operations state_ops =
{
  "ocaml_samplerate_state",
  finalize_src,
  custom_compare_default,
  custom_hash_default,
  custom_serialize_default,
  custom_deserialize_default
};

CAMLprim value ocaml_samplerate_new(value converter, value channels)
{
  int err;
  SRC_STATE *state = src_new(Int_val(converter), Int_val(channels), &err);
  value ans;

  ans = caml_alloc_custom(&state_ops, sizeof(SRC_STATE*), 1, 0);
  assert(state); /* TODO: raise depending on err */
  State_val(ans) = state;

  return ans;
}

CAMLprim value ocaml_samplerate_process(value src, value _ratio, value _inbuf, value _inbufofs, value _inbuflen, value _outbuf, value _outbufofs, value _outbuflen)
{
  CAMLparam4(src, _ratio, _inbuf, _outbuf);
  CAMLlocal1(ans);
  int inbufofs = Int_val(_inbufofs);
  int inbuflen = Int_val(_inbuflen);
  int outbufofs = Int_val(_outbufofs);
  int outbuflen = Int_val(_outbuflen);
  float *inbuf, *outbuf;
  SRC_DATA data;
  SRC_STATE *state = State_val(src);
  int i;

  inbuf = malloc(inbuflen * sizeof(float));
  for (i = 0; i < inbuflen; i++)
    inbuf[i] = Double_field(_inbuf, inbufofs + i);
  outbuf = malloc(outbuflen * sizeof(float));
  data.data_in = inbuf;
  data.input_frames = inbuflen;
  data.data_out = outbuf;
  data.output_frames = outbuflen;
  data.src_ratio = Double_val(_ratio);
  if (inbuflen == 0)
    data.end_of_input = 1;
  else
    data.end_of_input = 0;

  caml_enter_blocking_section();
  assert(!src_process(state, &data));
  caml_leave_blocking_section();

  for (i = 0; i < data.output_frames_gen; i++)
    Store_double_field(_outbuf, outbufofs + i, outbuf[i]);
  ans = caml_alloc_tuple(2);
  Store_field(ans, 0, Val_int(data.input_frames_used));
  Store_field(ans, 1, Val_int(data.output_frames_gen));

  CAMLreturn(ans);
}

CAMLprim value ocaml_samplerate_process_byte(value *argv, int argc)
{
  return ocaml_samplerate_process(argv[0], argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]);
}

CAMLprim value ocaml_samplerate_reset(value src)
{
  src_reset(State_val(src));

  return Val_unit;
}

CAMLprim value ocaml_samplerate_set_ratio(value src, value ratio)
{
  src_set_ratio(State_val(src), Double_val(ratio));

  return Val_unit;
}
