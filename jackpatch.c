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
 * jackpatch
 * this can query and adjust the port connections on the jack system
 */

#include "m_pd.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <jack/jack.h>

#define CLASS_NAME "jackpatch"

static t_class *jackpatch_class;

typedef struct _jackpatch
{
    t_object x_obj;
    char source[321];
    char destination[321];
    t_outlet *output;
    char expression[321];
    char *buffer; //used internally it doesn't have to be reserved every time
    t_symbol *outputsel;
    t_symbol *inputsel;
    t_atom *a_outlist;
} t_jackpatch;

static jack_client_t *jc;

void jackpatch_reset_client() {
    jc = (jack_client_t *)NULL;
}

jack_client_t * jackpatch_get_jack_client()
{
    if (!jc)
    {
        jack_status_t status;
        jack_options_t options = JackNoStartServer;
        jc = jack_client_open ("jackpatch-pd", options, &status, NULL);
        jack_on_shutdown(jc, jackpatch_reset_client, (void *)NULL);
    }
    return jc;
}

void jackpatch_output_ports(t_jackpatch *x, const char **ports)
{
    int l = 0;
    int n = 0;
    int portflags = 0;
    char *t;
    t_symbol *s_client, *s_port;
    t_symbol *s_sel = x->inputsel;
    if (ports)
    {
        while (ports[n])
        {
            l = strlen(ports[n]);
            t = strchr(ports[n],':');
            if (t)
            {
                portflags = jack_port_flags(jack_port_by_name(jc, ports[n]));
                s_port = gensym(strchr(ports[n], ':') + 1);
                int clientlen = l - strlen(s_port->s_name) - 1;
                strncpy(x->buffer, ports[n], clientlen);
                x->buffer[clientlen] = '\0';
                s_client = gensym(x->buffer);
                SETSYMBOL(x->a_outlist,s_client);
                SETSYMBOL(x->a_outlist+1,s_port);
                if(portflags & JackPortIsOutput) s_sel = x->outputsel;
                outlet_anything(x->output,s_sel,2, x->a_outlist);
            }
            n++;
        }
    }
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
        pd_error(x, "%s: not enough arguments given", CLASS_NAME);
        return 1;
    }
    sprintf(x->source, "%s:%s", output_client->s_name, output_port->s_name);
    sprintf(x->destination, "%s:%s", input_client->s_name, input_port->s_name);
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
        outlet_float(x->output, connected);
    } else {
        pd_error(x, "%s: JACK server is not running", CLASS_NAME);
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
        outlet_float(x->output, 0);
        logpost(x, 3,
                "[%s] disconnecting '%s' --> '%s'", CLASS_NAME, x->source, x->destination);
    } else {
        pd_error(x, "%s: JACK server is not running", CLASS_NAME);
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
        outlet_float(x->output, connected);
    } else {
        pd_error(x, "%s: JACK server is not running", CLASS_NAME);
    }
}

void jackpatch_get_connections(t_jackpatch *x, t_symbol *client, t_symbol *port)
{
    if (jc)
    {
        const char **ports;
        if(jackpatch_getnames(x, client, port, client, port))
            return;
        logpost(x, 3,
                "[jack-connect] querying connection '%s' --> '%s'", x->source, x->destination);
        ports = jack_port_get_all_connections(jc,(jack_port_t *)jack_port_by_name(jc, x->source));
        jackpatch_output_ports(x, ports);
        jack_free(ports);
    } else {
        pd_error(x, "%s: JACK server is not running", CLASS_NAME);
    }
}

void jackpatch_get_outputs(t_jackpatch *x, t_symbol *client, t_symbol *port)
{
    if (jc)
    {
        const char ** ports;
        int portflags = JackPortIsOutput;
        sprintf(x->expression, "%s:%s", client->s_name, port->s_name);
        ports = jack_get_ports (jc, x->expression,NULL,portflags);
        jackpatch_output_ports(x, ports);
        jack_free(ports);
    } else {
        pd_error(x, "%s: JACK server is not running", CLASS_NAME);
    }
}

void jackpatch_get_inputs(t_jackpatch *x, t_symbol *client, t_symbol *port)
{
    if (jc)
    {
        const char ** ports;
        int portflags = JackPortIsInput;
        sprintf(x->expression, "%s:%s", client->s_name, port->s_name);
        ports = jack_get_ports (jc, x->expression,NULL,portflags);
        jackpatch_output_ports(x, ports);
        jack_free(ports);
    } else {
        pd_error(x, "%s: JACK server is not running", CLASS_NAME);
    }
}

void jackpatch_get_clients(t_jackpatch *x)
{
    if (jc)
    {
        const char ** ports;
        int n = 0;
        char *t;
        int cl;
        t_symbol *s_client;
        t_symbol *s_prevclient = (t_symbol *)NULL;
        ports = jack_get_ports (jc, NULL, NULL, (long unsigned int)NULL);
        if (ports)
        {
            while (ports[n])
            {
                t = strchr(ports[n], ':');
                cl = t - ports[n];
                if (t)
                {
                    strcpy(x->buffer, ports[n]);
                    (*(x->buffer + cl)) = '\0';
                    s_client = gensym(x->buffer);
                    SETSYMBOL(x->a_outlist,s_client);
                    if(s_client != s_prevclient)
                    {
                        outlet_symbol(x->output,s_client);
                    }
                    s_prevclient = s_client;
                }
                n++;
            }
        }
        jack_free(ports);
    } else {
        pd_error(x, "%s: JACK server is not running", CLASS_NAME);
    }
}

void jackpatch_is_running(t_jackpatch *x)
{
    if(jc)
    {
        outlet_float(x->output, 1);
    } else {
        outlet_float(x->output, 0);
    }
}

void jackpatch_free(t_jackpatch *x)
{
    free(x->buffer);
    free(x->a_outlist);
}

void *jackpatch_new(void)
{
    t_jackpatch * x = (t_jackpatch *)pd_new(jackpatch_class);
    x->output = outlet_new(&x->x_obj, 0);
    x->a_outlist = getbytes(3 * sizeof(t_atom));
    x->buffer = getbytes(128);
    x->outputsel = gensym("output");
    x->inputsel = gensym("input");
    jc = jackpatch_get_jack_client();
    return (void*)x;
}

void jackpatch_setup(void)
{
    jackpatch_class = class_new(gensym(CLASS_NAME), (t_newmethod)jackpatch_new,
                                (t_method)jackpatch_free, sizeof(t_jackpatch),
                                CLASS_DEFAULT, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_connect, gensym("connect"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_disconnect, gensym("disconnect"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_query, gensym("query"),
        A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_get_connections, gensym("get_connections"),
        A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_get_outputs, gensym("get_outputs"),
        A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_get_inputs, gensym("get_inputs"),
        A_DEFSYMBOL, A_DEFSYMBOL, 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_get_clients, gensym("get_clients"), 0);
    class_addmethod(jackpatch_class, (t_method)jackpatch_is_running, gensym("is_running"), 0);
}
