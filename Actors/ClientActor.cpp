#include "ClientActor.h"
#include <string>

const char * clientCapabilities =
                                "capabilities\n"
                                "    data\n"
                                "        name = \"ip\"\n"
                                "        type = \"string\"\n"
                                "        value = \"192.168.0.1\"\n"
                                "        api_call = \"SET HOST\"\n"
                                "        api_value = \"s\"\n"           // optional picture format used in zsock_send
                                "    data\n"
                                "        name = \"port\"\n"
                                "        type = \"int\"\n"
                                "        value = \"6200\"\n"
                                "        min = \"1\"\n"
                                "        max = \"10000\"\n"
                                "        api_call = \"SET PORT\"\n"
                                "        api_value = \"i\"\n"           // optional picture format used in zsock_send
                                "inputs\n"
                                "    input\n"
                                "        type = \"OSC\"\n";

zmsg_t* Client::handleMsg( sphactor_event_t * ev ) {

    if ( streq(ev->type, "INIT") ) {
        //init capabilities
        sphactor_actor_set_capability((sphactor_actor_t*)ev->actor, zconfig_str_load(clientCapabilities));
    }
    /*
     * Application will hang if done here when closing application entirely...
     * Even if you have destroyed client actors in between... very odd...
     * */
    else if ( streq(ev->type, "DESTROY") ) {
        zsys_info("Handling Destroy");
        //TODO: Fix zsys_shutdown crash when destroying dgrams along the way...
        if ( this->dgrams != NULL ) {
            //zsock_destroy(&this->dgrams);
            this->dgrams = NULL;
        }

        return ev->msg;
    }
    else if ( streq(ev->type, "SOCK") ) {
        if ( ev->msg == NULL ) return NULL;
        if ( this->dgrams == NULL ) return ev->msg;



        byte *msgBuffer;
        zframe_t* frame;

        do {
        frame = zmsg_pop(ev->msg);
            if ( frame ) {
                //parse zosc_t msg
                msgBuffer = zframe_data(frame);
                size_t len = zframe_size(frame);

                //TODO: figure out if we can send this directly from the zframe_data or not...
                //zosc_t * oscMsg = zosc_frommem( (char*)msgBuffer, len);
                int rc = zsock_send( dgrams, "b", msgBuffer, len);//zosc_data(oscMsg), zosc_size(oscMsg));
                if ( rc != 0 ) {
                    zsys_info("Error sending zosc message to: %s", this->host.c_str());
                }
            }
        } while (frame != NULL );

        zmsg_destroy(&ev->msg);

        return NULL;
    }
    else if ( streq(ev->type, "API")) {
        //pop msg for command
        char * cmd = zmsg_popstr(ev->msg);
        if (cmd) {
            if ( streq(cmd, "SET PORT") ) {
                if ( this->dgrams != NULL ) {
                    zsock_destroy(&this->dgrams);
                }

                char * port = zmsg_popstr(ev->msg);

                this->port = port;
                std::string url = ("udp://" + this->host + ":" + this->port);
                this->dgrams = zsock_new_dgram (url.c_str());
                if ( this->dgrams != NULL ) {
                    zsys_info("Could not create dgram: %s", url.c_str());
                }

                zsys_info("SET PORT: %s", url.c_str());

                zstr_free(&port);
            }
            else if ( streq(cmd, "SET HOST") ) {
                if ( this->dgrams != NULL ) {
                    zsock_destroy(&this->dgrams);
                }

                char * host_addr = zmsg_popstr(ev->msg);
                this->host = host_addr;

                std::string url = ("udp://" + this->host + ":" + this->port);
                this->dgrams = zsock_new_dgram (url.c_str());
                if ( this->dgrams != NULL ) {
                    zsys_info("Could not create dgram: %s", url.c_str());
                }

                zsys_info("SET HOST: %s", url.c_str());

                zstr_free(&host_addr);
            }

            zstr_free(&cmd);
        }

        zmsg_destroy(&ev->msg);

        return NULL;
    }

    return ev->msg;
}
