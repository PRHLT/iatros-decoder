/*! \mainpage
 
  
 \section intro Introduction
 
  This is the documentation of the iAtros library, the new Improved Automatic Trainable RecOgnition System.

  For a short tutorial of how to use this library visit \subpage tutorial.

  The viterbi algorithm is divided into three stages: the \subpage initial, the \subpage viterbi and the \subpage end.
  To know more about the file formats visit \subpage models.


  \section bug Bug report
 
  If you want to notify a bug you can contact with:\n
  Audio system - vtamarit@iti.upv.es\n
  Viterbi - mlujan@iti.upv.es\n
  Anithing else - You can email both\n
  
  <b>Thanks for using iAtros!</b>

  \section acknowledgement Acklowledgement
  Supported by:

  \htmlonly
  <div style="display:block;text-align:center; width: 100%;">
    <style type="text/css">
      img {
        margin-right:40px;
        margin-left: 40px;
        border: 0px;
        vertical-align: middle;
      }
    </style>
    <a href="http://ec.europa.eu/"><img alt="MCIN" src="figures/logo-mcin.png"/></a>
    <a href="http://www.micinn.es/"><img alt="FEDER" src="figures/logo-feder.png"/></a> 
    <a href="http://miprcv.iti.upv.es/"><img alt="MIPRCV" src="figures/logo-MIPRCV.png"/></a>
  </div>
  \endhtmlonly
 */




/*! \page initial Initial stage

En la etapa inicial la idea es llenar el heap para pasarselo a viterbi_frame con algún elemento.

Si la lista de estados iniciales tiene algún elemento intentamos meter estas palabras en el heap sino intentamos meter los unigramas en el heap.

*/

/*! \page viterbi Viterbi frame

Inicialización de variables:

-> Se crea un nuevo heap para ir metiendo los nuevos estados que se van creando en la etapa.

-> Se crea una tablah hash asociada al nuevo heap.

-> Se utiliza un vector de probabilidades para apuntar de que elementos se ha llevado a cabo el cálculo de la probabilidad de emisión. Se inicializa el vector (t_probability).

-> Se utiliza otro vector con el mismo propósito para apuntar que estados han sido visitados en la tabla de palabras. Se inicializa el vector (visit_words).

Recorremos todos los elementos del heap de entrada:

-> El estado puede encontrarse en tres tipos de transiciones:

--> Transición de HMM.

--> Transición de modelo léxico.

--> Transición de modelo de lenguaje.


-> Comprobamos que el estado del heap que vamos a expandir supere el BEAM.

-> Transición de HMM:

--> Se calcula la nueva probabilidad del estado.

--> Se intenta insertar el nuevo estado en el nuevo heap (\subpage insert_beam).

-> Transición de modelo léxico:

--> Se recorren las múltiples pronunciaciones:

---> Se calcula la nueva probabilidad del estado.

---> Se intenta insertar el nuevo estado en el nuevo heap (\subpage insert_beam).

-> Transición de modelo de lenguaje:

--> Se inserta el nuevo estado en la tabla de palabras.

La expansión de estados que se encuentran en una transición de modelo de lenguaje se lleva a cabo cuando se ha recorrido todo el heap de entrada.
La idea es que los estados que se encuentran en una transición de modelo de lenguaje se insertan en primer lugar en la tabla de palabras y posteriormente se expanden todos los estados que se han insertado en la tabla de palabras, de forma que los estados que se unifican en el tabla de palabras sólo se expanden una vez. Mientras que si cada vez que se inserta un elemento en la tabla de palabras se expande, se expanden estados que van a ser reemplazados en el heap. De esta forma, se ahorra cálculo, pero el resultado es el mismo.

Cuando se ha recorrido todo el heap de entrada se expanden los estados de la tabla de palabras de esta etapa:

--> Si se trabaja con un trigrama, primero se expanden los trigramas correspondientes al estado a expandir, después se desciende por backoff y se expanden los bigramas y por último se expanden todos los unigramas, mientras todos ellos superen un umbral calculado respecto a una cota optimista de las probabilidades de emisión calculadas (\subpage insert_beam).

Si en el fichero de configuración se introduce un silencio o un espacio en blanco, se intenta introducir en el heap un nuevo estado con dicho símbolo.

Se libera la memoria y la tabla hash.

*/

/*! \page end End stage

La última etapa del reconocimiento tiene el mismo comportamiento que cualquier etapa anterior del reconocimiento pero sin consumir un cepstral, la idea es sumar la probabilidad de final al estado, siendo además necesario comprobar que el estado sea final de modelo léxico o de HMM.

*/

/*! \page insert_beam Insert element

-> Si el elemento supera el BEAM se intentará insertar en el nuevo heap, si directamente no supera el BEAM el nuevo estado se descarta.

-> Se crea un nuevo estado, es decir, se busca una posición libre en el pool de estados.

-> Se intenta insertar el nuevo estado en el heap:

--> Se busca el estado en la tabla hash.

---> Si el estado no se encuentra en la tabla hash:

----> Si el heap está lleno y el nuevo estado es mejor que el mínimo elemento del heap entonces se elimina el mínimo elemento del heap.

----> Si el heap no está lleno se inserta el nuevo estado en el heap y en la tabla hash y se incrementa el número de elementos del pool de estados.

---> Si el estado se encuentra en la tabla hash:

----> Si la probabilidad del nuevo estado es mejor se reemplaza la probabilidad del estado del heap por la probabilidad del nuevo estado. Y se ordena el heap, es decir, el elemento se baja en el heap, ya que el heap es un min-heap.

*/


/*! \page models Models

\section intro Introduction

En esta sección se muestran ejemplo de las estructuras que se utilizan para guardar los datos de los modelos de lenguaje, acústicos y léxicos.

\section grammar Language models

iATROS soporta (grammar) n-gramas en formato arpa y modelos de estados finitos (FSM) en formato v2.

Ejemplo de modelo de lenguaje en formato arpa:
\verbatim
\data\
ngram 1=5
ngram 2=4
ngram 3=3

\1-grams:
-0.60206 </s> 0
-99 <s> -99
-0.60206 administracion -99
-0.60206 de -99
-0.60206 fuentes -99

\2-grams:
0 <s> administracion 0
0 administracion de 0
0 de fuentes 0
0 fuentes </s> 0

\3-grams:
0 <s> administracion de
0 administracion de fuentes
0 de fuentes </s>

\end\
\endverbatim

Al codificar las palabras como números obtenemos:
\verbatim
<s> 0
</s> 1
administracion 15
de 79
fuentes 142
\endverbatim

Las palabras se codifican con números tan altos porque en el modelo léxico se pueden encontrar más palabras.

<img src="figures/grammar.jpg" height="200" width="250">

El dibujo muestra la estructura donde se guardan los modelos de lenguaje (tanto n-gramas como FSM).

Grammar tiene una tabla hash, un vector, el número de estados del modelo y el grado (n).
La tabla hash se utiliza para preprocesar la información del modelo de lenguaje y enlazar los estados con backoff, pero en el proceso de decodificación la tabla hash no se utiliza.
El vector tiene todos los estados del modelo. Los estados tienen un vector con las palabras que salen del estado, el número de palabras, el nombre del estado, el número de palabras del estado, ya que el nombre del estado puede tener n-1 palabras, la probabilidad de bo, un puntero al estado de bo y el número del estado.

En el caso del ejemplo:

El estado 0 tiene el vector con todos los unigramas, el nombre está vacio y no tiene bo.

El estado 1 tiene como nombre "</s>" pero no tiene palabras.

El estado 2 tiene como nombre "<s>" y continua con la palabra administración que es el estado 6.

El estado 9 tiene como nombre "fuentes </s>" no tiene palabras pero bo es el estado 1 que tiene la palabra "</s>".

\section hmm HMM models

Ejemplo de un HMM:

<img src="figures/hmm.jpg" height="200" width="250">

La idea de principal de esta estructura es la posibilidad de compartir matrices, estados, ... en un hmm. Por lo tanto, la estructura tiene fonemas, estados, matrices, distribuciones (mixturas de gaussianas), medias, varianzas, número de características y el número de elento que hay de cada tipo.

El fonema "B" tiene 5 estados y el estado 1 tiene 32 gaussianas. La idea es que si los fonemas comparten estados, el estado se guarda en la estructura hmm y el fonema apunta al estado correspondiente.


\section lex Lexical models

Los modelos léxicos (lex) que soporta iATROS deben estar escritos en formato v2 y se puede trabajar con múltiples pronunciaciones.

Ejemplo de un modelo léxico.

Si tenemos el siguiente modelo en v2:
\verbatim
Name "10"
State 0 i=1
0 1 "1" p=0.6
1 2 "0" p=1
2 3 "@" p=1
0 4 "1" p=0.4
4 5 "0" p=1
5 6 "." p=1
6 3 "@" p=1
State 3 f=1
\endverbatim

<img src="figures/lexdia.jpg" height="1" width="2">
<img src="figures/lex.jpg" height="200" width="250">

La estructura que guarda los modelos léxicos tiene en este caso 6172 modelos cargados. En el dibujo se muestra el modelo correspondiente al número 10. Tiene 7 estados y el estado inicial es el 0 y el final el 3. Y el número que se ha asignado a la palabra 10 es el número 17.
En estados se puede encontrar la información referente a casa estado, cada estado guarda el nombre, el número de aristas y la información de cada arista. Se muestran algunas aristas en las que se puede ver que las aristas guardan el estado destino, la palabra que hay en la arista, el fonema y el logaritmo de la probabilidad.


*/


/** \page tutorial Tutorial
 
This is a short tutorial of how to use the viterbi library. 

\section code The code

The following is a code snippet that loads the models and decodes a 
feature file. 

\code
// include the minimum set of headers
#include <iatros/decoder.h>
#include <iatros/lattice.h>
#include <iatros/search.h>
#include <iatros/viterbi.h>
\endcode

\code
// create a decoder from a config file
FILE *file = fopen("iatros.cnf", "r");
decoder_t *decoder = decoder_create_from_file(file);


// load the features
features_t *feas = features_create_from_file("sentence.fea");

// create a search instance from the decoder
search_t *search = search_create(decoder);

// create an empty lattice where the results will be stored
lattice_t *lattice = lattice_create_from_args(args, decoder);

// perform the decoding
decode(search, feas, lattice);

// print the output
symbol_t *sentence;
float prob = lattice_best_hyp(lattice, &sentence);
if (sentence != NULL) {
  char *sentence_str = NULL;
  extended_vocab_symbols_to_string(sentence, lattice->decoder->vocab, &sentence_str);
  printf("%s\n", sentence_str);
  free(sentence);
  free(sentence_str);
} else {
  printf("Sentence not recognized\n");
}


// free the resources
fclose(file);
search_delete(search);
lattice_delete(lattice);
features_delete(features);
decoder_delete(decoder);
\endcode

\section compiling Compiling

To compile, make sure that the include and lib directories are visible to the compiler.
If you followed a local installation, make sure to append the options `-Ilocal_dir' and `-Llocal_dir'.
If the libraries are installed in the system that is not necessary.

In addition, you have to link the libraries with your program. To do so, append the following options
to the compiler: `-lprhlt -liatros' for the dynamic linking, and `-lprhlt_nonshared -liatros_nonshared' 
for the static version.

 */
