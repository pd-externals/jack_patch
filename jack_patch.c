/* jack utility externals for linux pd
 * copyright 2003 Gerard van Dongen gml@xs4all.nl
 * copyright 2022 Roman Haefeli <reduzent@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * jack_patch
 * this can query and set the port connections on the jack system
 */

#include "m_pd.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <jack/jack.h>

#define CLASS_NAME "jack_patch"

static t_class *jackpatch_class;

typedef struct _jackpatch
{
    t_object x_obj;
    char source[321];
    char destination[321];
    t_outlet *connected, *input_ports, *output_ports;
    char expression[128];
    char *buffer; //used internally it doesn't have to be reserved every time
    t_atom *a_outlist;
} t_jackpatch;

static jack_client_t *jc;

jack_client_t * jackx_get_jack_client()
{
    if (!jc)
    {
        jack_status_t status;
        jack_options_t options = JackNoStartServer;
        jc = jack_client_open ("jackx-pd", options, &status, NULL);
        if (status & JackServerFailed) {
            pd_error(NULL, "jackx: unable to connect to JACK server");
            jc = NULL;
        }
        if (status) {
            if (status & JackServerStarted) {
                verbose(1, "jackx: started server");
            } else {
                pd_error(NULL, "jackx: server returned status %d", status);
            }
        }
    }
    return jc;
}

int jackpatch_getnames(t_jackpatch *x,
                                t_symbol *output_client, t_symbol *output_port,
                                t_symbol *input_client, t_symbol *input_port)
{
    // check if all arguments have been specified
    if(!strcmp(input_port->s_name, "")
        || !strcmp(input_client->s_name, "")
        || !strcmp(output_port->s_name, "")
        || !strcmp(output_client->s_name, ""))
    {
        logpost(x, 1, "%s: not enough arguments given", CLASS_NAME);
        return 1;
    }
    char* to = x->source;
    to = (char*)stpcpy( to, output_client->s_name);
    to = (char*)stpcpy(to,":");
    to = (char*)stpcpy(to, output_port->s_name);
    to = x->destination;
    to = (char*)stpcpy(to, input_client->s_name);
    to = (char*)stpcpy(to,":");
    to = (char*)stpcpy(to, input_port->s_name);
    return 0;
}

void jackpatch_connect(t_jackpatch *x,
                                t_symbol *output_client, t_symbol *output_port,
                                t_symbol *input_client, t_symbol *input_port)
{
    if (jc)
    {
        int status;
        int connected = 0;
        if(jackpatch_getnames(x, output_client, output_port, input_client, input_port))
            return;
        logpost(x, 3,
                "[%s] connecting '%s' --> '%s'", CLASS_NAME,  x->source, x->destination);
        status = jack_connect(jc, x->source, x->destination);
        if ((!status) || status == EEXIST) connected = 1;
        outlet_float(x->connected, connected);
    } else {
        logpost(x, 1, "%s: JACK server is not running", CLASS_NAME);
    }
}

void jackpatch_disconnect(t_jackpatch *x,
                                t_symbol *output_client, t_symbol *output_port,
                                t_symbol *input_client, t_symbol *input_port)
{
    if (jc)
    {
        if(jackpatch_getnames(x, output_client, output_port, input_client, input_port))
            return;
        jack_disconnect(jc, x->source, x->destination);
        outlet_float(x->connected, 0);
        logpost(x, 3,
                "[%s] disconnecting '%s' --> '%s'", CLASS_NAME, x->source, x->destination);
    } else {
        logpost(x, 1, "%s: JACK server is not running", CLASS_NAME);
    }
}

void jackpatch_query(t_jackpatch *x,
                    t_symbol *output_client, t_symbol *output_port,
                    t_symbol *input_client, t_symbol *input_port)
{
    if (jc)
    {
        const char **ports;
        int n=0;
        if(jackpatch_getnames(x, output_client, output_port, input_client, input_port))
            return;
        logpost(x, 3,
                "[jack-connect] querying connection '%s' --> '%s'", x->source, x->destination);
        ports = jack_port_get_all_connections(jc,(jack_port_t *)jack_port_by_name(jc, x->source));
        int connected = 0;
        if(ports)
        {
            while (ports[n])
            {
                logpost(x, 4,
                        "n = %i", n);
                if (!strcmp(ports[n], x->destination))
                {
                    connected = 1;
                    break;
                }
                n++;
            }
            jack_free(ports);
        }
        outlet_float(x->connected, connected);
    } else {
        logpost(x, 1, "%s: JACK server is not running", CLASS_NAME);
    }
}

void jackpatch_get_connections(t_jackpatch *x,
                    t_symbol *client, t_symbol *port)
{
    if (jc)
    {
        const char **ports;
        int l = 0;
        int n = 0;
        t_symbol *s_port, *s_client;
        char *t;
        if(jackpatch_getnames(x, client, port, client, port))
            return;
        logpost(x, 3,
                "[jack-connect] querying connection '%s' --> '%s'", x->source, x->destination);
        ports = jack_port_get_all_connections(jc,(jack_port_t *)jack_port_by_name(jc, x->source));
        if(ports)
        {
            while (ports[n])
            {
                l = strlen(ports[n]);
                t = strchr(ports[n],':');
                if (t)
                {
                    s_port = gensym(strchr(ports[n], ':') + 1);
                    int clientlen = l - strlen(s_port->s_name) - 1;
                    strncpy(x->buffer, ports[n], clientlen);
                    x->buffer[clientlen] = '\0';
                    s_client = gensym(x->buffer);
                    SETSYMBOL(x->a_outlist,s_client);
                    SETSYMBOL(x->a_outlist+1,s_port);
                    outlet_list(x->output_ports,&s_list,2, x->a_outlist);
                }
                n++;
            }
        }
        jack_free(ports);
    }
}

void jackpatch_input(t_jackpatch *x, t_symbol *s,int argc, t_atom *argv)
{
    if (jc)
    {
        const char ** ports;

        int l = 0;
        int n = 0;
        int keyflag = 0;
        int expflag =0;
        int portflags = 0;
        t_symbol *s_client;
        t_symbol *s_port;
        char *t;

        if (!strcmp(s->s_name,"bang"))
        {
            strcpy(x->expression,"");
            expflag = 1;
        }
        else
        {

            //parse symbol s and all arguments for keywords:
            //physical,virtual,input and output

            if (!strcmp(s->s_name,"physical"))
            {
                portflags = portflags | JackPortIsPhysical;
                keyflag = 1;
            }
            if (!strcmp(s->s_name,"virtual"))
            {
                portflags = portflags & (~JackPortIsPhysical);
                keyflag = 1;
            }
            if (!strcmp(s->s_name,"input"))
            {
                portflags = portflags | JackPortIsInput;
                keyflag = 1;
            }

            if (!strcmp(s->s_name,"output"))
            {
                portflags = portflags | JackPortIsOutput;
                keyflag = 1;
            }
            if (!keyflag)
            {
                strcpy(x->expression,s->s_name);
                expflag = 1;
            }
            for (n=0; n<argc; n++)
            {
                keyflag = 0;
                atom_string(argv+n, x->buffer,128);
                if (!strcmp(x->buffer,"physical"))
                {
                    portflags = portflags | JackPortIsPhysical;
                    keyflag = 1;
                }
                if (!strcmp(x->buffer,"virtual"))
                {
                    portflags = portflags & (~JackPortIsPhysical);
                    keyflag = 1;
                }
                if (!strcmp(x->buffer,"input"))
                {
                    portflags = portflags | JackPortIsInput;
                    keyflag = 1;
                }

                if (!strcmp(x->buffer,"output"))
                {
                    portflags = portflags | JackPortIsOutput;
                    keyflag = 1;
                }
                if (!keyflag && !expflag)
                {
                    strcpy(x->expression, x->buffer);
                    expflag = 1;
                }

            }
        }
        ports = jack_get_ports (jc, x->expression,NULL,portflags|JackPortIsOutput);
        n=0;
        if (ports)
        {
            while (ports[n])
            {
                //seperate port and client

                l = strlen(ports[n]);
                t = strchr(ports[n],':');

                if (t)
                {
                    s_port = gensym(strchr(ports[n], ':') + 1);

                    int clientlen = l - strlen(s_port->s_name) - 1;
                    strncpy(x->buffer, ports[n], clientlen);
                    x->buffer[clientlen] = '\0';
                    s_client = gensym(x->buffer);

                    SETSYMBOL(x->a_outlist,s_client);
                    SETSYMBOL(x->a_outlist+1,s_port);

                    // output in output-outlet
                    outlet_list(x->output_ports,&s_list,2, x->a_outlist);
                }
                n++;
            }
        }
        free(ports);
        ports = jack_get_ports (jc, x->expression,NULL,portflags|JackPortIsInput);
        n=0;
        if (ports)
        {
            while (ports[n])
            {
                l = strlen(ports[n]);
                t = strchr(ports[n],':');
                if (t)
                {
                    s_port = gensym(strchr(ports[n], ':') + 1);

                    int clientlen = l - strlen(s_port->s_name) - 1;
                    strncpy(x->buffer, ports[n], clientlen);
                    x->buffer[clientlen] = '\0';
                    s_client = gensym(x->buffer);

                    SETSYMBOL(x->a_outlist,s_client);
                    SETSYMBOL(x->a_outlist+1,s_port);

                    // output in output-outlet
                    outlet_list(x->input_ports,&s_list,2, x->a_outlist);
                }
                n++;
            }
        }
        free(ports);
        strcpy(x->expression,"");//reset regex
    } else {
        logpost(x, 1, "%s: JACK server is not running", CLASS_NAME);
    }
}

void *jackpatch_new(void)
{
    t_jackpatch * x = (t_jackpatch *)pd_new(jackpatch_class);
    x->connected = outlet_new(&x->x_obj, &s_float);
    x->output_ports = outlet_new(&x->x_obj, &s_list);
    x->input_ports = outlet_new(&x->x_obj, &s_list);
    x->a_outlist = getbytes(3 * sizeof(t_atom));
    x->buffer = getbytes(128);
    return (void*)x;
}

void jack_patch_setup(void)
{
    jc = jackx_get_jack_client();

    jackpatch_class = class_new(gensym(CLASS_NAME), (t_newmethod)jackpatch_new,
                                  0, sizeof(t_jackpatch), CLASS_DEFAULT, 0);

    class_addmethod(jackpatch_class, (t_method)jackpatch_connect, gensym("connect"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_disconnect, gensym("disconnect"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_query, gensym("query"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_get_connections, gensym("get_connections"),
        A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addanything(jackpatch_class, jackpatch_input);
}
