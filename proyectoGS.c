/*
 *
 * Proyecto GStreamer - Aplicaciones Multimedia (2020-2021)
 * Universidad Carlos III de mpg123audiodecrid
 *
 * Equipo (TO DO: rellenar con los datos adecuados):
 * - Alumno 1 (nombre, apellidos y NIA)
 * - Alumno 2 (nombre, apellidos y NIA)
 *
 * Versión implementada (TO DO: eliminar las líneas que no procedan):
 * - pipeline
 * - reproducción mp3 visualmente (goom)
 * - conversión mp3 a ogg
 *  - manejar eventos de teclado (mostrar tiempo)
 */

  // ------------------------------------------------------------
  // Procesar argumentos
  // ------------------------------------------------------------

  // REF: https://www.gnu.org/software/libc/manual/html_node/Parsing-Program-Arguments.html#Parsing-Program-Arguments

  /*
   * Argumentos del programa:
   * -h: presenta la ayuda y termina (con estado 0).
   * -f fichero_mp3: nombre del fichero del audio de entrada en formato MP3.
   * -o fichero_ogg: nombre del fichero del audio que se va a generar en formato OGG.
   * -t: responde al evento de teclado para poner el instante de tiempo enla imagen visualizada.
   */
#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data){
  GMainLoop * loop = ( GMainLoop *) data ;


  switch ( GST_MESSAGE_TYPE ( msg )){
    case GST_MESSAGE_EOS:
      g_print ("[bus] ( ) :: End of stream \n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR:{
      GError *err;
      gchar *debug;

      gst_message_parse_error (msg, &err, &debug);
      g_print ("Error: %s\n", err->message);
      g_error_free (err);
      g_free (debug);

      g_main_loop_quit (loop);
      break;
    }



    default: {

      break;
    }
  }

  return TRUE;
}

int main (int   argc, char *argv[])
{
  //Tuberia
  GstElement *pipeline;
  //sumideros y fuentes
  GstElement *filesrc,*alsasink,*ximagesink,*filesink; //4
  //Codificadores,decodificadores y parsers
  GstElement *mpegaudioparse,*vorbisenc,*mpg123audiodec;//3
  //Convertidores
  GstElement *audioconvert,*audio_converter_file, *audio_eco_convert,*audio_eco_convert2,*videoconvert;//5
  //Efectos
  GstElement *goom,*kaleidoscope,*tunnel,*timeoverlay,*audioecho_sound,*audioecho_file;//6
  //Separador,colas y muxer
  GstElement *tee, *queue1, *queue2 , *queue3,*oggmux; //5
  //Bucle
  GMainLoop *loop;
  //Bus
  GstBus *bus;
  //Variables que nos serviran para seleccionar y controlar los parametros seleccionados
  gboolean genera_fichero = FALSE;
  gboolean original_sound = TRUE;
  //Variables para las direcciones
  char *mp3_file;
  char *ogg_file = NULL;

  if(argc<2){
    g_print("Numero de argumentos incorrecto");
  }

  int c;

  opterr = 0;  // no es necesario declararla, la exporta getopt

  while ((c = getopt (argc, argv, "hto:f:")) != -1) {
    switch (c)
      {
      case 'h':
	// ayuda
	g_print("usage: %s -t -o <fichero_ogg> -f <fichero_mp3>.mp3\n", argv[0]);
  g_print("Funcionamiento:\nEl parametro -f es obligatorio.\nPoner el parametro t implica añadir efecto de eco.\nOmitir el parametro -o implica no generar un archivo ogg de salida.\n");

        return 0;

      case 'f':

    mp3_file = optarg;
        break;

      case 'o':

	      g_print("Has elegido la opcion que genera un archivo ogg.\n");
        genera_fichero = TRUE;
        ogg_file = optarg;

        break;

      case 't':

	      g_print("Ha elegido la opcion que reproduce el sonido con efecto eco.\n");
        original_sound=FALSE;
        break;

      case '?':
	// getopt devuelve '?' si encuentra una opción desconocida
	// o si falta el argumento para una opción que lo requiere
	// La opción conflictiva queda almacenada en optopt
        if ((optopt == 'o') ) {
	  // falta argumento para opción que lo requiere
          fprintf (stderr, "Error: la opción -%c requiere un argumento\n", optopt);
		}
		// error: opción desconocida
        else if (isprint (optopt))
          fprintf (stderr, "Error: argumento `-%c' no válido\n", optopt);
        else
          fprintf (stderr, "Error: argumento `\\x%x' no válido.\n", optopt);
        return 1;

      default:
        fprintf (stderr,
                 "Error: argumento %d no válido\n", optind);

        return 1;
      }
  }

   // getopt recoloca los argumentos no procesados al final
  // el primero será el nombre del fichero de entrada (es correcto)
  // si hay algún otro: error argumento desconocido
  int index = 0;
  for (index = optind+1; index < argc; index++) {
    printf ("Error: argumento %s no válido\n", argv[index]);
    return 1;
  }

  gst_init (&argc, &argv);


  loop = g_main_loop_new(NULL, FALSE);




  pipeline = gst_pipeline_new ("audio-player");


  //creacion de factorias para todos los elementos
  filesrc = gst_element_factory_make("filesrc","file-source");
  mpegaudioparse = gst_element_factory_make("mpegaudioparse","mp3-parser");
  mpg123audiodec = gst_element_factory_make ("mpg123audiodec","mp3-decoder");
  audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");
  audio_converter_file = gst_element_factory_make ("audioconvert", "audio_converter_file");
  audio_eco_convert = gst_element_factory_make ("audioconvert", "audio_eco_convert");
  audio_eco_convert2 = gst_element_factory_make ("audioconvert", "audio_eco_convert2");
  goom = gst_element_factory_make ("goom","efecto_goom");
	videoconvert = gst_element_factory_make ("videoconvert","video-converter");
  ximagesink = gst_element_factory_make ("ximagesink", "video_output");
  tee = gst_element_factory_make ("tee", "Tee");
  queue1 = gst_element_factory_make ("queue",NULL);
  queue2 = gst_element_factory_make ("queue",NULL);
  queue3 = gst_element_factory_make ("queue",NULL);
  vorbisenc = gst_element_factory_make ("vorbisenc","audio_ogg_encoder");
  oggmux = gst_element_factory_make ("oggmux","audio_ogg_muxer");
  filesink = gst_element_factory_make ("filesink","audio_ogg_filesink");
  alsasink = gst_element_factory_make ("alsasink","audio_ogg_alsasink");
  kaleidoscope = gst_element_factory_make ("kaleidoscope","kaleidoscopio");
  tunnel = gst_element_factory_make ("tunnel","tunnel-effect");
  timeoverlay = gst_element_factory_make ("timeoverlay","timeoverlay-effect");
  audioecho_sound = gst_element_factory_make ("audioecho","audioecho_sound");
  audioecho_file = gst_element_factory_make ("audioecho","audioecho_file");

  if (!pipeline || !filesrc || !mpegaudioparse || !mpg123audiodec
   || !tee || !queue1 || !queue2 || !queue3
   || !audioconvert || !goom || !videoconvert || !timeoverlay || !ximagesink || !audio_eco_convert || !audio_eco_convert2
   || !audio_converter_file || !vorbisenc || !oggmux || !filesink || !alsasink || !kaleidoscope || !tunnel || !audioecho_sound || !audioecho_file){
    g_printerr ("No se ha podido crear alguno de los elementos. Saliendo.\n");
    return -1;
  }



   g_print("Configurando dirección...: (%s)\n", mp3_file);
   //Configuramos la propiedad location con el valor de la direccion del archivo en cuestion
   g_object_set(G_OBJECT (filesrc), "location", mp3_file, NULL);

   //Configuramos las propiedades del efecto eco.
   g_object_set(G_OBJECT (audioecho_sound), "intensity", 0.8, NULL);
   g_object_set(G_OBJECT (audioecho_sound), "feedback", 0.9, NULL);
   g_object_set(G_OBJECT (audioecho_sound), "delay", 400000000, NULL);
   //Configuramos las propiedades del efecto eco 2 veces: una para el elemento-efecto del hilo que reproduce el audio,
   // y la segunda para el elemento-efecto del hilo que genera el archivo nuevo
   g_object_set(G_OBJECT (audioecho_file), "intensity", 0.8, NULL);
   g_object_set(G_OBJECT (audioecho_file), "feedback", 0.9, NULL);
   g_object_set(G_OBJECT (audioecho_file), "delay", 400000000, NULL);

   if (genera_fichero){
     strcat(ogg_file, ".ogg");
     g_print("Configurando direccion: (%s)\n", ogg_file);
     g_object_set(G_OBJECT(filesink), "location", ogg_file, NULL);
   }


   bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
   gst_bus_add_watch (bus, bus_call, loop);
   gst_object_unref (bus);

   //los if's siguientes son para cada uno de los 4 posibles casos en cuanto a los parametros de entrada introducidos.

   if (genera_fichero && !original_sound){
     g_print("Tuberia con salida OGG.........\n");

     gst_bin_add_many(GST_BIN (pipeline), filesrc,mpegaudioparse,mpg123audiodec,tee,
                      queue1,audioconvert,goom,videoconvert,kaleidoscope,tunnel,timeoverlay,ximagesink,
                      queue2,audio_eco_convert,audioecho_sound,audio_eco_convert2,alsasink,
                      queue3,audio_converter_file,audioecho_file,vorbisenc,oggmux,filesink, NULL);



    gst_element_link_many(filesrc,mpegaudioparse,mpg123audiodec,tee, NULL);
    gst_element_link_many(tee,queue1,audioconvert,goom,videoconvert,kaleidoscope,tunnel,timeoverlay,ximagesink, NULL);
    gst_element_link_many(tee, queue2,audio_eco_convert,audioecho_sound,audio_eco_convert2,alsasink,NULL);
    gst_element_link_many(tee, queue3,audio_converter_file,audioecho_file, vorbisenc,oggmux,filesink,NULL);

   }
   if(!genera_fichero && !original_sound){
    g_print("Tuberia sin salida OGG.........\n");

    gst_bin_add_many(GST_BIN (pipeline),filesrc,mpegaudioparse,mpg123audiodec,tee,
                     queue1,audioconvert,goom,videoconvert,kaleidoscope,tunnel,timeoverlay,ximagesink,
                     queue2,audio_eco_convert,audioecho_sound,audio_eco_convert2,alsasink,NULL);



    gst_element_link_many(filesrc,mpegaudioparse,mpg123audiodec,tee,NULL);
    gst_element_link_many(tee,queue1,audioconvert,goom,videoconvert,kaleidoscope,tunnel,timeoverlay,ximagesink,NULL);
    gst_element_link_many(tee,queue2,audio_eco_convert,audioecho_sound,audio_eco_convert2,alsasink, NULL);
  }
   if(genera_fichero && original_sound){
	   g_print("Tuberia con salida OGG y audio original.........\n");

     gst_bin_add_many(GST_BIN (pipeline),filesrc,mpegaudioparse,mpg123audiodec,tee,
                      queue1,audioconvert,goom,videoconvert,kaleidoscope,tunnel,timeoverlay,ximagesink,
                      queue2,audio_eco_convert,alsasink,
                      queue3,audio_converter_file,vorbisenc,oggmux, filesink,NULL);



    gst_element_link_many(filesrc,mpegaudioparse,mpg123audiodec,tee,NULL);
    gst_element_link_many(tee,queue1,audioconvert,goom,videoconvert,kaleidoscope,tunnel,timeoverlay,ximagesink, NULL);
    gst_element_link_many(tee,queue2,audio_eco_convert,alsasink,NULL);
    gst_element_link_many(tee,queue3,audio_converter_file,vorbisenc,oggmux,filesink,NULL);

   }

   if(!genera_fichero && original_sound){

    g_print("Tuberia sin salida OGG y audio original.........\n");

    gst_bin_add_many(GST_BIN (pipeline),filesrc,mpegaudioparse,mpg123audiodec,tee,
                      queue1,audioconvert,goom,videoconvert,kaleidoscope,tunnel,timeoverlay,ximagesink,
                      queue2,audio_eco_convert,alsasink,NULL);



    gst_element_link_many(filesrc,mpegaudioparse,mpg123audiodec,tee,NULL);
    gst_element_link_many(tee,queue1,audioconvert,goom,videoconvert,kaleidoscope,tunnel,timeoverlay,ximagesink,NULL);
    gst_element_link_many(tee,queue2,audio_eco_convert,alsasink,NULL);

}



  g_print("Reproduciendo .......\n");
  //Ponemos tuberia a funcionar
  gst_element_set_state (pipeline, GST_STATE_PLAYING);




  //Run the loops
  g_print("Running....\n");
  g_main_loop_run (loop);

  g_print("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));

  return 0;
}
