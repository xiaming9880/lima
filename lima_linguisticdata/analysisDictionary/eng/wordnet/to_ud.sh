#!/bin/bash
# 
# Shell script created by Romaric Besancon on Wed Nov 22 2017 
# Version : $Id$ 
#

cp formes-eng.dic formes-eng.dic.2
sed -e 's/NN[S]*/NOUN/' -e 's/JJ[RS]*/ADJ/' -e 's/VB[DNGZP]*/VERB/' formes-eng.dic.2 > formes-eng.dic
