#! /bin/bash -x

export LC_NUMERIC=C

workdir=$PWD
echo "PWD: $workdir"

#Creo el directorio para guardar los resultados
dir=$workdir/resultados
mkdir -p $dir

corpus=$1
if test ! -d "$workdir/$corpus"; then
  #Descargo los datos 
  wget -c http://users.dsic.upv.es/~mlujan/$corpus".tar.gz"
  #Descomprimo los datos
  tar -xvzf $corpus".tar.gz" 
fi

mkdir -p $dir/$corpus


#Diferentes ficheros de configuracion
for i in `ls $corpus/conf/*`; do
 name=`basename $i .cnf`

 run=bash
 if test `hostname` = "node001"; then run=qsub; fi

$run << EORUN
#!/bin/bash -x
#PBS -N ${corpus}_${name}
#PBS -m abe
#PBS -V

if test -f $HOME/.bashrc; then . $HOME/.bashrc;fi
cd $workdir

lineas=\$(cat $corpus/models/test.ref | wc -l)

#Reconocimiento con iatros
../src/iatros-offline -c $i -t &> $dir/$corpus/$name.iatros

#Calculo de RTF
grep "xRT " $dir/$corpus/$name.iatros > $dir/$corpus/$name.tim 
tiempo=\$(cat $dir/$corpus/$name.tim | awk -v j=\$lineas 'BEGIN{i=0}{i=i+\$2;} END{printf("%.2f\n", (i/j))}')

#Calculo de WER
cat $dir/$corpus/$name.iatros | grep -v "WARNING\!\:" | grep -v "WARNING\:" | grep -v "xRT" | grep -v "Start decoding" > $dir/$corpus/$name.iatros2
paste -d '#' $workdir/$corpus/models/test.ref $dir/$corpus/$name.iatros2 > $dir/$corpus/$name.recout
#Filtro la salida
sed 's/ sil / /g' $dir/$corpus/$name.recout | sed 's/sil\$//g' | sed 's/^sil //g' | sed 's/<s> //g' | sed 's/ <\/s>\$//g' | sed "s/'?//g" | sed "s/<BACKOFF>//g" |sed "s/?//g" | sed 's/\.//g' | sed 's/\,//g' | sed "s/^ //g" | sed "s/# /#/g" | sed "s/\.#/#/g" | sed "s/'\!//g" | sed "s/\!//g" | sed "s/'//g" | sed 's/^ //g' | sed 's/^ *//g' | sed 's/* \$//g' | sed 's/<//g' | sed 's/>//g' | sed -e 's/\/s//g' | sed -e 's/\\\\//g' | sed -e 's/\%//g' | sed -e 's/sp//g'  > $dir/$corpus/$name.tasas

#Corpus Fichero_conf WER RTF
printf "%s %s %.2f %.2f\n" $corpus $name \$(tasas -s " " -f "#" -ie $dir/$corpus/$name.tasas) \$tiempo >> $dir/resultados

exit
EORUN

done
