/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* 
 * File:   main.cpp
 * Created on 10 de octubre de 2013, 16:27
 * Author: victor Fuentes
 * University of the Basque country / Universidad del Pais Vasco (UPV/EHU)
 * I2T Research Group
 */

#include <cstdlib>
#include <signal.h>
#include <sys/wait.h>
#include <rofl/platform/unix/cunixenv.h>
#include <rofl/common/ciosrv.h>
#include "proxy/ALHINP.h"





#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <microhttpd.h>

#define PAGE "<html><head><title>libmicrohttpd demo</title></head><body>libmicrohttpd demo</body></html>"

using namespace rofl;




//Handler to stop ciosrv
void interrupt_handler(int dummy=0) {
	//Only stop ciosrv 
	ciosrv::stop();
}
void TCL_finish(int dummy=0) {
	//Only stop ciosrv 
    wait(NULL);
    
}
class ALHINP;
ALHINP* ALHINP::singleton = NULL;
bool ALHINP::instanceFlag=false;
ALHINP* ALHINP::getInstance(){
    if(! ALHINP::instanceFlag){
        ALHINP::singleton = new ALHINP();
        ALHINP::instanceFlag = true;
        return singleton;
    }else{
        return singleton;
    }
}


static int ahc_echo(void * cls,
		    struct MHD_Connection * connection,
		    const char * url,
		    const char * method,
                    const char * version,
		    const char * upload_data,
		    size_t * upload_data_size,
                    void ** ptr) {
    

      
  struct MHD_Response *response;
  int ret;
  ALHINP* instance;
  instance = ALHINP::getInstance();
  instance->test();
    //string to be contructed;
    const char *page = "<html><body>Hello, browser REST!</body></html>";
  response =
    MHD_create_response_from_buffer (strlen (page), (void *) page, 
				     MHD_RESPMEM_PERSISTENT);
  ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
  MHD_destroy_response (response);

  return ret;
}

int main(int argc, char** argv){


	//Capture control+C
	signal(SIGINT, interrupt_handler);
        signal(SIGCLD, TCL_finish);

	/* update defaults */
	rofl::csyslog::initlog(
			rofl::csyslog::LOGTYPE_FILE,//STDERR
			rofl::csyslog::DBG,//EMERGENCY
			std::string("./controlador.log"),
			"an example: ");

	rofl::csyslog::set_debug_level("ciosrv", "emergency");
	rofl::csyslog::set_debug_level("cthread", "emergency");

        rofl::ciosrv::init();
        
//        //ALHINP* ofdocsis;
//        //ofdocsis = ALHINP::getInstance();
        ALHINP ofdocsis (argv[1]);
        
//        struct MHD_Daemon * d;
//        d = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY,
//		       8080,
//		       NULL,
//                       NULL,
//		       &ahc_echo,
//		       NULL,
//                       MHD_OPTION_END);
        
        
        //ciosrv run. Only will stop in Ctrl+C
	rofl::ciosrv::run();

	//Printing nice trace
	printf("\nCleaning the house...\n");
        fflush(stdout);
        
	//ciosrv destroy
	rofl::ciosrv::destroy();
//        MHD_stop_daemon(d);
	printf("House cleaned!\nGoodbye\n");
        fflush(stdout);
	
	exit(EXIT_SUCCESS);
}

