(lang dune 2.8)
(version 0.1.7)
(name samplerate)
(source (github savonet/ocaml-samplerate))
(license BSD-2-Clause)
(authors "The Savonet Team <savonet-users@lists.sourceforge.net>")
(maintainers "The Savonet Team <savonet-users@lists.sourceforge.net>")

(generate_opam_files true)
(use_standard_c_and_cxx_flags false)

(package
 (name samplerate)
 (synopsis "Samplerate audio conversion library")
 (description "Bindings for the samplerate library which provides functions for changing samplerate of audio data")
 (depends
  dune
  (dune-configurator :build)
  (conf-samplerate :build)
  (conf-pkg-config :build))
)
