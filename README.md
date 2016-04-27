iATROS (improved ATROS) is an implementation of a previous speech recogniser (ATROS) that can be used in both speech and handwritten text recognition. iATROS provides a modular structure that can be used to build different systems whose core is a Viterbi-like search on a Hidden Markov Model network. iATROS provides standard tools for off-line recognition and on-line speech recognition (based on ALSA modules).

iATROS is composed of two preprocessing and feature extraction modules (for speech signal and handwriting images) and a core recognition module. The preprocessing and feature extraction modules provide feature vectors to the recognition module, that using HMM models and language models performs the search for the best recognition hypothesis. All the modules are implemented in C.

## How to build iATROS:

```bash
$ mkdir build
$ cd build
$ ../configure --build-type release
$ make
$ make install
```
